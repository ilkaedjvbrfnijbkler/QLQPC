#include "util/crash_handler.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <iterator>

namespace qlqpc::util
{
    static_assert(sizeof(void*) == 4, "crash_handler expects 32-bit x86");

    namespace
    {
        wchar_t crash_log_path[MAX_PATH + 32]{};
        LPTOP_LEVEL_EXCEPTION_FILTER prev_filter = nullptr;
        std::atomic<bool> installed{false};
        std::atomic_flag writing_flag = ATOMIC_FLAG_INIT;

        bool append_leaf(
            wchar_t* dst,
            DWORD cap,
            DWORD prefix_len,
            const wchar_t* suffix,
            DWORD suffix_len)
        {
            if (dst == nullptr || (prefix_len + suffix_len + 1) > cap)
            {
                return false;
            }
            for (DWORD i = 0; i <= suffix_len; ++i)
            {
                dst[prefix_len + i] = suffix[i];
            }
            return true;
        }

        void init_crash_log_path()
        {
            constexpr wchar_t log_leaf[] = L"qlqpc_crash.log";
            constexpr DWORD log_leaf_len = 15;

            crash_log_path[0] = L'\0';
            wchar_t mod_path[MAX_PATH]{};
            const DWORD len = GetModuleFileNameW(nullptr, mod_path, static_cast<DWORD>(std::size(mod_path)));
            if (len == 0 || len >= std::size(mod_path))
            {
                return;
            }
            DWORD slash = len;
            while (slash > 0 && mod_path[slash - 1] != L'\\' && mod_path[slash - 1] != L'/')
            {
                --slash;
            }
            if (slash == 0 || (slash + log_leaf_len + 1) > std::size(crash_log_path))
            {
                return;
            }
            for (DWORD i = 0; i < slash; ++i)
            {
                crash_log_path[i] = mod_path[i];
            }
            if (!append_leaf(crash_log_path, static_cast<DWORD>(std::size(crash_log_path)), slash, log_leaf, log_leaf_len))
            {
                crash_log_path[0] = L'\0';
            }
        }

        void resolve_fault_module(void* addr, char* out_name, DWORD cap, uintptr_t& out_offset)
        {
            out_offset = reinterpret_cast<uintptr_t>(addr);
            std::snprintf(out_name, cap, "unknown");
            if (addr == nullptr)
            {
                return;
            }

            HMODULE fault_mod = nullptr;
            if (!GetModuleHandleExW(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    static_cast<LPCWSTR>(addr),
                    &fault_mod) ||
                fault_mod == nullptr)
            {
                return;
            }

            out_offset = reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(fault_mod);

            wchar_t wpath[MAX_PATH]{};
            const DWORD len = GetModuleFileNameW(fault_mod, wpath, static_cast<DWORD>(std::size(wpath)));
            if (len == 0 || len >= std::size(wpath))
            {
                return;
            }

            DWORD start = len;
            while (start > 0 && wpath[start - 1] != L'\\' && wpath[start - 1] != L'/')
            {
                --start;
            }
            const int written = WideCharToMultiByte(
                CP_UTF8, 0, wpath + start, static_cast<int>(len - start),
                out_name, static_cast<int>(cap - 1), nullptr, nullptr);
            if (written > 0)
            {
                out_name[written] = '\0';
            }
        }

        LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* ep)
        {
            if (!writing_flag.test_and_set(std::memory_order_acquire))
            {
                if (ep != nullptr && ep->ExceptionRecord != nullptr && crash_log_path[0] != L'\0')
                {
                    const HANDLE file = CreateFileW(
                        crash_log_path,
                        FILE_APPEND_DATA,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        nullptr);
                    if (file != INVALID_HANDLE_VALUE)
                    {
                        SYSTEMTIME st{};
                        GetLocalTime(&st);

                        char mod_name[128]{};
                        uintptr_t mod_offset = 0;
                        resolve_fault_module(
                            ep->ExceptionRecord->ExceptionAddress,
                            mod_name,
                            sizeof(mod_name),
                            mod_offset);

                        char line[512]{};
                        int n = 0;
                        if (ep->ContextRecord != nullptr)
                        {
                            const CONTEXT* ctx = ep->ContextRecord;
                            n = std::snprintf(
                                line, sizeof(line),
                                "%04u-%02u-%02uT%02u:%02u:%02u.%03u pid=%lu tid=%lu code=0x%08lX addr=0x%08lX "
                                "module=%s+0x%lX eip=0x%08lX esp=0x%08lX ebp=0x%08lX eax=0x%08lX ecx=0x%08lX edx=0x%08lX\r\n",
                                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                                static_cast<unsigned long>(GetCurrentProcessId()),
                                static_cast<unsigned long>(GetCurrentThreadId()),
                                static_cast<unsigned long>(ep->ExceptionRecord->ExceptionCode),
                                static_cast<unsigned long>(reinterpret_cast<uintptr_t>(ep->ExceptionRecord->ExceptionAddress)),
                                mod_name,
                                static_cast<unsigned long>(mod_offset),
                                static_cast<unsigned long>(ctx->Eip),
                                static_cast<unsigned long>(ctx->Esp),
                                static_cast<unsigned long>(ctx->Ebp),
                                static_cast<unsigned long>(ctx->Eax),
                                static_cast<unsigned long>(ctx->Ecx),
                                static_cast<unsigned long>(ctx->Edx));
                        }
                        else
                        {
                            n = std::snprintf(
                                line, sizeof(line),
                                "%04u-%02u-%02uT%02u:%02u:%02u.%03u pid=%lu tid=%lu code=0x%08lX addr=0x%08lX "
                                "module=%s+0x%lX\r\n",
                                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                                static_cast<unsigned long>(GetCurrentProcessId()),
                                static_cast<unsigned long>(GetCurrentThreadId()),
                                static_cast<unsigned long>(ep->ExceptionRecord->ExceptionCode),
                                static_cast<unsigned long>(reinterpret_cast<uintptr_t>(ep->ExceptionRecord->ExceptionAddress)),
                                mod_name,
                                static_cast<unsigned long>(mod_offset));
                        }

                        if (n > 0)
                        {
                            DWORD bytes_written = 0;
                            const DWORD to_write = (n < static_cast<int>(sizeof(line)))
                                ? static_cast<DWORD>(n)
                                : static_cast<DWORD>(sizeof(line) - 1);
                            WriteFile(file, line, to_write, &bytes_written, nullptr);
                        }
                        CloseHandle(file);
                    }
                }
                writing_flag.clear(std::memory_order_release);
            }

            if (prev_filter != nullptr && prev_filter != &unhandled_exception_filter)
            {
                return prev_filter(ep);
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    void install_crash_handler()
    {
        if (!installed.exchange(true, std::memory_order_acq_rel))
        {
            init_crash_log_path();
            prev_filter = SetUnhandledExceptionFilter(&unhandled_exception_filter);
        }
    }
}
