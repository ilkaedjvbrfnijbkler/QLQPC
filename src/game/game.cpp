#include "game/game.hpp"

#include "util/hook_engine.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <string>

namespace qlqpc::game
{
    static_assert(offsetof(ql_fog, color_int) == 28, "ql_fog.color_int");
    static_assert(offsetof(ql_fog, tc_scale) == 32, "ql_fog.tc_scale");
    static_assert(offsetof(ql_fog, color) == 36, "ql_fog.color");
    static_assert(offsetof(ql_fog, depth_for_opaque) == 48, "ql_fog.depth_for_opaque");
    static_assert(offsetof(ql_fog, has_surface) == 52, "ql_fog.has_surface");
    static_assert(sizeof(ql_fog) == 72, "ql_fog size");

    namespace
    {
        util::inline_hook vm_create_hook{};
        util::inline_hook vm_free_hook{};
        HMODULE active_cgame = nullptr;
        cgame_loaded_t on_cgame_loaded_cb = nullptr;
        cgame_unloading_t on_cgame_unloading_cb = nullptr;

        // must not be noexcept: VM_Create/VM_Free can longjmp via engine Com_Error
        void* __cdecl hk_vm_create(const char* module_name, int a2, int a3, int a4)
        {
            void* const result = vm_create_hook.call<void*>(module_name, a2, a3, a4);
            if (module_name != nullptr && std::strcmp(module_name, "cgame") == 0)
            {
                const HMODULE h_cgame = GetModuleHandleA("cgamex86.dll");
                if (h_cgame != nullptr)
                {
                    active_cgame = h_cgame;
                    if (on_cgame_loaded_cb != nullptr)
                    {
                        on_cgame_loaded_cb(h_cgame);
                    }
                }
            }
            return result;
        }

        int __cdecl hk_vm_free(void* vm)
        {
            // vm_t holds its loaded DLL handle at +0x40; ignore ui/qagame VM teardowns
            if (vm != nullptr)
            {
                const auto vm_dll = *reinterpret_cast<const HMODULE*>(reinterpret_cast<uintptr_t>(vm) + vm_off::dll_handle);
                if (vm_dll != nullptr && vm_dll == active_cgame)
                {
                    if (on_cgame_unloading_cb != nullptr)
                    {
                        on_cgame_unloading_cb();
                    }
                    active_cgame = nullptr;
                }
            }
            return vm_free_hook.call<int>(vm);
        }

        const std::array<util::inline_hook_spec, 2> vm_inline_specs{{
            { &vm_create_hook, exe::addr_vm_create, reinterpret_cast<void*>(&hk_vm_create) },
            { &vm_free_hook,   exe::addr_vm_free,   reinterpret_cast<void*>(&hk_vm_free) },
        }};

        bool check_window_focused(HWND hwnd)
        {
            if (hwnd == nullptr)
            {
                return true;
            }
            if (IsWindow(hwnd) == FALSE || IsIconic(hwnd) != FALSE)
            {
                return false;
            }
            const HWND fg = GetForegroundWindow();
            if (fg == nullptr)
            {
                return false;
            }
            if (fg == hwnd || IsChild(hwnd, fg) != FALSE)
            {
                return true;
            }
            const HWND root_hw = GetAncestor(hwnd, GA_ROOT);
            const HWND root_fg = GetAncestor(fg, GA_ROOT);
            return root_hw != nullptr && (root_fg == root_hw || GetWindow(fg, GW_OWNER) == root_hw);
        }
    }

    uintptr_t exe_base()
    {
        static const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        return base;
    }

    bool verify_pe_module(HMODULE mod, uint32_t expected_timestamp, uint32_t expected_image_size)
    {
        if (mod == nullptr)
        {
            return false;
        }
        const auto* base = reinterpret_cast<const uint8_t*>(mod);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE ||
            dos->e_lfanew <= 0 ||
            static_cast<size_t>(dos->e_lfanew) > (0x1000u - sizeof(IMAGE_NT_HEADERS32)))
        {
            return false;
        }
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        return nt->Signature == IMAGE_NT_SIGNATURE &&
               nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
               nt->FileHeader.TimeDateStamp == expected_timestamp &&
               nt->OptionalHeader.SizeOfImage == expected_image_size;
    }

    int read_client_team(HMODULE h_cgame, int client_num, int fallback)
    {
        if (h_cgame == nullptr || client_num < 0 || client_num >= cgame::max_clients)
        {
            return fallback;
        }
        const auto* team0 = cgame_ptr<const uint8_t>(h_cgame, cgame::addr_cgs_clientinfo_team0);
        if (team0 == nullptr)
        {
            return fallback;
        }
        return *reinterpret_cast<const int*>(
            team0 + static_cast<size_t>(client_num) * cgame::clientinfo_stride);
    }

    void query_hwnd_state_cached(
        HWND hwnd,
        bool& out_valid,
        bool& out_minimized,
        bool& out_focused)
    {
        static HWND last_hwnd = nullptr;
        static int64_t last_poll_qpc = 0;
        static int64_t poll_interval_qpc = 0;
        static bool cached_valid = false;
        static bool cached_minimized = false;
        static bool cached_focused = true;

        if (hwnd == nullptr)
        {
            last_hwnd = nullptr;
            cached_valid = false;
            cached_minimized = false;
            cached_focused = true;
            out_valid = false;
            out_minimized = false;
            out_focused = true;
            return;
        }

        if (poll_interval_qpc <= 0)
        {
            LARGE_INTEGER freq{};
            if (QueryPerformanceFrequency(&freq) == FALSE || freq.QuadPart <= 0)
            {
                std::abort();
            }
            poll_interval_qpc = std::max<int64_t>(1, freq.QuadPart / 125);
        }

        LARGE_INTEGER now_qpc{};
        QueryPerformanceCounter(&now_qpc);
        if (hwnd != last_hwnd || (now_qpc.QuadPart - last_poll_qpc) >= poll_interval_qpc)
        {
            last_hwnd = hwnd;
            last_poll_qpc = now_qpc.QuadPart;

            if (IsWindow(hwnd) == FALSE)
            {
                cached_valid = false;
                cached_minimized = false;
                cached_focused = false;
            }
            else
            {
                cached_valid = true;
                RECT rc{};
                if (IsIconic(hwnd) != FALSE ||
                    GetClientRect(hwnd, &rc) == FALSE ||
                    rc.right <= rc.left ||
                    rc.bottom <= rc.top)
                {
                    cached_minimized = true;
                    cached_focused = false;
                }
                else
                {
                    cached_minimized = false;
                    cached_focused = check_window_focused(hwnd);
                }
            }
        }

        out_valid = cached_valid;
        out_minimized = cached_minimized;
        out_focused = cached_focused;
    }

    std::filesystem::path get_module_dir_dynamic(HMODULE mod)
    {
        std::wstring buf(MAX_PATH, L'\0');
        for (;;)
        {
            const DWORD len = GetModuleFileNameW(mod, buf.data(), static_cast<DWORD>(buf.size()));
            if (len == 0)
            {
                return {};
            }
            if (len < buf.size() - 1)
            {
                buf.resize(len);
                return std::filesystem::path(buf).parent_path();
            }
            buf.resize(buf.size() * 2);
        }
    }

    bool install_vm_hooks(cgame_loaded_t on_loaded, cgame_unloading_t on_unloading)
    {
        on_cgame_loaded_cb = on_loaded;
        on_cgame_unloading_cb = on_unloading;
        if (!util::install_inline_table(&exe_addr, vm_inline_specs))
        {
            on_cgame_loaded_cb = nullptr;
            on_cgame_unloading_cb = nullptr;
            return false;
        }
        return true;
    }

    void release_vm_hooks()
    {
        util::release_hook_table(vm_inline_specs);
        active_cgame = nullptr;
        on_cgame_loaded_cb = nullptr;
        on_cgame_unloading_cb = nullptr;
    }

    void attach_existing_cgame()
    {
        if (const HMODULE h_existing = GetModuleHandleA("cgamex86.dll"); h_existing != nullptr)
        {
            active_cgame = h_existing;
            if (on_cgame_loaded_cb != nullptr)
            {
                on_cgame_loaded_cb(h_existing);
            }
        }
    }

    void detach_active_cgame()
    {
        if (on_cgame_unloading_cb != nullptr)
        {
            on_cgame_unloading_cb();
        }
        active_cgame = nullptr;
    }
}
