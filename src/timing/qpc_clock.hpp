#pragma once

#include <cstdint>

namespace qlqpc::timing
{
    void clock_init();
    void clock_init_elapsed_ms(int32_t elapsed_ms);
    void clock_rebase();

    int64_t clock_frequency();
    int64_t clock_now_ticks();

    int clock_milliseconds();
}
