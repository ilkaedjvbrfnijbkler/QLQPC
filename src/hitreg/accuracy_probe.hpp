#pragma once

#include <windows.h>

#include <cstdint>

namespace qlqpc::hitreg
{
    void init();
    void shutdown();
    void reset();

    void set_enabled(bool enabled);

    bool on_cgame_loaded(HMODULE h_cgame);
    void on_cgame_unloading();

    void on_usercmd(int32_t server_time, bool attack_held);
}
