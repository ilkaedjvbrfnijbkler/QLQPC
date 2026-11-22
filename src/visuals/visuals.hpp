#pragma once

#include <windows.h>

namespace qlqpc::visuals
{
    bool install_engine_hooks();
    void release_engine_hooks();
    bool attach_cgame(HMODULE h_cgame);
    void detach_cgame();

    void update_fog(
        bool enabled,
        int r,
        int g,
        int b,
        float distance,
        float intensity_scale,
        bool override_native);

    void reset_session_state();
}
