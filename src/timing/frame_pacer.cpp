#include "timing/frame_pacer.hpp"

#include "timing/qpc_clock.hpp"

#include <windows.h>

#include <cstdlib>
#include <immintrin.h>

namespace qlqpc::timing
{
    namespace
    {
        using d3dkmt_get_proc_prio_t = LONG(__stdcall*)(HANDLE, int*);
        using d3dkmt_set_proc_prio_t = LONG(__stdcall*)(HANDLE, int);
        using dwm_flush_t = HRESULT(__stdcall*)();

        HANDLE wait_timer = nullptr;
        int64_t last_frame_ticks = 0;
        bool pacer_ready = false;

        d3dkmt_get_proc_prio_t d3dkmt_get_prio = nullptr;
        d3dkmt_set_proc_prio_t d3dkmt_set_prio = nullptr;
        bool d3dkmt_probed = false;

        dwm_flush_t dwm_flush = nullptr;
        bool dwm_probed = false;

        bool bg_active = false;
        bool gpu_prio_changed = false;
        int saved_gpu_prio = 2; // D3DKMT_SCHEDULINGPRIORITYCLASS_NORMAL
        bool thread_prio_changed = false;
        DWORD bg_thread_id = 0;
        int saved_thread_prio = THREAD_PRIORITY_NORMAL;

        void resolve_d3dkmt()
        {
            if (d3dkmt_probed)
            {
                return;
            }
            d3dkmt_probed = true;
            // intentionally avoid FreeLibrary so shutdown stays safe under the OS loader lock
            HMODULE h_gdi = GetModuleHandleW(L"gdi32.dll");
            if (h_gdi == nullptr)
            {
                h_gdi = LoadLibraryW(L"gdi32.dll");
            }
            if (h_gdi != nullptr)
            {
                d3dkmt_get_prio = reinterpret_cast<d3dkmt_get_proc_prio_t>(
                    GetProcAddress(h_gdi, "D3DKMTGetProcessSchedulingPriorityClass"));
                d3dkmt_set_prio = reinterpret_cast<d3dkmt_set_proc_prio_t>(
                    GetProcAddress(h_gdi, "D3DKMTSetProcessSchedulingPriorityClass"));
            }
        }

        bool resolve_dwm_flush()
        {
            if (!dwm_probed)
            {
                dwm_probed = true;
                HMODULE h_dwm = GetModuleHandleW(L"dwmapi.dll");
                if (h_dwm == nullptr)
                {
                    h_dwm = LoadLibraryW(L"dwmapi.dll");
                }
                if (h_dwm != nullptr)
                {
                    dwm_flush = reinterpret_cast<dwm_flush_t>(
                        GetProcAddress(h_dwm, "DwmFlush"));
                }
            }
            return dwm_flush != nullptr;
        }
    }

    bool pacer_init()
    {
        if (pacer_ready)
        {
            return true;
        }

        clock_init();

        constexpr DWORD create_waitable_timer_high_resolution = 0x00000002;
        wait_timer = CreateWaitableTimerExW(
            nullptr,
            nullptr,
            create_waitable_timer_high_resolution | CREATE_WAITABLE_TIMER_MANUAL_RESET,
            TIMER_ALL_ACCESS);

        if (wait_timer == nullptr)
        {
            return false;
        }

        last_frame_ticks = clock_now_ticks();
        pacer_ready = true;
        return true;
    }

    void pacer_shutdown()
    {
        set_background_mode(false);
        if (wait_timer != nullptr)
        {
            CloseHandle(wait_timer);
            wait_timer = nullptr;
        }
        pacer_ready = false;
        last_frame_ticks = 0;
    }

    namespace
    {
        void precision_sleep_until(int64_t target_ticks, bool allow_spin = true)
        {
            if (!pacer_ready && !pacer_init())
            {
                std::abort();
            }

            const int64_t freq = clock_frequency();
            const int64_t half_ms_ticks = freq / 2000;
            const int64_t sleep_threshold_ticks = (freq * 3) / 2000;

            const int64_t now = clock_now_ticks();
            const int64_t remaining = target_ticks - now;

            if (remaining <= 0)
            {
                return;
            }

            if (!allow_spin)
            {
                const int64_t wait_100ns = (remaining * 10000000LL) / freq;
                LARGE_INTEGER due_time{};
                due_time.QuadPart = (wait_100ns > 1) ? -wait_100ns : -1LL;
                if (SetWaitableTimer(wait_timer, &due_time, 0, nullptr, nullptr, FALSE) == FALSE)
                {
                    std::abort();
                }
                WaitForSingleObject(wait_timer, INFINITE);
                return;
            }

            if (remaining > sleep_threshold_ticks)
            {
                const int64_t sleep_ticks = remaining - half_ms_ticks;
                LARGE_INTEGER due_time{};
                due_time.QuadPart = -((sleep_ticks * 10000000LL) / freq);
                if (SetWaitableTimer(wait_timer, &due_time, 0, nullptr, nullptr, FALSE) == FALSE)
                {
                    std::abort();
                }
                WaitForSingleObject(wait_timer, INFINITE);
            }

            while (clock_now_ticks() < target_ticks)
            {
                _mm_pause();
            }
        }
    }

    void precision_sleep(int msec)
    {
        if (msec <= 0)
        {
            return;
        }

        const int64_t freq = clock_frequency();
        const int64_t target_ticks = clock_now_ticks() + (static_cast<int64_t>(msec) * freq) / 1000;
        precision_sleep_until(target_ticks);
    }

    void pace_frame(int max_fps, bool allow_spin)
    {
        if (max_fps <= 0)
        {
            return;
        }

        if (!pacer_ready)
        {
            pacer_init();
        }

        const int64_t freq = clock_frequency();
        const int64_t frame_interval_ticks = freq / max_fps;
        const int64_t now = clock_now_ticks();

        if (last_frame_ticks == 0 || (now - last_frame_ticks) > (frame_interval_ticks * 4))
        {
            last_frame_ticks = now;
            return;
        }

        const int64_t target_ticks = last_frame_ticks + frame_interval_ticks;
        precision_sleep_until(target_ticks, allow_spin);
        if (allow_spin)
        {
            last_frame_ticks = target_ticks;
        }
        else
        {
            // clamp to actual wakeup time so OS timer slack cannot cause catch-up bursts
            const int64_t now_after = clock_now_ticks();
            last_frame_ticks = (now_after > target_ticks) ? now_after : target_ticks;
        }
    }

    bool sync_to_dwm()
    {
        if (!resolve_dwm_flush())
        {
            return false;
        }
        const HRESULT hr = dwm_flush();
        return SUCCEEDED(hr);
    }

    void set_background_mode(bool enabled)
    {
        if (enabled == bg_active)
        {
            return;
        }

        const HANDLE proc = GetCurrentProcess();
        if (enabled)
        {
            resolve_d3dkmt();
            gpu_prio_changed = false;
            saved_gpu_prio = 2; // D3DKMT_SCHEDULINGPRIORITYCLASS_NORMAL
            if (d3dkmt_set_prio != nullptr)
            {
                int queried_prio = 2;
                if (d3dkmt_get_prio != nullptr && d3dkmt_get_prio(proc, &queried_prio) == 0)
                {
                    saved_gpu_prio = queried_prio;
                }
                if (saved_gpu_prio > 0)
                {
                    LONG st = d3dkmt_set_prio(proc, 0); // D3DKMT_SCHEDULINGPRIORITYCLASS_IDLE
                    if (st != 0 && saved_gpu_prio > 1)
                    {
                        st = d3dkmt_set_prio(proc, 1); // D3DKMT_SCHEDULINGPRIORITYCLASS_BELOW_NORMAL
                    }
                    gpu_prio_changed = (st == 0);
                }
            }

            thread_prio_changed = false;
            bg_thread_id = GetCurrentThreadId();
            const int cur_thread_prio = GetThreadPriority(GetCurrentThread());
            if (cur_thread_prio != THREAD_PRIORITY_ERROR_RETURN &&
                cur_thread_prio > THREAD_PRIORITY_BELOW_NORMAL)
            {
                saved_thread_prio = cur_thread_prio;
                if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL) != FALSE)
                {
                    thread_prio_changed = true;
                }
            }
            bg_active = true;
            return;
        }

        if (gpu_prio_changed && d3dkmt_set_prio != nullptr)
        {
            d3dkmt_set_prio(proc, saved_gpu_prio);
        }
        gpu_prio_changed = false;

        if (thread_prio_changed && bg_thread_id != 0)
        {
            if (GetCurrentThreadId() == bg_thread_id)
            {
                SetThreadPriority(GetCurrentThread(), saved_thread_prio);
            }
            else
            {
                const HANDLE h_thread = OpenThread(THREAD_SET_INFORMATION, FALSE, bg_thread_id);
                if (h_thread != nullptr)
                {
                    SetThreadPriority(h_thread, saved_thread_prio);
                    CloseHandle(h_thread);
                }
            }
        }
        thread_prio_changed = false;
        bg_thread_id = 0;
        bg_active = false;
    }
}
