#include "cvars/cvars.hpp"
#include "game/game.hpp"
#include "hitreg/accuracy_probe.hpp"
#include "timing/frame_pacer.hpp"
#include "timing/qpc_clock.hpp"
#include "timing/timing_hooks.hpp"
#include "util/crash_handler.hpp"
#include "util/hook_engine.hpp"
#include "visuals/visuals.hpp"
#include "vulkan/vulkan_backend.hpp"
#include "webui/webui_hooks.hpp"

#include <windows.h>
#include <mmsystem.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

namespace qlqpc
{
    namespace
    {
        util::inline_hook winmain_hook{};
        util::inline_hook hook_qgl_init{};
        util::inline_hook hook_qgl_shutdown{};
        std::atomic<bool> bootstrapped{false};

        bool armed = false;

        void fatal_error(const wchar_t* msg)
        {
            MessageBoxW(nullptr, msg, L"QLQPC", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
            ExitProcess(1);
        }

        bool is_target_process()
        {
            wchar_t buf[MAX_PATH]{};
            DWORD len = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(std::size(buf)));
            std::wstring heap_buf;
            std::wstring_view full_path;
            if (len > 0 && len < std::size(buf))
            {
                full_path = std::wstring_view(buf, len);
            }
            else if (len >= std::size(buf))
            {
                heap_buf.resize(MAX_PATH * 2u, L'\0');
                for (;;)
                {
                    len = GetModuleFileNameW(nullptr, heap_buf.data(), static_cast<DWORD>(heap_buf.size()));
                    if (len == 0)
                    {
                        return false;
                    }
                    if (len < heap_buf.size() - 1)
                    {
                        heap_buf.resize(len);
                        full_path = heap_buf;
                        break;
                    }
                    heap_buf.resize(heap_buf.size() * 2u);
                }
            }
            else
            {
                return false;
            }

            const auto pos = full_path.find_last_of(L"\\/");
            const std::wstring_view filename = (pos == std::wstring_view::npos) ? full_path : full_path.substr(pos + 1);
            return filename.size() == 19 && (_wcsicmp(filename.data(), L"quakelive_steam.exe") == 0);
        }

        void on_cgame_loaded(HMODULE h_cgame)
        {
            if (!game::verify_pe_module(
                    h_cgame,
                    game::cgame::expected_timestamp,
                    game::cgame::expected_image_size))
            {
                fatal_error(L"QLQPC detected an unsupported cgamex86.dll build.");
            }
            if (!timing::attach_cgame(h_cgame) ||
                !visuals::attach_cgame(h_cgame) ||
                !hitreg::on_cgame_loaded(h_cgame))
            {
                fatal_error(L"QLQPC failed to attach cgamex86.dll hooks.");
            }
        }

        void on_cgame_unloading()
        {
            hitreg::on_cgame_unloading();
            hitreg::reset();
            visuals::detach_cgame();
            timing::detach_cgame();
        }

        int __cdecl hk_qgl_init(const char* dllname)
        {
            cvars::register_qlqpc_cvars();
            cvars::clear_qlqpc_user_created_flags();

            const bool want_vulkan = cvars::read_vulkan_enabled();
            cvars::sync_runtime_from_engine_cvars(false);

            if (want_vulkan)
            {
                const auto* existing_hinst_gl = game::exe_ptr<void* const>(game::exe::addr_hinst_opengl);
                if (existing_hinst_gl == nullptr || *existing_hinst_gl != nullptr)
                {
                    fatal_error(L"QLQPC cannot initialize Vulkan because OpenGL is already loaded.");
                }
                if (!vulkan::init_qgl_vulkan(game::exe_base()))
                {
                    fatal_error(L"QLQPC failed to initialize the Vulkan backend.");
                }
                return 1;
            }

            vulkan::destroy_vulkan_device();
            return hook_qgl_init.call<int>(dllname);
        }

        int __cdecl hk_qgl_shutdown()
        {
            if (vulkan::is_vulkan_backend_active())
            {
                vulkan::shutdown_qgl_vulkan(game::exe_base());
                if (!cvars::read_vulkan_enabled())
                {
                    vulkan::destroy_vulkan_device();
                }
                return 0;
            }
            return hook_qgl_shutdown.call<int>();
        }

        void release_all_hooks()
        {
            hitreg::shutdown();
            game::detach_active_cgame();

            webui::release_hooks();
            vulkan::destroy_vulkan_device();
            hook_qgl_shutdown.reset();
            hook_qgl_init.reset();
            visuals::release_engine_hooks();
            game::release_vm_hooks();
            timing::release_engine_hooks();
            cvars::release_hooks();

            timing::pacer_shutdown();
        }

        bool install_all()
        {
            if (armed)
            {
                return true;
            }

            int32_t initial_elapsed_ms = 0;
            const auto* p_sys_time_base = game::exe_ptr<const uint32_t>(game::exe::addr_sys_time_base);
            if (p_sys_time_base != nullptr && *p_sys_time_base != 0u)
            {
                const uint32_t diff = timeGetTime() - *p_sys_time_base;
                if (diff < 0x7FFFFFFFu)
                {
                    initial_elapsed_ms = static_cast<int32_t>(diff);
                }
            }
            timing::clock_init_elapsed_ms(initial_elapsed_ms);

            if (!timing::pacer_init())
            {
                return false;
            }
            timing::reset_all();
            visuals::reset_session_state();
            hitreg::init();
            hitreg::set_enabled(false);

            cvars::reset_for_rearm();

            if (!cvars::install_hooks() ||
                !timing::install_engine_hooks() ||
                !game::install_vm_hooks(&on_cgame_loaded, &on_cgame_unloading))
            {
                release_all_hooks();
                return false;
            }

            game::attach_existing_cgame();

            if (!visuals::install_engine_hooks())
            {
                release_all_hooks();
                return false;
            }

            auto qgl_i = util::create_inline(
                reinterpret_cast<void*>(game::exe_addr(game::exe::addr_qgl_init)),
                reinterpret_cast<void*>(&hk_qgl_init));
            if (!qgl_i)
            {
                release_all_hooks();
                return false;
            }
            hook_qgl_init = std::move(qgl_i);

            auto qgl_s = util::create_inline(
                reinterpret_cast<void*>(game::exe_addr(game::exe::addr_qgl_shutdown)),
                reinterpret_cast<void*>(&hk_qgl_shutdown));
            if (!qgl_s)
            {
                release_all_hooks();
                return false;
            }
            hook_qgl_shutdown = std::move(qgl_s);

            if (!webui::install_hooks())
            {
                release_all_hooks();
                return false;
            }

            cvars::relax_engine_cvar_bounds();

            armed = true;
            return true;
        }

        void uninstall_all(bool process_terminating)
        {
            if (!armed)
            {
                return;
            }
            if (process_terminating)
            {
                // skip unpatching potentially unmapped DLLs and static destructor cleanup under loader lock
                util::set_process_terminating_hooks();
            }
            else
            {
                release_all_hooks();
            }
            armed = false;
        }

        int WINAPI hk_win_main(HINSTANCE h_instance, HINSTANCE h_prev_instance, LPSTR lp_cmd_line, int n_show_cmd)
        {
            if (!bootstrapped.exchange(true, std::memory_order_acq_rel))
            {
                util::install_crash_handler();
                if (!install_all())
                {
                    fatal_error(L"QLQPC failed to install engine hooks.");
                }
            }
            return winmain_hook.stdcall<int>(h_instance, h_prev_instance, lp_cmd_line, n_show_cmd);
        }
    }
}

BOOL APIENTRY DllMain(HMODULE h_module, DWORD ul_reason_for_call, LPVOID lp_reserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(h_module);

        if (qlqpc::is_target_process())
        {
            if (!qlqpc::game::verify_pe_module(
                    GetModuleHandleW(nullptr),
                    qlqpc::game::exe::expected_timestamp,
                    qlqpc::game::exe::expected_image_size))
            {
                return FALSE;
            }
            const uintptr_t win_main_addr =
                qlqpc::game::exe_addr(qlqpc::game::exe::addr_win_main);
            qlqpc::winmain_hook = qlqpc::util::create_inline(
                reinterpret_cast<void*>(win_main_addr),
                reinterpret_cast<void*>(&qlqpc::hk_win_main));
            if (!qlqpc::winmain_hook)
            {
                return FALSE;
            }
        }
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        const bool process_terminating = (lp_reserved != nullptr);
        qlqpc::uninstall_all(process_terminating);
        if (process_terminating)
        {
            qlqpc::util::set_process_terminating_hooks();
        }
        else
        {
            qlqpc::winmain_hook.reset();
        }
        break;
    }
    default:
        break;
    }
    return TRUE;
}
