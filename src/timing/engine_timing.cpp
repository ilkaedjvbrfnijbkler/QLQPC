#include "timing/timing_hooks.hpp"
#include "timing/timing_internal.hpp"

#include "cvars/cvars.hpp"
#include "game/game.hpp"
#include "hitreg/accuracy_probe.hpp"
#include "timing/frame_pacer.hpp"
#include "timing/qpc_clock.hpp"
#include "util/hook_engine.hpp"
#include "visuals/visuals.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace qlqpc::timing::detail
{
    qpc_fps_estimator fps_estimator{};
    double cur_sub_ms_frac = 0.0;
    double cur_exact_render_ms = 0.0;
}

namespace qlqpc::timing
{
    namespace
    {
        util::inline_hook hook_sys_milliseconds{};
        util::inline_hook hook_sys_init_time{};
        util::inline_hook hook_sys_sleep{};
        util::inline_hook hook_sys_que_event{};
        util::inline_hook hook_com_modify_msec{};
        util::inline_hook hook_cl_frame{};
        util::inline_hook hook_cl_send_command{};
        util::inline_hook hook_cl_create_new_commands{};
        util::inline_hook hook_cl_set_cgame_time{};
        util::inline_hook hook_cl_adjust_time_delta{};
        util::inline_hook hook_cl_parse_gamestate{};

        util::mid_hook hook_sys_get_event_msg{};
        util::mid_hook hook_sys_get_event_empty{};
        util::mid_hook hook_com_frame_pacer{};
        util::mid_hook hook_cl_mouse_rate{};

        tick_msec_accumulator msec_acc{};
        monotonic_render_clock render_clock{};
        command_cadence_pacer cmd_pacer{};
        mouse_rate_timer rate_timer{};
        snapshot_clock_sync snap_sync{};
        loopback_packet_limiter loopback_limiter{};

        double cur_mouse_dt_ms = 1.0;
        bool has_unsent_cmd = false;
        bool was_non_standard_clock = false;

        int __cdecl hk_sys_milliseconds()
        {
            return clock_milliseconds();
        }

        void __cdecl hk_sys_init_time()
        {
            clock_rebase();
            msec_acc.reset();
            detail::cur_sub_ms_frac = 0.0;
            detail::fps_estimator.reset();
            reset_session();
            visuals::reset_session_state();
            hitreg::reset();
            hook_sys_init_time.call<void>();
        }

        int __cdecl hk_sys_sleep(int msec)
        {
            precision_sleep(msec);
            return 1;
        }

        void __cdecl hk_sys_que_event(int time, int type, int value, int value2, int ptr_length, void* ptr)
        {
            if (time == 0)
            {
                time = clock_milliseconds();
            }
            hook_sys_que_event.call<void>(time, type, value, value2, ptr_length, ptr);
        }

        void on_get_event_msg_time(util::mid_context& ctx)
        {
            ctx.ecx = static_cast<uintptr_t>(clock_milliseconds());
        }

        void on_get_event_empty(util::mid_context& ctx)
        {
            ctx.eax = static_cast<uintptr_t>(clock_milliseconds());
        }

        int __cdecl hk_com_modify_msec(int msec)
        {
            const int64_t now_ticks = clock_now_ticks();
            const int64_t freq = clock_frequency();

            const int whole_ms = msec_acc.step(now_ticks, freq);
            if (!cvars::is_decoupling_active())
            {
                detail::cur_sub_ms_frac = 0.0;
                if (msec < 1)
                {
                    return 0;
                }
                return hook_com_modify_msec.call<int>(msec);
            }

            detail::cur_sub_ms_frac = msec_acc.sub_ms_fraction(freq);

            if (whole_ms < 1)
            {
                return 0;
            }

            return hook_com_modify_msec.call<int>(whole_ms);
        }

        void on_com_frame_pacer(util::mid_context& ctx)
        {
            const int max_fps = cvars::read_maxfps();

            bool allow_spin = true;
            bool is_valid = false;
            bool is_minimized = false;
            bool is_focused = true;
            static const auto* p_hwnd = game::exe_ptr<const HWND>(game::exe::addr_hwnd_parent);
            const HWND hwnd = (p_hwnd != nullptr) ? *p_hwnd : nullptr;
            if (hwnd != nullptr)
            {
                game::query_hwnd_state_cached(hwnd, is_valid, is_minimized, is_focused);
                allow_spin = is_valid && !is_minimized && is_focused;
            }

            const bool background = !allow_spin;
            set_background_mode(background);

            if (background)
            {
                if (!sync_to_dwm())
                {
                    const int bg_fps = (max_fps > 0) ? std::min(60, max_fps) : 60;
                    pace_frame(bg_fps, false);
                }
                else if (max_fps > 0)
                {
                    pace_frame(max_fps, false);
                }
            }
            else if (max_fps > 0)
            {
                pace_frame(max_fps, allow_spin);
            }

            // zero Com_Frame's local wait target so it skips the coarse Sleep loop
            *reinterpret_cast<int*>(ctx.ebp - 4) = 0;
        }

        void on_cl_mouse_move_rate(util::mid_context& ctx)
        {
            if (!cvars::is_decoupling_active())
            {
                return;
            }
            // CL_MouseMove locals: mx at ebp-0x10, my at ebp-0x14, rate at ebp-0x04
            const float mx = *reinterpret_cast<const float*>(ctx.ebp - 0x10);
            const float my = *reinterpret_cast<const float*>(ctx.ebp - 0x14);
            const double dist = std::sqrt(static_cast<double>(mx) * static_cast<double>(mx)
                                        + static_cast<double>(my) * static_cast<double>(my));
            const double dt_ms = std::max(0.05, cur_mouse_dt_ms);
            *reinterpret_cast<float*>(ctx.ebp - 0x04) = static_cast<float>(dist / dt_ms);
        }

        void* __cdecl hk_cl_frame(int msec)
        {
            cvars::ensure_main_thread_cvars();

            static const auto* p_cls_state = game::exe_ptr<const int>(game::exe::addr_cls_state);
            const int state = (p_cls_state != nullptr) ? *p_cls_state : 0;

            cvars::sync_controls_on_main_thread();

            if (p_cls_state != nullptr)
            {
                if (state < 7)
                {
                    reset_session();
                    visuals::reset_session_state();
                }
                else if (state == 8)
                {
                    // flush raw input queued during frame pacing before CL_SendCommand and rendering
                    static const auto fn_in_frame = game::exe_fn<game::in_frame_t>(game::exe::addr_in_frame);
                    static const auto fn_event_loop = game::exe_fn<game::com_event_loop_t>(game::exe::addr_com_event_loop);
                    if (fn_in_frame != nullptr && fn_event_loop != nullptr)
                    {
                        fn_in_frame();
                        fn_event_loop();
                    }
                }
            }

            return hook_cl_frame.call<void*>(msec);
        }

        int __cdecl hk_cl_parse_gamestate(void* msg)
        {
            reset_session();
            visuals::reset_session_state();

            const int result = hook_cl_parse_gamestate.call<int>(msg);

            msec_acc.reanchor(clock_now_ticks());
            return result;
        }

        int read_cl_maxpackets()
        {
            static const auto* p_cvar_maxpackets = game::exe_ptr<void* const>(game::exe::addr_cvar_maxpackets_ptr);
            if (p_cvar_maxpackets != nullptr && *p_cvar_maxpackets != nullptr)
            {
                return game::read_cvar_int(*p_cvar_maxpackets, 125);
            }
            return 125;
        }

        bool is_local_or_lan_netchan()
        {
            static const auto* p_netchan_type = game::exe_ptr<const int>(game::exe::addr_clc_netchan_type);
            static const auto* p_netchan_ip = game::exe_ptr<const int>(game::exe::addr_clc_netchan_ip);
            static const auto fn_is_lan = game::exe_fn<int(__cdecl*)(int, int)>(game::exe::addr_sys_is_lan_address);
            if (p_netchan_type == nullptr || p_netchan_ip == nullptr)
            {
                return false;
            }
            if (*p_netchan_type == 2)
            {
                return true;
            }
            return (fn_is_lan != nullptr) && (fn_is_lan(*p_netchan_type, *p_netchan_ip) != 0);
        }

        void __cdecl hk_cl_send_command()
        {
            static const auto* p_cls_state = game::exe_ptr<const int>(game::exe::addr_cls_state);
            const int cls_state = (p_cls_state != nullptr) ? *p_cls_state : 0;
            if (p_cls_state == nullptr || cls_state < 8)
            {
                has_unsent_cmd = false;
                cmd_pacer.reset();
                if (cls_state == 7 && is_local_or_lan_netchan())
                {
                    if (!loopback_limiter.should_send(
                            clock_now_ticks(), clock_frequency(), read_cl_maxpackets()))
                    {
                        return;
                    }
                }
                hook_cl_send_command.call<void>();
                return;
            }

            static const auto* p_sv_running = game::exe_ptr<void* const>(game::exe::addr_cvar_sv_running_ptr);
            static const auto* p_cl_paused = game::exe_ptr<void* const>(game::exe::addr_cvar_cl_paused_ptr);
            static const auto* p_sv_paused = game::exe_ptr<void* const>(game::exe::addr_cvar_sv_paused_ptr);
            if (p_sv_running != nullptr && p_cl_paused != nullptr && p_sv_paused != nullptr)
            {
                if (game::read_cvar_int(*p_sv_running) != 0 &&
                    game::read_cvar_int(*p_cl_paused) != 0 &&
                    game::read_cvar_int(*p_sv_paused) != 0)
                {
                    return;
                }
            }

            const int64_t now_ticks = clock_now_ticks();
            const int64_t freq = clock_frequency();

            static const auto* p_mouse_dx = game::exe_ptr<const int>(game::exe::addr_cl_mouse_dx);
            static const auto* p_mouse_dy = game::exe_ptr<const int>(game::exe::addr_cl_mouse_dy);
            const bool has_mouse_counts =
                (p_mouse_dx != nullptr && *p_mouse_dx != 0) ||
                (p_mouse_dy != nullptr && *p_mouse_dy != 0);

            if (!cvars::is_decoupling_active())
            {
                cmd_pacer.reset();
                rate_timer.reset();
                has_unsent_cmd = false;

                // guard against cl.frame_msec == 0 NaN in CL_KeyState when com_frameTime == cl.oldRealtime
                static const auto* p_com_frametime = game::exe_ptr<const int>(game::exe::addr_com_frametime);
                static const auto* p_old_realtime = game::exe_ptr<const int>(game::exe::addr_old_realtime);
                if (p_com_frametime != nullptr && p_old_realtime != nullptr)
                {
                    const int delta_ms = *p_com_frametime - *p_old_realtime;
                    if (delta_ms < 1 && delta_ms > -500)
                    {
                        return;
                    }
                }

                if (is_local_or_lan_netchan() &&
                    !loopback_limiter.should_send(now_ticks, freq, read_cl_maxpackets()))
                {
                    static const auto fn_create_new_cmds = game::exe_fn<game::void_cmd_t>(game::exe::addr_cl_create_new_commands);
                    if (fn_create_new_cmds != nullptr)
                    {
                        fn_create_new_cmds();
                    }
                    return;
                }

                hook_cl_send_command.call<void>();
                return;
            }

            cur_mouse_dt_ms = rate_timer.on_mouse_sample(now_ticks, freq, has_mouse_counts);

            const int requested_rate = cvars::read_cmdrate();
            const bool is_cmd_tick = cmd_pacer.should_emit_command(now_ticks, freq, requested_rate);

            if (is_cmd_tick)
            {
                static const auto fn_create_new_cmds = game::exe_fn<game::void_cmd_t>(game::exe::addr_cl_create_new_commands);
                if (fn_create_new_cmds != nullptr)
                {
                    fn_create_new_cmds();
                    has_unsent_cmd = true;
                }
            }
            else
            {
                // subframe aim update via stack dummy usercmd so viewangles remain crisp between ticks
                static const auto* p_m_filter = game::exe_ptr<void* const>(game::exe::addr_cvar_m_filter_ptr);
                const bool filter_active = (p_m_filter != nullptr && game::read_cvar_int(*p_m_filter) != 0);

                if (has_mouse_counts || filter_active)
                {
                    static auto* const p_frame_msec = game::exe_ptr<int>(game::exe::addr_frame_msec);
                    if (p_frame_msec != nullptr && *p_frame_msec < 1)
                    {
                        *p_frame_msec = 1;
                    }

                    static const auto fn_mouse_move = game::exe_fn<game::cl_mouse_move_t>(game::exe::addr_cl_mouse_move_fn);
                    if (fn_mouse_move != nullptr)
                    {
                        alignas(4) uint8_t dummy_cmd[game::usercmd_bytes]{};
                        fn_mouse_move(dummy_cmd);
                    }
                }
            }

            if (has_unsent_cmd)
            {
                static const auto fn_ready = game::exe_fn<game::cl_ready_to_send_t>(game::exe::addr_cl_ready_to_send);
                static const auto fn_write = game::exe_fn<game::cl_write_packet_t>(game::exe::addr_cl_write_packet);
                if (fn_ready != nullptr && fn_write != nullptr)
                {
                    bool ready = (fn_ready() != 0);
                    if (ready && is_local_or_lan_netchan())
                    {
                        ready = loopback_limiter.should_send(now_ticks, freq, read_cl_maxpackets());
                    }
                    if (ready)
                    {
                        fn_write();
                        has_unsent_cmd = false;
                    }
                    else if (is_cmd_tick)
                    {
                        static const auto* p_show_send = game::exe_ptr<void* const>(game::exe::addr_cvar_showsend_ptr);
                        if (p_show_send != nullptr && game::read_cvar_int(*p_show_send) != 0)
                        {
                            static const auto fn_printf = game::exe_fn<game::com_printf_t>(game::exe::addr_com_printf);
                            if (fn_printf != nullptr)
                            {
                                fn_printf(". ");
                            }
                        }
                    }
                }
            }
        }

        void __cdecl hk_cl_create_new_commands()
        {
            static const auto* p_com_frametime = game::exe_ptr<const int>(game::exe::addr_com_frametime);
            static auto* const p_old_realtime = game::exe_ptr<int>(game::exe::addr_old_realtime);
            static auto* const p_frame_msec = game::exe_ptr<int>(game::exe::addr_frame_msec);
            static auto* const p_real_frametime = game::exe_ptr<int>(game::exe::addr_cls_real_frametime);
            static const auto* p_cmd_number = game::exe_ptr<const int>(game::exe::addr_cmd_number);
            static auto* const p_cmd_ring = game::exe_ptr<uint8_t>(game::exe::addr_cmd_ring);

            if (!cvars::is_decoupling_active())
            {
                if (p_com_frametime != nullptr && p_old_realtime != nullptr && *p_com_frametime <= *p_old_realtime)
                {
                    *p_old_realtime = *p_com_frametime - 1;
                }
                hook_cl_create_new_commands.call<void>();
                if (p_frame_msec != nullptr && *p_frame_msec < 1)
                {
                    *p_frame_msec = 1;
                }
                if (p_cmd_number != nullptr && p_cmd_ring != nullptr)
                {
                    const int cmd_num = *p_cmd_number;
                    const uint8_t* cmd = p_cmd_ring + ((cmd_num & 63) * game::usercmd_bytes);
                    const int32_t server_time = *reinterpret_cast<const int32_t*>(cmd);
                    const int32_t buttons = *reinterpret_cast<const int32_t*>(cmd + 16);
                    hitreg::on_usercmd(server_time, (buttons & 1) != 0);
                }
                return;
            }

            int cmd_delta_ms = 4;
            if (p_com_frametime != nullptr && p_old_realtime != nullptr)
            {
                if (*p_com_frametime <= *p_old_realtime)
                {
                    *p_old_realtime = *p_com_frametime - 1;
                }
                cmd_delta_ms = std::clamp(*p_com_frametime - *p_old_realtime, 1, 200);
            }

            int saved_real_frametime = 0;
            if (p_real_frametime != nullptr)
            {
                saved_real_frametime = *p_real_frametime;
                *p_real_frametime = cmd_delta_ms;
            }

            hook_cl_create_new_commands.call<void>();

            if (p_real_frametime != nullptr)
            {
                *p_real_frametime = saved_real_frametime;
            }
            if (p_frame_msec != nullptr && *p_frame_msec < 1)
            {
                *p_frame_msec = 1;
            }

            if (p_cmd_number != nullptr && p_cmd_ring != nullptr)
            {
                const int cmd_num = *p_cmd_number;
                uint8_t* const cmd = p_cmd_ring + ((cmd_num & 63) * game::usercmd_bytes);

                if (cmd_num > 1)
                {
                    const uint8_t* prev_cmd = p_cmd_ring + (((cmd_num - 1) & 63) * game::usercmd_bytes);
                    const int32_t prev_st = *reinterpret_cast<const int32_t*>(prev_cmd);
                    int32_t& cur_st = *reinterpret_cast<int32_t*>(cmd);
                    cur_st = clamp_command_server_time(cur_st, prev_st);
                }

                const int32_t server_time = *reinterpret_cast<const int32_t*>(cmd);
                const int32_t buttons = *reinterpret_cast<const int32_t*>(cmd + 16);
                hitreg::on_usercmd(server_time, (buttons & 1) != 0);
            }
        }

        void* __cdecl hk_cl_set_cgame_time()
        {
            if (!cvars::is_decoupling_active())
            {
                render_clock.reset();
                detail::cur_exact_render_ms = 0.0;
                return hook_cl_set_cgame_time.call<void*>();
            }

            static const auto* p_cls_state = game::exe_ptr<const int>(game::exe::addr_cls_state);
            static const auto* p_demoplaying = game::exe_ptr<const int>(game::exe::addr_clc_demoplaying);
            static const auto* p_cls_frametime = game::exe_ptr<const int>(game::exe::addr_cls_frametime);
            static auto* const p_server_time_delta = game::exe_ptr<int32_t>(game::exe::addr_cl_server_time_delta);

            if (p_cls_state != nullptr && *p_cls_state == 8 &&
                (p_demoplaying == nullptr || *p_demoplaying == 0) &&
                p_cls_frametime != nullptr && p_server_time_delta != nullptr)
            {
                const int32_t absorb = snap_sync.consume_monotonic_pullback(*p_cls_frametime);
                if (absorb > 0)
                {
                    *p_server_time_delta -= absorb;
                }
            }

            void* const res = hook_cl_set_cgame_time.call<void*>();

            static const auto* p_cl_server_time = game::exe_ptr<const int32_t>(game::exe::addr_cl_server_time);
            if (p_cl_server_time != nullptr && p_cls_state != nullptr && *p_cls_state == 8)
            {
                if (p_demoplaying != nullptr && *p_demoplaying != 0)
                {
                    detail::cur_exact_render_ms = static_cast<double>(*p_cl_server_time) + detail::cur_sub_ms_frac;
                }
                else
                {
                    detail::cur_exact_render_ms = render_clock.sample(*p_cl_server_time, detail::cur_sub_ms_frac);
                }
            }

            return res;
        }

        void __cdecl hk_cl_adjust_time_delta()
        {
            if (!cvars::is_decoupling_active())
            {
                snap_sync.reset();
                was_non_standard_clock = false;
                hook_cl_adjust_time_delta.call<void>();
                return;
            }
            static const auto* p_demoplaying = game::exe_ptr<const int>(game::exe::addr_clc_demoplaying);
            static const auto* p_timescale_cvar = game::exe_ptr<void* const>(game::exe::addr_cvar_timescale_ptr);

            const float timescale = (p_timescale_cvar != nullptr)
                                  ? game::read_cvar_float(*p_timescale_cvar, 1.0f)
                                  : 1.0f;
            if ((p_demoplaying != nullptr && *p_demoplaying != 0) ||
                (timescale != 0.0f && timescale != 1.0f))
            {
                was_non_standard_clock = true;
                hook_cl_adjust_time_delta.call<void>();
                return;
            }
            if (was_non_standard_clock)
            {
                was_non_standard_clock = false;
                snap_sync.reset();
                render_clock.reset();
            }

            static auto* const p_new_snapshots = game::exe_ptr<int>(game::exe::addr_cl_new_snapshots);
            static auto* const p_extrapolated = game::exe_ptr<int>(game::exe::addr_cl_extrapolated_snap);
            static const auto* p_snap_server_time = game::exe_ptr<const int32_t>(game::exe::addr_cl_snap_server_time);
            static const auto* p_cls_realtime = game::exe_ptr<const int32_t>(game::exe::addr_cls_realtime);
            static auto* const p_server_time_delta = game::exe_ptr<int32_t>(game::exe::addr_cl_server_time_delta);
            static auto* const p_cl_server_time = game::exe_ptr<int32_t>(game::exe::addr_cl_server_time);
            static auto* const p_cl_old_server_time = game::exe_ptr<int32_t>(game::exe::addr_cl_old_server_time);

            if (p_new_snapshots != nullptr)
            {
                *p_new_snapshots = 0;
            }
            if (p_extrapolated != nullptr)
            {
                *p_extrapolated = 0;
            }

            if (p_snap_server_time == nullptr || p_cls_realtime == nullptr || p_server_time_delta == nullptr)
            {
                return;
            }

            const auto adj = snap_sync.on_new_snapshot(
                *p_snap_server_time,
                *p_cls_realtime,
                *p_server_time_delta);

            *p_server_time_delta = adj.new_server_time_delta;

            if (adj.mode == snapshot_clock_sync::adjust_mode::reset)
            {
                if (p_cl_old_server_time != nullptr)
                {
                    *p_cl_old_server_time = *p_snap_server_time;
                }
                if (p_cl_server_time != nullptr)
                {
                    *p_cl_server_time = *p_snap_server_time;
                }
                render_clock.reset();
                detail::cur_exact_render_ms = static_cast<double>(*p_snap_server_time) + detail::cur_sub_ms_frac;
            }

            static const auto* p_show_td = game::exe_ptr<void* const>(game::exe::addr_cvar_showtimedelta_ptr);
            if (p_show_td != nullptr && game::read_cvar_int(*p_show_td) != 0)
            {
                static const auto fn_printf = game::exe_fn<game::com_printf_t>(game::exe::addr_com_printf);
                if (fn_printf != nullptr)
                {
                    if (adj.mode == snapshot_clock_sync::adjust_mode::reset)
                    {
                        fn_printf("<RESET> ");
                    }
                    else if (adj.mode == snapshot_clock_sync::adjust_mode::fast)
                    {
                        fn_printf("<FAST> ");
                    }
                    fn_printf("%i ", *p_server_time_delta);
                }
            }
        }

        const std::array<util::inline_hook_spec, 11> timing_inline_specs{{
            { &hook_sys_milliseconds,       game::exe::addr_sys_milliseconds,       reinterpret_cast<void*>(&hk_sys_milliseconds) },
            { &hook_sys_init_time,          game::exe::addr_sys_init_time,          reinterpret_cast<void*>(&hk_sys_init_time) },
            { &hook_sys_sleep,              game::exe::addr_sys_sleep,              reinterpret_cast<void*>(&hk_sys_sleep) },
            { &hook_sys_que_event,          game::exe::addr_sys_que_event,          reinterpret_cast<void*>(&hk_sys_que_event) },
            { &hook_com_modify_msec,        game::exe::addr_com_modify_msec,        reinterpret_cast<void*>(&hk_com_modify_msec) },
            { &hook_cl_frame,               game::exe::addr_cl_frame,               reinterpret_cast<void*>(&hk_cl_frame) },
            { &hook_cl_send_command,        game::exe::addr_cl_send_command,        reinterpret_cast<void*>(&hk_cl_send_command) },
            { &hook_cl_create_new_commands, game::exe::addr_cl_create_new_commands, reinterpret_cast<void*>(&hk_cl_create_new_commands) },
            { &hook_cl_set_cgame_time,      game::exe::addr_cl_set_cgame_time,      reinterpret_cast<void*>(&hk_cl_set_cgame_time) },
            { &hook_cl_adjust_time_delta,   game::exe::addr_cl_adjust_time_delta,   reinterpret_cast<void*>(&hk_cl_adjust_time_delta) },
            { &hook_cl_parse_gamestate,     game::exe::addr_cl_parse_gamestate,     reinterpret_cast<void*>(&hk_cl_parse_gamestate) },
        }};

        const std::array<util::mid_hook_spec, 4> timing_mid_specs{{
            { &hook_sys_get_event_msg,   game::exe::addr_sys_get_event_msg,   &on_get_event_msg_time },
            { &hook_sys_get_event_empty, game::exe::addr_sys_get_event_empty, &on_get_event_empty },
            { &hook_com_frame_pacer,     game::exe::addr_com_frame_pacer,     &on_com_frame_pacer },
            { &hook_cl_mouse_rate,       game::exe::addr_cl_mouse_rate_site,  &on_cl_mouse_move_rate },
        }};
    }

    bool install_engine_hooks()
    {
        if (!util::install_inline_table(&game::exe_addr, timing_inline_specs))
        {
            return false;
        }
        if (!util::install_mid_table(&game::exe_addr, timing_mid_specs))
        {
            util::release_hook_table(timing_inline_specs);
            return false;
        }
        return true;
    }

    void release_engine_hooks()
    {
        util::release_hook_table(timing_mid_specs);
        util::release_hook_table(timing_inline_specs);
    }

    void reset_all()
    {
        msec_acc.reset();
        detail::fps_estimator.reset();
        detail::cur_sub_ms_frac = 0.0;
        reset_session();
    }

    void reset_session()
    {
        render_clock.reset();
        cmd_pacer.reset();
        rate_timer.reset();
        snap_sync.reset();
        loopback_limiter.reset();
        detail::cur_exact_render_ms = 0.0;
        cur_mouse_dt_ms = 1.0;
        has_unsent_cmd = false;
        was_non_standard_clock = false;
        detail::reset_prediction_session();
    }
}
