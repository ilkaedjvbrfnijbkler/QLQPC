#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace qlqpc::visuals
{
    inline constexpr int lightning_style_beamless = 0;
    inline constexpr int lightning_style_min = 0;
    inline constexpr int lightning_style_default = 1;
    inline constexpr int lightning_style_quakeworld = 2;
    inline constexpr int lightning_style_team_arena = 3;
    inline constexpr int lightning_style_thin_shaft = 4;
    inline constexpr int lightning_style_wide_beam = 5;
    inline constexpr int lightning_style_max = 5;

    inline constexpr int team_gametype_threshold = 3; // GT_TEAM and above
    inline constexpr int max_tess_opacity_verts = 2048;

    inline constexpr uint32_t gls_srcblend_mask = 0x0Fu;
    inline constexpr uint32_t gls_srcblend_src_alpha = 0x05u;

    inline constexpr uint8_t lightning_tag_b0 = 0x51u;
    inline constexpr uint8_t lightning_tag_b1 = 0x4Cu;
    inline constexpr uint8_t lightning_tag_b2 = 0xFEu;

    inline int clamp_lightning_style(int style)
    {
        if (style < lightning_style_min || style > lightning_style_max)
        {
            return lightning_style_default;
        }
        return style;
    }

    inline uint8_t opacity_float_to_u8(float opacity)
    {
        if (!std::isfinite(opacity))
        {
            return 255u;
        }
        const float clamped = std::clamp(opacity, 0.0f, 1.0f);
        const int scaled = static_cast<int>(std::lround(clamped * 255.0f));
        return static_cast<uint8_t>(std::clamp(scaled, 0, 255));
    }

    inline bool classify_beam_is_team(
        int gametype,
        int local_client_num,
        int local_team,
        int shooter_client_num,
        int shooter_team)
    {
        if (shooter_client_num == local_client_num && local_client_num >= 0)
        {
            return true;
        }
        if (gametype < team_gametype_threshold)
        {
            return false;
        }
        return local_team > 0 && shooter_team == local_team;
    }

    inline void pack_lightning_opacity_tag(uint8_t rgba_out[4], uint8_t opacity_u8)
    {
        rgba_out[0] = lightning_tag_b0;
        rgba_out[1] = lightning_tag_b1;
        rgba_out[2] = lightning_tag_b2;
        rgba_out[3] = opacity_u8;
    }

    inline bool unpack_lightning_opacity_tag(const uint8_t rgba[4], uint8_t* out_opacity)
    {
        if (rgba[0] == lightning_tag_b0 && rgba[1] == lightning_tag_b1 && rgba[2] == lightning_tag_b2)
        {
            if (out_opacity != nullptr)
            {
                *out_opacity = rgba[3];
            }
            return true;
        }
        return false;
    }

    inline void apply_stage_vertex_opacity(
        uint8_t* colors_rgba,
        const uint8_t* vert_opacity_u8,
        int num_vertexes,
        uint32_t stage_state_bits)
    {
        if (colors_rgba == nullptr || vert_opacity_u8 == nullptr || num_vertexes <= 0)
        {
            return;
        }
        const int count = (std::min)(num_vertexes, max_tess_opacity_verts);
        const bool uses_src_alpha = ((stage_state_bits & gls_srcblend_mask) == gls_srcblend_src_alpha);
        for (int i = 0; i < count; ++i)
        {
            const uint32_t op = vert_opacity_u8[i];
            if (op == 255u)
            {
                continue;
            }
            uint8_t* const px = colors_rgba + (i * 4);
            if (uses_src_alpha)
            {
                px[3] = static_cast<uint8_t>((static_cast<uint32_t>(px[3]) * op + 127u) / 255u);
            }
            else
            {
                px[0] = static_cast<uint8_t>((static_cast<uint32_t>(px[0]) * op + 127u) / 255u);
                px[1] = static_cast<uint8_t>((static_cast<uint32_t>(px[1]) * op + 127u) / 255u);
                px[2] = static_cast<uint8_t>((static_cast<uint32_t>(px[2]) * op + 127u) / 255u);
            }
        }
    }
}
