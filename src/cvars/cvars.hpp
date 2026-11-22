#pragma once

#include <cstdint>

namespace qlqpc::cvars
{
    struct runtime_settings
    {
        bool force_white_names{false};
        bool hide_other_weapons{false};
        int team_lightning_style{1};
        int enemy_lightning_style{1};
        uint8_t team_lightning_opacity_u8{255u};
        uint8_t enemy_lightning_opacity_u8{255u};
    };

    const runtime_settings& settings();
    void reset_for_rearm();

    bool install_hooks();
    void release_hooks();

    void register_qlqpc_cvars();
    void clear_qlqpc_user_created_flags();
    void sync_runtime_from_engine_cvars(bool announce_restart_cvars);
    void ensure_main_thread_cvars();
    void sync_controls_on_main_thread();
    void relax_engine_cvar_bounds();

    bool is_decoupling_active();
    int read_cmdrate();
    int read_maxfps();
    bool read_vulkan_enabled();
}
