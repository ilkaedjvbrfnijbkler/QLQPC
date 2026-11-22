#pragma once

#include <windows.h>

namespace qlqpc::timing
{
    bool install_engine_hooks();
    void release_engine_hooks();
    bool attach_cgame(HMODULE h_cgame);
    void detach_cgame();

    void reset_all();
    void reset_session();
    bool is_ephemeral_pmove();
}
