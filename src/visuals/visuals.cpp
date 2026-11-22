#include "visuals/visuals.hpp"

#include "cvars/cvars.hpp"
#include "game/game.hpp"
#include "util/hook_engine.hpp"
#include "visuals/lightning_logic.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace qlqpc::visuals
{
    namespace
    {
        util::inline_hook hook_rb_begin_surface{};
        util::inline_hook hook_rb_surface_lightning_bolt{};
        util::inline_hook hook_fons_draw_text{};
        util::mid_hook hook_rb_iterate_post_colors{};
        util::mid_hook hook_fons_bounds_color{};

        util::mid_hook hook_cg_weapon_with_powerups{};
        util::mid_hook hook_cg_weapon_flash{};
        util::mid_hook hook_cg_lightning_bolt{};

        HMODULE active_cgame = nullptr;
        bool cgame_attached = false;

        std::array<game::ql_fog, 2> custom_fogs{};
        bool fog_enabled = false;
        bool fog_override_native = true;
        uint8_t* saved_tr_world = nullptr;
        game::ql_fog* saved_orig_fogs_ptr = nullptr;
        int saved_orig_numfogs = 0;
        game::ql_fog saved_orig_fog1{};
        bool saved_has_orig_fog1 = false;
        bool patched_map_had_native_fog = false;
        bool world_fog_patched = false;

        std::array<uint8_t, max_tess_opacity_verts> tess_vert_opacity{};
        bool tess_has_lightning_opacity = false;

        void restore_native_world_fog()
        {
            if (!world_fog_patched)
            {
                return;
            }
            auto* const tr_world_slot = game::exe_ptr<uint8_t*>(game::exe::addr_tr_world_ptr);
            if (tr_world_slot != nullptr && *tr_world_slot == saved_tr_world && saved_tr_world != nullptr)
            {
                auto* const numfogs_ptr = game::exe_ptr<int>(game::exe::addr_world_numfogs);
                auto* const fogs_ptr_ptr = game::exe_ptr<game::ql_fog*>(game::exe::addr_world_fogs_ptr);
                if (numfogs_ptr != nullptr && fogs_ptr_ptr != nullptr)
                {
                    if (!patched_map_had_native_fog)
                    {
                        if (*fogs_ptr_ptr == custom_fogs.data())
                        {
                            *fogs_ptr_ptr = saved_orig_fogs_ptr;
                            *numfogs_ptr = saved_orig_numfogs;
                        }
                    }
                    else if (saved_has_orig_fog1 &&
                             *fogs_ptr_ptr == saved_orig_fogs_ptr &&
                             *numfogs_ptr == saved_orig_numfogs)
                    {
                        (*fogs_ptr_ptr)[1] = saved_orig_fog1;
                    }
                }
            }
            world_fog_patched = false;
        }

        void __cdecl hk_rb_begin_surface(void* shader, int fog_num)
        {
            tess_has_lightning_opacity = false;

            if (shader != nullptr)
            {
                auto* const tr_world_slot = game::exe_ptr<uint8_t*>(game::exe::addr_tr_world_ptr);
                if (tr_world_slot != nullptr && *tr_world_slot != nullptr)
                {
                    auto* const numfogs_ptr = game::exe_ptr<int>(game::exe::addr_world_numfogs);
                    auto* const fogs_ptr_ptr = game::exe_ptr<game::ql_fog*>(game::exe::addr_world_fogs_ptr);
                    if (numfogs_ptr != nullptr && fogs_ptr_ptr != nullptr)
                    {
                        if (world_fog_patched)
                        {
                            const bool world_reloaded =
                                (*tr_world_slot != saved_tr_world) ||
                                (!patched_map_had_native_fog
                                    ? (*fogs_ptr_ptr != custom_fogs.data() || *numfogs_ptr != 2)
                                    : (*fogs_ptr_ptr != saved_orig_fogs_ptr || *numfogs_ptr != saved_orig_numfogs));
                            if (world_reloaded)
                            {
                                world_fog_patched = false;
                            }
                        }

                        const int orig_numfogs = world_fog_patched ? saved_orig_numfogs : *numfogs_ptr;
                        const bool map_has_native_fog = (orig_numfogs > 1);
                        const bool apply_custom_fog =
                            fog_enabled && (!map_has_native_fog || fog_override_native);

                        if (apply_custom_fog)
                        {
                            if (!world_fog_patched)
                            {
                                saved_tr_world = *tr_world_slot;
                                saved_orig_fogs_ptr = *fogs_ptr_ptr;
                                saved_orig_numfogs = *numfogs_ptr;
                                patched_map_had_native_fog = map_has_native_fog;
                                saved_has_orig_fog1 = (map_has_native_fog &&
                                                       saved_orig_fogs_ptr != nullptr &&
                                                       saved_orig_fogs_ptr != custom_fogs.data());
                                if (saved_has_orig_fog1)
                                {
                                    saved_orig_fog1 = saved_orig_fogs_ptr[1];
                                }
                                world_fog_patched = true;
                            }

                            if (!map_has_native_fog)
                            {
                                *fogs_ptr_ptr = custom_fogs.data();
                                *numfogs_ptr = 2;
                            }
                            else if (*fogs_ptr_ptr != nullptr)
                            {
                                (*fogs_ptr_ptr)[1] = custom_fogs[1];
                            }

                            if (fog_num == 0)
                            {
                                const float sort = *reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(shader) + game::shader_off::sort);
                                const int fog_pass = *reinterpret_cast<const int*>(reinterpret_cast<uintptr_t>(shader) + game::shader_off::fog_pass);
                                if (fog_pass != 0 && sort > 2.5f && sort < 14.0f)
                                {
                                    fog_num = 1;
                                }
                            }
                        }
                        else if (world_fog_patched)
                        {
                            restore_native_world_fog();
                        }
                    }
                }
            }

            hook_rb_begin_surface.call(shader, fog_num);
        }

        int __cdecl hk_rb_surface_lightning_bolt()
        {
            const auto* current_ent =
                *game::exe_ptr<const uint8_t* const>(game::exe::addr_backend_current_entity);
            const auto* num_verts_ptr =
                game::exe_ptr<const int>(game::exe::addr_tess_num_vertexes);

            uint8_t beam_opacity = 255u;
            const bool tagged = (current_ent != nullptr) &&
                                unpack_lightning_opacity_tag(current_ent + game::re_off::shader_rgba, &beam_opacity);
            const int start_v = (num_verts_ptr != nullptr && *num_verts_ptr > 0) ? *num_verts_ptr : 0;

            const int ret = hook_rb_surface_lightning_bolt.call<int>();

            if (tagged && num_verts_ptr != nullptr && (beam_opacity < 255u || tess_has_lightning_opacity))
            {
                const int end_v = *num_verts_ptr;
                if (end_v > 0)
                {
                    if (!tess_has_lightning_opacity)
                    {
                        tess_vert_opacity.fill(255u);
                        tess_has_lightning_opacity = true;
                    }
                    const int fill_start = (start_v <= end_v) ? start_v : 0;
                    const int clamped_start = std::clamp(fill_start, 0, max_tess_opacity_verts);
                    const int clamped_end = std::clamp(end_v, 0, max_tess_opacity_verts);
                    for (int i = clamped_start; i < clamped_end; ++i)
                    {
                        tess_vert_opacity[static_cast<size_t>(i)] = beam_opacity;
                    }
                }
            }
            return ret;
        }

        void on_rb_iterate_post_colors(util::mid_context& ctx)
        {
            if (!tess_has_lightning_opacity)
            {
                return;
            }
            const int num_verts = *game::exe_ptr<const int>(game::exe::addr_tess_num_vertexes);
            auto* const colors = game::exe_ptr<uint8_t>(game::exe::addr_tess_svars_colors);
            const auto* stage = reinterpret_cast<const uint8_t*>(ctx.ebx);
            const uint32_t state_bits = (stage != nullptr)
                ? *reinterpret_cast<const uint32_t*>(stage + game::stage_off::state_bits)
                : 0u;
            apply_stage_vertex_opacity(colors, tess_vert_opacity.data(), num_verts, state_bits);
        }

        double __cdecl hk_fons_draw_text(
            void* fons,
            float x,
            float y,
            const char* str,
            const char* end,
            int max_chars,
            float* out_bounds,
            int force_color)
        {
            if (cvars::settings().force_white_names)
            {
                force_color = 1;
            }
            return hook_fons_draw_text.call<double>(
                fons, x, y, str, end, max_chars, out_bounds, force_color);
        }

        void on_fons_bounds_color(util::mid_context& ctx)
        {
            if (cvars::settings().force_white_names)
            {
                ctx.eip = game::exe_addr(game::exe::addr_fons_bounds_color_skip);
            }
        }

        void on_cg_add_weapon_with_powerups(util::mid_context& ctx)
        {
            if (!cvars::settings().hide_other_weapons)
            {
                return;
            }
            if (ctx.eax == 0 || active_cgame == nullptr)
            {
                return;
            }
            const auto* snap_ptr = *game::cgame_ptr<const uint8_t* const>(active_cgame, game::cgame::addr_cg_snap);
            if (snap_ptr == nullptr)
            {
                return;
            }
            const int snap_client_num = *reinterpret_cast<const int*>(snap_ptr + game::snap_off::client_num);
            const int cent_client_num = *reinterpret_cast<const int*>(ctx.eax + game::cent_off::client_num);
            if (cent_client_num != snap_client_num)
            {
                ctx.eax = 0;
                ctx.eip = game::cgame_addr(active_cgame, game::cgame::addr_add_weapon_skip_target);
            }
        }

        void on_cg_add_weapon_flash(util::mid_context& ctx)
        {
            if (active_cgame == nullptr || !cvars::settings().hide_other_weapons)
            {
                return;
            }
            const auto* cent = *reinterpret_cast<const uint8_t* const*>(ctx.esp + 0x24);
            const auto* snap_ptr = *game::cgame_ptr<const uint8_t* const>(active_cgame, game::cgame::addr_cg_snap);
            const int snap_client_num = (snap_ptr != nullptr) ? *reinterpret_cast<const int*>(snap_ptr + game::snap_off::client_num) : -1;
            const int cent_client_num = (cent != nullptr) ? *reinterpret_cast<const int*>(cent + game::cent_off::client_num) : -2;

            if (cent_client_num != snap_client_num)
            {
                ctx.eip = game::cgame_addr(active_cgame, game::cgame::addr_add_weapon_flash_skip_target);
            }
        }

        void on_cg_lightning_bolt_ref_entity(util::mid_context& ctx)
        {
            if (active_cgame == nullptr)
            {
                return;
            }
            const auto* cent = *reinterpret_cast<const uint8_t* const*>(ctx.esp + 0x120);
            auto* const re = reinterpret_cast<uint8_t*>(ctx.esp + 0x60);
            const auto* snap_ptr = *game::cgame_ptr<const uint8_t* const>(active_cgame, game::cgame::addr_cg_snap);
            const int local_client_num = (snap_ptr != nullptr) ? *reinterpret_cast<const int*>(snap_ptr + game::snap_off::client_num) : -1;
            const int shooter_client_num = (cent != nullptr) ? *reinterpret_cast<const int*>(cent + game::cent_off::number) : -2;

            const int snap_pers_team = (snap_ptr != nullptr) ? *reinterpret_cast<const int*>(snap_ptr + game::snap_off::pers_team) : 0;
            const int local_team = game::read_client_team(active_cgame, local_client_num, snap_pers_team);
            const int shooter_team = game::read_client_team(active_cgame, shooter_client_num, 0);
            const int gametype = *game::cgame_ptr<const int>(active_cgame, game::cgame::addr_cgs_gametype);

            const auto& cfg = cvars::settings();
            const bool is_team = classify_beam_is_team(
                gametype, local_client_num, local_team, shooter_client_num, shooter_team);
            const int style = clamp_lightning_style(
                is_team ? cfg.team_lightning_style : cfg.enemy_lightning_style);
            const uint8_t opacity_u8 = is_team ? cfg.team_lightning_opacity_u8 : cfg.enemy_lightning_opacity_u8;

            if (style == lightning_style_beamless || opacity_u8 == 0u)
            {
                // skip beam refEntity while letting wall impact spark and hum run
                ctx.eip = game::cgame_addr(active_cgame, game::cgame::addr_lightning_bolt_skip_target);
                return;
            }

            const auto* shaders = game::cgame_ptr<const int32_t>(active_cgame, game::cgame::addr_cgs_media_lightning);
            const int32_t chosen_shader = shaders[style - 1];
            if (chosen_shader != 0)
            {
                *reinterpret_cast<int32_t*>(re + game::re_off::custom_shader) = chosen_shader;
            }
            if (opacity_u8 < 255u)
            {
                pack_lightning_opacity_tag(re + game::re_off::shader_rgba, opacity_u8);
            }
        }

        const std::array<util::inline_hook_spec, 3> visual_engine_inline_specs{{
            { &hook_rb_begin_surface,          game::exe::addr_rb_begin_surface,     reinterpret_cast<void*>(&hk_rb_begin_surface) },
            { &hook_rb_surface_lightning_bolt, game::exe::addr_rb_surface_lightning, reinterpret_cast<void*>(&hk_rb_surface_lightning_bolt) },
            { &hook_fons_draw_text,            game::exe::addr_fons_draw_text,       reinterpret_cast<void*>(&hk_fons_draw_text) },
        }};

        const std::array<util::mid_hook_spec, 2> visual_engine_mid_specs{{
            { &hook_rb_iterate_post_colors, game::exe::addr_rb_iterate_post_colors, &on_rb_iterate_post_colors },
            { &hook_fons_bounds_color,      game::exe::addr_fons_bounds_color_site, &on_fons_bounds_color },
        }};

        const std::array<util::mid_hook_spec, 3> visual_cgame_mid_specs{{
            { &hook_cg_weapon_with_powerups, game::cgame::addr_add_weapon_with_powerups,  &on_cg_add_weapon_with_powerups },
            { &hook_cg_weapon_flash,         game::cgame::addr_add_weapon_flash,          &on_cg_add_weapon_flash },
            { &hook_cg_lightning_bolt,       game::cgame::addr_lightning_bolt_ref_entity, &on_cg_lightning_bolt_ref_entity },
        }};
    }

    bool install_engine_hooks()
    {
        if (!util::install_inline_table(&game::exe_addr, visual_engine_inline_specs))
        {
            return false;
        }
        if (!util::install_mid_table(&game::exe_addr, visual_engine_mid_specs))
        {
            util::release_hook_table(visual_engine_inline_specs);
            return false;
        }
        return true;
    }

    void release_engine_hooks()
    {
        tess_has_lightning_opacity = false;
        util::release_hook_table(visual_engine_mid_specs);
        restore_native_world_fog();
        util::release_hook_table(visual_engine_inline_specs);
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
        util::release_hook_table(visual_cgame_mid_specs);
        active_cgame = h_cgame;

        const auto resolve = [h_cgame](uintptr_t preferred_va)
        {
            return game::cgame_addr(h_cgame, preferred_va);
        };
        if (!util::install_mid_table(resolve, visual_cgame_mid_specs))
        {
            util::release_hook_table(visual_cgame_mid_specs);
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
        util::release_hook_table(visual_cgame_mid_specs);
        active_cgame = nullptr;
        cgame_attached = false;
    }

    void update_fog(
        bool enabled,
        int r,
        int g,
        int b,
        float distance,
        float intensity_scale,
        bool override_native)
    {
        fog_enabled = enabled;
        const int clamped_r = std::clamp(r, 0, 255);
        const int clamped_g = std::clamp(g, 0, 255);
        const int clamped_b = std::clamp(b, 0, 255);
        const float fog_distance = (distance < 10.0f) ? 10.0f : distance;
        const float fog_intensity = (intensity_scale < 0.05f) ? 0.05f : intensity_scale;
        fog_override_native = override_native;

        const float effective_depth = fog_distance / fog_intensity;
        const float clamped_depth = (effective_depth < 1.0f) ? 1.0f : effective_depth;

        const float rf = static_cast<float>(clamped_r) / 255.0f;
        const float gf = static_cast<float>(clamped_g) / 255.0f;
        const float bf = static_cast<float>(clamped_b) / 255.0f;

        std::memset(&custom_fogs[0], 0, sizeof(game::ql_fog));

        game::ql_fog& fog = custom_fogs[1];
        std::memset(&fog, 0, sizeof(game::ql_fog));
        fog.color[0] = rf;
        fog.color[1] = gf;
        fog.color[2] = bf;
        fog.depth_for_opaque = clamped_depth;
        fog.tc_scale = 1.0f / (clamped_depth * 8.0f);
        fog.color_int = static_cast<uint32_t>(clamped_r) |
                        (static_cast<uint32_t>(clamped_g) << 8) |
                        (static_cast<uint32_t>(clamped_b) << 16) |
                        0xFF000000u;
        fog.has_surface = 0; // volumetric around camera
    }

    void reset_session_state()
    {
        restore_native_world_fog();
        tess_has_lightning_opacity = false;
    }
}
