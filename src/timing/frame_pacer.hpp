#pragma once

#include <cstdint>

namespace qlqpc::timing
{
    bool pacer_init();
    void pacer_shutdown();

    void precision_sleep(int msec);
    void pace_frame(int max_fps, bool allow_spin = true);
    bool sync_to_dwm();
    void set_background_mode(bool enabled);
}
