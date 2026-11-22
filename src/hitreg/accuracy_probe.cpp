#include "hitreg/accuracy_probe.hpp"

#include "game/game.hpp"
#include "hitreg/hitreg_state.hpp"
#include "timing/timing_hooks.hpp"
#include "util/hook_engine.hpp"

#include <array>
#include <atomic>
#include <cstring>
#include <utility>

namespace qlqpc::hitreg
{
    namespace
    {
        util::mid_hook hook_fire{};
        util::mid_hook hook_feedback{};
        util::mid_hook hook_draw{};
        util::mid_hook hook_cylinder_scale{};

        bool ray_trace_active = false;
        std::atomic<bool> probe_enabled{false};

        HMODULE active_cgame = nullptr;
        bool cgame_hooked = false;
        hitreg_state state{};
        SRWLOCK state_lock = SRWLOCK_INIT;

        struct cvar_scale_stub
        {
            uint8_t pad[0x2C]{};
            float value{1.1f};
        };
        constexpr cvar_scale_stub cylinder_scale_1_1{};

        void on_engine_cylinder_scale(util::mid_context& ctx)
        {
            if (ray_trace_active)
            {
                ctx.eax = reinterpret_cast<uintptr_t>(&cylinder_scale_1_1);
            }
        }

        __declspec(noinline) float native_scaled_add(float base, float value, double scale)
        {
            float result{};
            __asm
            {
                fld dword ptr [value]
                fmul qword ptr [scale]
                fadd dword ptr [base]
                fstp dword ptr [result]
            }
            return result;
        }

        bool native_angle_vectors(uintptr_t fn, const float* angles, float* forward)
        {
            if (fn == 0 || angles == nullptr || forward == nullptr)
            {
                return false;
            }
            __asm
            {
                push ebx
                push esi
                push edi
                mov esi, angles
                mov edi, forward
                xor ebx, ebx
                push 0
                mov eax, fn
                call eax
                add esp, 4
                pop edi
                pop esi
                pop ebx
            }
            return true;
        }

        bool native_lg_point_trace(
            HMODULE h_cgame,
            uint32_t* result,
            const float* start,
            const float* end,
            int skip,
            int mask)
        {
            const auto* trace_mode = game::cgame_ptr<const int32_t>(h_cgame, game::cgame::addr_trace_mode);
            if (trace_mode == nullptr)
            {
                return false;
            }
            if (*trace_mode != 0)
            {
                const uintptr_t fn_alt = game::cgame_addr(h_cgame, game::cgame::addr_alt_point_trace);
                if (fn_alt == 0)
                {
                    return false;
                }
                const bool prev = ray_trace_active;
                ray_trace_active = true;
                __asm
                {
                    push ebx
                    mov eax, fn_alt
                    mov ecx, result
                    mov edx, start
                    mov ebx, end
                    push mask
                    push skip
                    call eax
                    add esp, 8
                    pop ebx
                }
                ray_trace_active = prev;
                return true;
            }
            const auto trace_fn = game::cgame_fn<game::point_trace_t>(h_cgame, game::cgame::addr_point_trace);
            if (trace_fn == nullptr)
            {
                return false;
            }
            const bool prev = ray_trace_active;
            ray_trace_active = true;
            trace_fn(result, start, nullptr, nullptr, end, skip, mask);
            ray_trace_active = prev;
            return true;
        }

        struct client_ray_result
        {
            hitreg_trace_kind kind{hitreg_trace_kind::other};
            int32_t entity_number{-1};
            bool opponent_contact{false};
            bool available{false};
        };

        client_ray_result trace_native_client_lg_ray(HMODULE h_cgame, const uint8_t* player_state)
        {
            client_ray_result result{};
            const auto* serverinfo_flags =
                game::cgame_ptr<const int32_t>(h_cgame, game::cgame::addr_cgs_serverinfo_flags);
            if (player_state == nullptr || serverinfo_flags == nullptr)
            {
                return result;
            }
            const int32_t shooter = *reinterpret_cast<const int32_t*>(player_state + game::ps_off::client_num);
            if (shooter < 0 || shooter >= game::cgame::max_clients)
            {
                return result;
            }

            float angles[3]{};
            float forward[3]{};
            float origin[3]{};
            std::memcpy(angles, player_state + game::ps_off::viewangles, sizeof(angles));
            std::memcpy(origin, player_state + game::ps_off::origin, sizeof(origin));

            const uintptr_t fn_angles = game::cgame_addr(h_cgame, game::cgame::addr_angle_vectors);
            if (!native_angle_vectors(fn_angles, angles, forward))
            {
                return result;
            }

            const int32_t pm_flags = *reinterpret_cast<const int32_t*>(player_state + game::ps_off::pm_flags);
            const int32_t viewheight = *reinterpret_cast<const int32_t*>(player_state + game::ps_off::viewheight);
            const double muzzle_offset = (pm_flags & 1) ? 3.0 : 5.0;

            float start[3]{};
            float end[3]{};
            for (int axis = 0; axis < 3; ++axis)
            {
                const float snapped = static_cast<float>(static_cast<int32_t>(origin[axis]));
                const float base = (axis == 2) ? (snapped + static_cast<float>(viewheight)) : snapped;
                start[axis] = native_scaled_add(base, forward[axis], muzzle_offset);
                end[axis] = native_scaled_add(start[axis], forward[axis], 768.0);
            }

            uint32_t trace_result[14]{};
            const int32_t mask = (*serverinfo_flags & 0x02000000) != 0 ? 0x00000001 : 0x06000001;
            if (!native_lg_point_trace(h_cgame, trace_result, start, end, shooter, mask))
            {
                return result;
            }

            result.entity_number = static_cast<int32_t>(trace_result[13]);
            bool opponent = (result.entity_number >= 0 &&
                             result.entity_number < game::cgame::max_clients &&
                             result.entity_number != shooter);
            if (opponent)
            {
                const int32_t local_team = game::read_client_team(h_cgame, shooter);
                const int32_t candidate_team = game::read_client_team(h_cgame, result.entity_number);
                opponent = hitreg_is_opponent(local_team, candidate_team);
            }

            result.opponent_contact = opponent;
            result.kind = opponent                       ? hitreg_trace_kind::player
                          : result.entity_number == 1022 ? hitreg_trace_kind::world
                          : result.entity_number == 1023 ? hitreg_trace_kind::none
                                                         : hitreg_trace_kind::other;
            result.available = true;
            return result;
        }

        bool measure_text(uintptr_t fn, const char* text, float scale, int font, int& width, int& height)
        {
            if (fn == 0 || text == nullptr)
            {
                return false;
            }
            width = 0;
            height = 0;
            int* p_w = &width;
            int* p_h = &height;
            __asm
            {
                mov ecx, text
                xor edx, edx
                mov esi, p_w
                xor edi, edi
                push scale
                push font
                call fn
                add esp, 8
                mov ecx, text
                xor edx, edx
                xor esi, esi
                mov edi, p_h
                push scale
                push font
                call fn
                add esp, 8
            }
            return (width > 0 && height > 0);
        }

        void on_fire_weapon(util::mid_context& ctx)
        {
            if (!probe_enabled.load(std::memory_order_relaxed))
            {
                return;
            }
            if (timing::is_ephemeral_pmove())
            {
                return;
            }
            if (ctx.esi == 0 || ctx.edx != 0x14)
            {
                return;
            }
            const auto* player_state = *reinterpret_cast<const uint8_t* const*>(ctx.esi);
            if (player_state == nullptr)
            {
                return;
            }

            const int32_t command_time = *reinterpret_cast<const int32_t*>(player_state + game::ps_off::command_time);
            const int32_t weapon = *reinterpret_cast<const int32_t*>(player_state + game::ps_off::weapon);

            AcquireSRWLockExclusive(&state_lock);
            if (!state.wants_client_ray(weapon, command_time))
            {
                state.on_weapon_fire(weapon, command_time, hitreg_trace_kind::other, false, true);
                ReleaseSRWLockExclusive(&state_lock);
                return;
            }

            HMODULE h_cg = active_cgame;
            ReleaseSRWLockExclusive(&state_lock);
            if (h_cg == nullptr)
            {
                return;
            }

            const client_ray_result ray = trace_native_client_lg_ray(h_cg, player_state);
            AcquireSRWLockExclusive(&state_lock);
            state.on_weapon_fire(weapon, command_time, ray.kind, ray.opponent_contact, ray.available);
            ReleaseSRWLockExclusive(&state_lock);
        }

        void on_server_feedback(util::mid_context& ctx)
        {
            if (!probe_enabled.load(std::memory_order_relaxed))
            {
                return;
            }
            if (ctx.esi == 0 || ctx.ebp == 0)
            {
                return;
            }
            const auto* new_ps = reinterpret_cast<const uint8_t*>(ctx.esi);
            const auto* old_ps = reinterpret_cast<const uint8_t*>(ctx.ebp);

            const int32_t new_command_time = *reinterpret_cast<const int32_t*>(new_ps + game::ps_off::command_time);
            const int32_t old_command_time = *reinterpret_cast<const int32_t*>(old_ps + game::ps_off::command_time);
            const int32_t new_hits = *reinterpret_cast<const int32_t*>(new_ps + game::ps_off::pers_hits);
            const int32_t old_hits = *reinterpret_cast<const int32_t*>(old_ps + game::ps_off::pers_hits);

            AcquireSRWLockExclusive(&state_lock);
            state.on_server_feedback_transition(old_command_time, new_command_time, old_hits, new_hits);
            ReleaseSRWLockExclusive(&state_lock);
        }

        void on_draw_fps(util::mid_context& ctx)
        {
            if (!probe_enabled.load(std::memory_order_relaxed))
            {
                return;
            }
            if (ctx.esp == 0)
            {
                return;
            }

            char text[64]{};
            AcquireSRWLockShared(&state_lock);
            const bool should_draw = state.format_display_text(text, sizeof(text));
            ReleaseSRWLockShared(&state_lock);

            if (!should_draw || text[0] == '\0')
            {
                return;
            }

            HMODULE h_cg = active_cgame;
            if (h_cg == nullptr)
            {
                return;
            }

            const auto* args = reinterpret_cast<const uint32_t*>(ctx.esp);
            float source_y = 0.0f;
            float scale = 0.0f;
            float adjust = 0.0f;
            std::memcpy(&source_y, &args[1], sizeof(source_y));
            std::memcpy(&scale, &args[3], sizeof(scale));
            std::memcpy(&adjust, &args[6], sizeof(adjust));
            const int font = static_cast<int>(args[2]);

            int width = 0;
            int height = 0;
            const uintptr_t fn_measure = game::cgame_addr(h_cg, game::cgame::addr_text_measure);
            if (!measure_text(fn_measure, text, scale, font, width, height))
            {
                return;
            }

            const auto paint_fn = game::cgame_fn<game::text_paint_t>(h_cg, game::cgame::addr_text_paint);
            const auto* color = reinterpret_cast<const float*>(args[4]);
            if (paint_fn != nullptr && color != nullptr)
            {
                constexpr float fps_right_edge = 635.0f;
                const float line_spacing = static_cast<float>(height) * 0.62f;
                paint_fn(
                    fps_right_edge - static_cast<float>(width),
                    source_y + line_spacing,
                    font,
                    scale,
                    color,
                    text,
                    adjust,
                    static_cast<int>(args[7]),
                    static_cast<int>(args[8]));
            }
        }

        const std::array<util::mid_hook_spec, 3> cgame_mid_specs{{
            { &hook_fire,     game::cgame::addr_hitreg_fire,     &on_fire_weapon },
            { &hook_feedback, game::cgame::addr_hitreg_feedback, &on_server_feedback },
            { &hook_draw,     game::cgame::addr_hitreg_draw,     &on_draw_fps },
        }};

        bool hook_cgame(HMODULE h_cg)
        {
            if (h_cg == nullptr)
            {
                return false;
            }
            if (cgame_hooked && active_cgame == h_cg)
            {
                return true;
            }

            auto cyl = util::create_mid(
                reinterpret_cast<void*>(game::exe_addr(game::exe::addr_cylinder_scale)),
                &on_engine_cylinder_scale);
            if (!cyl)
            {
                active_cgame = nullptr;
                return false;
            }

            const auto resolve_cgame = [h_cg](uintptr_t preferred_va)
            {
                return game::cgame_addr(h_cg, preferred_va);
            };
            if (!util::install_mid_table(resolve_cgame, cgame_mid_specs))
            {
                active_cgame = nullptr;
                return false;
            }

            hook_cylinder_scale = std::move(cyl);
            cgame_hooked = true;
            return true;
        }

        void unhook_cgame()
        {
            if (!cgame_hooked)
            {
                return;
            }

            util::release_hook_table(cgame_mid_specs);
            hook_cylinder_scale.reset();
            cgame_hooked = false;
        }
    }

    void init()
    {
        InitializeSRWLock(&state_lock);
        AcquireSRWLockExclusive(&state_lock);
        state.reset();
        ReleaseSRWLockExclusive(&state_lock);
    }

    void shutdown()
    {
        unhook_cgame();
    }

    void reset()
    {
        AcquireSRWLockExclusive(&state_lock);
        state.reset();
        ReleaseSRWLockExclusive(&state_lock);
    }

    void set_enabled(bool enabled)
    {
        const bool prev = probe_enabled.exchange(enabled, std::memory_order_acq_rel);
        if (prev != enabled)
        {
            reset();
        }
    }

    bool on_cgame_loaded(HMODULE h_cg)
    {
        if (h_cg == nullptr)
        {
            return false;
        }
        if (cgame_hooked && active_cgame == h_cg)
        {
            return true;
        }
        if (cgame_hooked)
        {
            unhook_cgame();
        }
        active_cgame = h_cg;
        return hook_cgame(h_cg);
    }

    void on_cgame_unloading()
    {
        unhook_cgame();
        active_cgame = nullptr;
    }

    void on_usercmd(int32_t server_time, bool attack_held)
    {
        if (!probe_enabled.load(std::memory_order_relaxed))
        {
            return;
        }
        AcquireSRWLockExclusive(&state_lock);
        state.on_usercmd(server_time, attack_held);
        ReleaseSRWLockExclusive(&state_lock);
    }
}
