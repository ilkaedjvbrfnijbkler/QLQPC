#include "timing/timing_hooks.hpp"
#include "timing/timing_internal.hpp"

#include "cvars/cvars.hpp"
#include "game/game.hpp"
#include "timing/qpc_clock.hpp"
#include "util/hook_engine.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

namespace qlqpc::timing
{
    namespace
    {
        util::inline_hook hook_cg_predict_player_state{};
        util::inline_hook hook_cg_calc_view_values{};
        util::mid_hook hook_cg_pmove_post_angles{};
        util::mid_hook hook_cg_frame_interp{};
        util::mid_hook hook_cg_draw_fps{};

        HMODULE active_cgame = nullptr;
        bool cgame_attached = false;

        alignas(16) std::array<uint8_t, game::player_state_bytes> committed_pps{};
        float committed_step_change = 0.0f;
        int32_t committed_step_time = 0;
        int32_t committed_client_num = -1;
        bool committed_pps_valid = false;

        bool ephemeral_pmove_active = false;
        float ephemeral_dt_sec = 0.0f;

        void apply_precise_viewangles_to_ps(uint8_t* ps)
        {
            if (ps == nullptr)
            {
                return;
            }
            const int32_t pm_type = *reinterpret_cast<const int32_t*>(ps + game::ps_off::pm_type);
            const int32_t health = *reinterpret_cast<const int32_t*>(ps + game::ps_off::health);

            // allow live mouse rotation when camera is in freecam states matching PM_UpdateViewAngles
            if (pm_type == 5 || pm_type == 6 || (pm_type != 2 && pm_type != 4 && health <= 0))
            {
                return;
            }

            static const auto* p_cl_viewangles = game::exe_ptr<const float>(game::exe::addr_cl_viewangles);
            if (p_cl_viewangles == nullptr)
            {
                return;
            }

            const auto* ps_delta_angles = reinterpret_cast<const int32_t*>(ps + game::ps_off::delta_angles);
            auto* const ps_viewangles = reinterpret_cast<float*>(ps + game::ps_off::viewangles);
            compute_precise_viewangles(p_cl_viewangles, ps_delta_angles, ps_viewangles);
        }

        void build_ephemeral_subframe_cmd(uint8_t* out_cmd, int32_t committed_cmd_time)
        {
            static const auto* p_cmd_number = game::exe_ptr<const int>(game::exe::addr_cmd_number);
            static const auto* p_cmd_ring = game::exe_ptr<const uint8_t>(game::exe::addr_cmd_ring);
            static const auto* p_cl_viewangles = game::exe_ptr<const float>(game::exe::addr_cl_viewangles);
            static const auto* p_key_catchers = game::exe_ptr<const int>(game::exe::addr_cls_key_catchers);

            if (p_cmd_number != nullptr && p_cmd_ring != nullptr)
            {
                const uint8_t* latest = p_cmd_ring + ((*p_cmd_number & 63) * game::usercmd_bytes);
                std::memcpy(out_cmd, latest, game::usercmd_bytes);
            }
            else
            {
                std::memset(out_cmd, 0, game::usercmd_bytes);
            }

            *reinterpret_cast<int32_t*>(out_cmd) = committed_cmd_time + 1;

            auto& buttons = *reinterpret_cast<uint32_t*>(out_cmd + 16);
            buttons &= ~(1u | 4u | 8u);

            if (p_cl_viewangles != nullptr)
            {
                auto* const cmd_angles = reinterpret_cast<int32_t*>(out_cmd + 4);
                cmd_angles[0] = static_cast<int32_t>(ql_angle_to_short(p_cl_viewangles[0]));
                cmd_angles[1] = static_cast<int32_t>(ql_angle_to_short(p_cl_viewangles[1]));
                cmd_angles[2] = static_cast<int32_t>(ql_angle_to_short(p_cl_viewangles[2]));
            }

            if (p_key_catchers == nullptr || (*p_key_catchers & 0xFFFFFFEF) == 0)
            {
                static const auto* p_speed_active = game::exe_ptr<const int>(game::exe::addr_in_speed_active);
                static const auto* p_cl_run_cvar = game::exe_ptr<void* const>(game::exe::addr_cvar_cl_run_ptr);
                static const auto* p_fwd_active = game::exe_ptr<const int>(game::exe::addr_in_forward_active);
                static const auto* p_back_active = game::exe_ptr<const int>(game::exe::addr_in_back_active);
                static const auto* p_right_active = game::exe_ptr<const int>(game::exe::addr_in_moveright_active);
                static const auto* p_left_active = game::exe_ptr<const int>(game::exe::addr_in_moveleft_active);
                static const auto* p_up_active = game::exe_ptr<const int>(game::exe::addr_in_up_active);
                static const auto* p_down_active = game::exe_ptr<const int>(game::exe::addr_in_down_active);

                const int speed_key = (p_speed_active != nullptr && *p_speed_active != 0) ? 1 : 0;
                const int cl_run = (p_cl_run_cvar != nullptr && game::read_cvar_int(*p_cl_run_cvar, 1) != 0) ? 1 : 0;
                int move_val = 127;
                if (speed_key != cl_run)
                {
                    buttons &= ~0x10u;
                    move_val = 127;
                }
                else
                {
                    buttons |= 0x10u;
                    move_val = 64;
                }

                const int fwd = ((p_fwd_active != nullptr && *p_fwd_active != 0) ? move_val : 0)
                              - ((p_back_active != nullptr && *p_back_active != 0) ? move_val : 0);
                const int rgt = ((p_right_active != nullptr && *p_right_active != 0) ? move_val : 0)
                              - ((p_left_active != nullptr && *p_left_active != 0) ? move_val : 0);
                const int up = ((p_up_active != nullptr && *p_up_active != 0) ? move_val : 0)
                             - ((p_down_active != nullptr && *p_down_active != 0) ? move_val : 0);

                out_cmd[23] = static_cast<uint8_t>(static_cast<int8_t>(std::clamp(fwd, -127, 127)));
                out_cmd[24] = static_cast<uint8_t>(static_cast<int8_t>(std::clamp(rgt, -127, 127)));
                out_cmd[25] = static_cast<uint8_t>(static_cast<int8_t>(std::clamp(up, -127, 127)));
            }
            else
            {
                out_cmd[23] = 0;
                out_cmd[24] = 0;
                out_cmd[25] = 0;
            }
        }

        void on_cg_pmove_post_angles(util::mid_context& ctx)
        {
            if (!ephemeral_pmove_active || active_cgame == nullptr)
            {
                return;
            }

            auto* const p_pml_frametime = game::cgame_ptr<float>(active_cgame, game::cgame::addr_pml_frametime);
            auto* const p_pml_msec = game::cgame_ptr<int32_t>(active_cgame, game::cgame::addr_pml_msec);
            if (p_pml_frametime != nullptr)
            {
                *p_pml_frametime = ephemeral_dt_sec;
            }
            if (p_pml_msec != nullptr)
            {
                *p_pml_msec = 0;
            }

            // EBP points to pmove_t whose first field is playerState_t*
            if (ctx.ebp != 0)
            {
                auto* const ps = *reinterpret_cast<uint8_t* const*>(ctx.ebp);
                apply_precise_viewangles_to_ps(ps);
            }
        }

        int __cdecl hk_cg_predict_player_state()
        {
            const HMODULE h_cg = active_cgame;
            if (h_cg == nullptr)
            {
                return hook_cg_predict_player_state.call<int>();
            }

            auto* const p_valid_pps = game::cgame_ptr<int32_t>(h_cg, game::cgame::addr_cg_valid_pps);
            auto* const p_pps = game::cgame_ptr<uint8_t>(h_cg, game::cgame::addr_cg_pps);
            auto* const p_step_change = game::cgame_ptr<float>(h_cg, game::cgame::addr_cg_step_change);
            auto* const p_step_time = game::cgame_ptr<int32_t>(h_cg, game::cgame::addr_cg_step_time);

            if (p_valid_pps != nullptr && *p_valid_pps == 0)
            {
                committed_pps_valid = false;
            }
            else if (committed_pps_valid && p_pps != nullptr)
            {
                const int32_t cur_client = *reinterpret_cast<const int32_t*>(p_pps + game::ps_off::client_num);
                if (cur_client == committed_client_num)
                {
                    std::memcpy(p_pps, committed_pps.data(), game::player_state_bytes);
                    if (p_step_change != nullptr)
                    {
                        *p_step_change = committed_step_change;
                    }
                    if (p_step_time != nullptr)
                    {
                        *p_step_time = committed_step_time;
                    }
                }
                else
                {
                    committed_pps_valid = false;
                }
            }

            const int result = hook_cg_predict_player_state.call<int>();

            if (!cvars::is_decoupling_active())
            {
                committed_pps_valid = false;
                ephemeral_pmove_active = false;
                return result;
            }

            if (p_valid_pps == nullptr || *p_valid_pps == 0 || p_pps == nullptr)
            {
                committed_pps_valid = false;
                return result;
            }

            std::memcpy(committed_pps.data(), p_pps, game::player_state_bytes);
            committed_client_num = *reinterpret_cast<const int32_t*>(p_pps + game::ps_off::client_num);
            committed_step_change = (p_step_change != nullptr) ? *p_step_change : 0.0f;
            committed_step_time = (p_step_time != nullptr) ? *p_step_time : 0;
            committed_pps_valid = true;

            const auto* p_hyperspace = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_hyperspace);
            const auto* p_demo = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_demo_playback);
            const auto* p_nopredict = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_nopredict_int);
            const auto* p_cg_snap = game::cgame_ptr<const uintptr_t>(h_cg, game::cgame::addr_cg_snap);
            const auto* p_cg_time = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_time);

            if ((p_hyperspace != nullptr && *p_hyperspace != 0) ||
                (p_demo != nullptr && *p_demo != 0) ||
                (p_nopredict != nullptr && *p_nopredict != 0) ||
                p_cg_snap == nullptr || *p_cg_snap == 0 || p_cg_time == nullptr)
            {
                return result;
            }

            const uint32_t snap_pm_flags = *reinterpret_cast<const uint32_t*>(*p_cg_snap + game::snap_off::pm_flags);
            if ((snap_pm_flags & 0x1000u) != 0)
            {
                return result;
            }

            const int32_t pm_type = *reinterpret_cast<const int32_t*>(p_pps + game::ps_off::pm_type);
            if (pm_type == 5 || pm_type == 6)
            {
                return result;
            }

            if (detail::cur_exact_render_ms <= 0.0)
            {
                return result;
            }
            const int32_t committed_cmd_time = *reinterpret_cast<const int32_t*>(p_pps + game::ps_off::command_time);
            const float dt_sub_sec = compute_subframe_pmove_dt_sec(detail::cur_exact_render_ms, committed_cmd_time);

            if (dt_sub_sec <= 1.0e-6f)
            {
                apply_precise_viewangles_to_ps(p_pps);
                return result;
            }

            auto* const p_cg_pmove = game::cgame_ptr<void>(h_cg, game::cgame::addr_cg_pmove);
            auto* const p_cg_pmove_cmd = game::cgame_ptr<uint8_t>(h_cg, game::cgame::addr_cg_pmove_cmd);
            auto* const p_pmove_fixed = game::cgame_ptr<int32_t>(h_cg, game::cgame::addr_cg_pmove_fixed);
            auto* const p_step_delta = game::cgame_ptr<float>(h_cg, game::cgame::addr_cg_pmove_step_delta);
            auto* const p_c_pmove = game::cgame_ptr<int32_t>(h_cg, game::cgame::addr_c_pmove_counter);
            const auto fn_pmove_single = game::cgame_fn<game::pmove_single_t>(h_cg, game::cgame::addr_pmove_single);
            const auto fn_step_offset = game::cgame_fn<game::cg_step_offset_t>(h_cg, game::cgame::addr_step_offset);

            if (p_cg_pmove == nullptr || p_cg_pmove_cmd == nullptr || fn_pmove_single == nullptr)
            {
                apply_precise_viewangles_to_ps(p_pps);
                return result;
            }

            const int32_t saved_c_pmove = (p_c_pmove != nullptr) ? *p_c_pmove : 0;

            *reinterpret_cast<uint8_t**>(p_cg_pmove) = p_pps;
            if (p_pmove_fixed != nullptr)
            {
                *p_pmove_fixed = 0;
            }
            if (p_step_delta != nullptr)
            {
                *p_step_delta = 0.0f;
            }

            build_ephemeral_subframe_cmd(p_cg_pmove_cmd, committed_cmd_time);

            ephemeral_dt_sec = dt_sub_sec;
            ephemeral_pmove_active = true;
            fn_pmove_single(p_cg_pmove);
            ephemeral_pmove_active = false;

            if (fn_step_offset != nullptr)
            {
                if (p_step_delta != nullptr && *p_step_delta != 0.0f && p_step_time != nullptr)
                {
                    auto* const p_pm_step_time = game::cgame_ptr<int32_t>(h_cg, game::cgame::addr_cg_pmove_step_time);
                    if (p_pm_step_time != nullptr && *p_pm_step_time <= *p_step_time)
                    {
                        *p_pm_step_time = *p_step_time + 1;
                    }
                }
                fn_step_offset();
            }

            // keep committed weapon/fire flags intact so cg_drawGun 0 beam stays visible during ephemeral prediction
            restore_weapon_state(p_pps, committed_pps.data());

            if (p_c_pmove != nullptr)
            {
                *p_c_pmove = saved_c_pmove;
            }

            return result;
        }

        int __cdecl hk_cg_calc_view_values()
        {
            const int res = hook_cg_calc_view_values.call<int>();

            if (!cvars::is_decoupling_active())
            {
                return res;
            }

            const HMODULE h_cg = active_cgame;
            if (h_cg == nullptr)
            {
                return res;
            }

            const auto* p_cg_time = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_time);
            auto* const p_vieworg = game::cgame_ptr<float>(h_cg, game::cgame::addr_cg_refdef_vieworg);
            const auto* p_pps = game::cgame_ptr<const uint8_t>(h_cg, game::cgame::addr_cg_pps);
            if (p_cg_time == nullptr || p_vieworg == nullptr || p_pps == nullptr)
            {
                return res;
            }

            if (detail::cur_exact_render_ms <= 0.0)
            {
                return res;
            }
            const int32_t cg_time = *p_cg_time;
            const float frac = std::clamp(
                static_cast<float>(detail::cur_exact_render_ms - static_cast<double>(cg_time)),
                0.0f,
                0.999999f);
            if (frac <= 1.0e-6f)
            {
                return res;
            }

            const int32_t pm_type = *reinterpret_cast<const int32_t*>(p_pps + game::ps_off::pm_type);
            if (pm_type == 5)
            {
                return res;
            }

            const auto* p_err_decay = game::cgame_ptr<const float>(h_cg, game::cgame::addr_cg_error_decay_val);
            const auto* p_err_time = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_pred_err_time);
            const auto* p_err_vec = game::cgame_ptr<const float>(h_cg, game::cgame::addr_cg_pred_err_vec);
            if (p_err_decay != nullptr && *p_err_decay > 0.0f && p_err_time != nullptr && *p_err_time != 0 && p_err_vec != nullptr)
            {
                const float decay_ms = *p_err_decay;
                const float f_int = (decay_ms - static_cast<float>(cg_time - *p_err_time)) / decay_ms;
                if (f_int > 0.0f && f_int < 1.0f)
                {
                    const float f_exact = std::max(0.0f, f_int - (frac / decay_ms));
                    const float df = f_exact - f_int;
                    p_vieworg[0] += p_err_vec[0] * df;
                    p_vieworg[1] += p_err_vec[1] * df;
                    p_vieworg[2] += p_err_vec[2] * df;
                }
            }

            const auto* p_third_person = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_third_person);
            const auto* p_cg_snap = game::cgame_ptr<const uintptr_t>(h_cg, game::cgame::addr_cg_snap);
            if ((p_third_person == nullptr || *p_third_person == 0) && p_cg_snap != nullptr && *p_cg_snap != 0)
            {
                const int32_t snap_pm_type = *reinterpret_cast<const int32_t*>(*p_cg_snap + game::snap_off::pm_type);
                const int32_t snap_health = *reinterpret_cast<const int32_t*>(*p_cg_snap + game::snap_off::health);
                if (snap_pm_type != 5 && snap_health > 0)
                {
                    const auto* p_duck_change = game::cgame_ptr<const float>(h_cg, game::cgame::addr_cg_duck_change);
                    const auto* p_duck_time = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_duck_time);
                    if (p_duck_change != nullptr && p_duck_time != nullptr)
                    {
                        const int32_t dt_duck = cg_time - *p_duck_time;
                        if (dt_duck >= 0 && dt_duck < 100)
                        {
                            p_vieworg[2] += (frac * (*p_duck_change)) * 0.01f;
                        }
                    }

                    const auto* p_step_change = game::cgame_ptr<const float>(h_cg, game::cgame::addr_cg_step_change);
                    const auto* p_step_time = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_step_time);
                    if (p_step_change != nullptr && p_step_time != nullptr)
                    {
                        const int32_t dt_step = cg_time - *p_step_time;
                        if (dt_step >= 0 && dt_step < 100)
                        {
                            p_vieworg[2] += (frac * (*p_step_change)) * 0.01f;
                        }
                    }
                }
            }

            return res;
        }

        void on_cg_frame_interpolation(util::mid_context&)
        {
            if (!cvars::is_decoupling_active())
            {
                return;
            }
            const HMODULE h_cg = active_cgame;
            if (h_cg == nullptr)
            {
                return;
            }

            const auto* p_snap = game::cgame_ptr<const uintptr_t>(h_cg, game::cgame::addr_cg_snap);
            const auto* p_next_snap = game::cgame_ptr<const uintptr_t>(h_cg, game::cgame::addr_cg_next_snap);
            const auto* p_cg_time = game::cgame_ptr<const int32_t>(h_cg, game::cgame::addr_cg_time);
            auto* const p_interp = game::cgame_ptr<float>(h_cg, game::cgame::addr_cg_frame_interp);

            if (p_snap == nullptr || p_next_snap == nullptr || p_cg_time == nullptr || p_interp == nullptr)
            {
                return;
            }
            if (*p_snap == 0 || *p_next_snap == 0)
            {
                return;
            }

            const int32_t snap_st = *reinterpret_cast<const int32_t*>(*p_snap + game::snap_off::server_time);
            const int32_t next_snap_st = *reinterpret_cast<const int32_t*>(*p_next_snap + game::snap_off::server_time);
            if (next_snap_st <= snap_st)
            {
                return;
            }

            if (detail::cur_exact_render_ms <= 0.0)
            {
                return;
            }
            *p_interp = compute_continuous_frame_interpolation(detail::cur_exact_render_ms, snap_st, next_snap_st);
        }

        void on_cg_draw_fps(util::mid_context& ctx)
        {
            const int fps = detail::fps_estimator.on_frame(clock_now_ticks(), clock_frequency());
            ctx.eax = static_cast<uintptr_t>(fps);
        }

        const std::array<util::inline_hook_spec, 2> pred_cgame_inline_specs{{
            { &hook_cg_predict_player_state, game::cgame::addr_predict_player_state, reinterpret_cast<void*>(&hk_cg_predict_player_state) },
            { &hook_cg_calc_view_values,     game::cgame::addr_calc_view_values,     reinterpret_cast<void*>(&hk_cg_calc_view_values) },
        }};

        const std::array<util::mid_hook_spec, 3> pred_cgame_mid_specs{{
            { &hook_cg_pmove_post_angles, game::cgame::addr_pmove_post_angles, &on_cg_pmove_post_angles },
            { &hook_cg_frame_interp,      game::cgame::addr_frame_interp_site, &on_cg_frame_interpolation },
            { &hook_cg_draw_fps,          game::cgame::addr_draw_fps_site,     &on_cg_draw_fps },
        }};
    }

    namespace detail
    {
        void reset_prediction_session()
        {
            committed_pps_valid = false;
            committed_client_num = -1;
            committed_step_change = 0.0f;
            committed_step_time = 0;
            ephemeral_pmove_active = false;
            ephemeral_dt_sec = 0.0f;
        }
    }

    bool attach_cgame(HMODULE h_cgame)
    {
        if (h_cgame == nullptr)
        {
            return false;
        }
        if (cgame_attached && active_cgame == h_cgame)
        {
            return true;
        }

        util::release_hook_table(pred_cgame_mid_specs);
        util::release_hook_table(pred_cgame_inline_specs);

        active_cgame = h_cgame;
        committed_pps_valid = false;
        ephemeral_pmove_active = false;
        detail::fps_estimator.reset();

        const auto resolve_cgame = [h_cgame](uintptr_t preferred_va)
        {
            return game::cgame_addr(h_cgame, preferred_va);
        };
        if (!util::install_inline_table(resolve_cgame, pred_cgame_inline_specs) ||
            !util::install_mid_table(resolve_cgame, pred_cgame_mid_specs))
        {
            util::release_hook_table(pred_cgame_mid_specs);
            util::release_hook_table(pred_cgame_inline_specs);
            active_cgame = nullptr;
            cgame_attached = false;
            return false;
        }

        cgame_attached = true;
        return true;
    }

    void detach_cgame()
    {
        if (!cgame_attached)
        {
            return;
        }

        util::release_hook_table(pred_cgame_mid_specs);
        util::release_hook_table(pred_cgame_inline_specs);

        active_cgame = nullptr;
        cgame_attached = false;
        committed_pps_valid = false;
        ephemeral_pmove_active = false;
    }

    bool is_ephemeral_pmove()
    {
        return ephemeral_pmove_active;
    }
}
