#include "timing/qpc_clock.hpp"

#include <windows.h>

#include <cstdlib>

namespace qlqpc::timing
{
    namespace
    {
        int64_t frequency_hz = 0;
        int64_t base_ticks = 0;
        bool clock_ready = false;
    }

    void clock_init()
    {
        clock_init_elapsed_ms(0);
    }

    void clock_init_elapsed_ms(int32_t elapsed_ms)
    {
        if (clock_ready)
        {
            return;
        }

        LARGE_INTEGER freq{};
        if (QueryPerformanceFrequency(&freq) == FALSE || freq.QuadPart <= 0)
        {
            std::abort();
        }
        frequency_hz = freq.QuadPart;

        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        const int64_t backdate_ticks =
            (elapsed_ms > 0)
                ? ((static_cast<int64_t>(elapsed_ms) * frequency_hz) / 1000)
                : 0;
        base_ticks = now.QuadPart - backdate_ticks;

        clock_ready = true;
    }

    void clock_rebase()
    {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        base_ticks = now.QuadPart;
    }

    int64_t clock_frequency()
    {
        if (!clock_ready)
        {
            clock_init();
        }
        return frequency_hz;
    }

    int64_t clock_now_ticks()
    {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        return now.QuadPart;
    }

    int clock_milliseconds()
    {
        if (!clock_ready)
        {
            clock_init();
        }
        const int64_t elapsed = clock_now_ticks() - base_ticks;
        return static_cast<int>((elapsed * 1000) / frequency_hz);
    }
}
