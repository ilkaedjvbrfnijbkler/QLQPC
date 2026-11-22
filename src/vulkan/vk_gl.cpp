#include "vulkan/vk_internal.hpp"

#include <emmintrin.h>

namespace qlqpc::vulkan
{
    namespace
    {
        template <typename index_t>
        bool compact_typed_indices(
            const index_t* src,
            uint32_t count,
            uint32_t max_vert_span,
            uint32_t* dst,
            uint32_t& out_min_idx,
            uint32_t& out_vert_count)
        {
            uint32_t min_idx = static_cast<uint32_t>(src[0]);
            uint32_t max_idx = min_idx;
            for (uint32_t i = 0; i < count; ++i)
            {
                const auto idx = static_cast<uint32_t>(src[i]);
                dst[i] = idx;
                if (idx < min_idx)
                {
                    min_idx = idx;
                }
                if (idx > max_idx)
                {
                    max_idx = idx;
                }
            }
            const uint64_t span = static_cast<uint64_t>(max_idx) - min_idx + 1ull;
            if (span > max_vert_span)
            {
                return false;
            }
            if (min_idx != 0u)
            {
                for (uint32_t i = 0; i < count; ++i)
                {
                    dst[i] -= min_idx;
                }
            }
            out_min_idx = min_idx;
            out_vert_count = static_cast<uint32_t>(span);
            return true;
        }

        uint64_t hash_index_slice(const uint32_t* src, uint32_t count)
        {
            uint64_t h = 14695981039346656037ull;
            for (uint32_t i = 0; i < count; ++i)
            {
                h ^= static_cast<uint64_t>(src[i]) + 0x9e3779b97f4a7c15ull;
                h *= 1099511628211ull;
            }
            return h;
        }
    }

    std::vector<mat4>& current_matrix_stack()
    {
        if (ctx.matrix_mode == GL_PROJECTION)
        {
            return ctx.proj_stack;
        }
        if (ctx.matrix_mode == GL_TEXTURE)
        {
            return ctx.tex_stack;
        }
        return ctx.mv_stack;
    }

    void set_state_flag(uint32_t cap, bool enabled)
    {
        switch (cap)
        {
            case GL_TEXTURE_2D:
                if (ctx.tex_2d_enabled[ctx.active_tex_unit] != enabled)
                {
                    ctx.tex_2d_enabled[ctx.active_tex_unit] = enabled;
                    ctx.pso_key_dirty = true;
                    ctx.tex_bindings_dirty = true;
                }
                break;
            case GL_TEXTURE_RECTANGLE_ARB:
                if (ctx.tex_rect_enabled[ctx.active_tex_unit] != enabled)
                {
                    ctx.tex_rect_enabled[ctx.active_tex_unit] = enabled;
                    ctx.pso_key_dirty = true;
                    ctx.tex_bindings_dirty = true;
                }
                break;
            case GL_CULL_FACE:
                if (ctx.cull_enabled != enabled)
                {
                    ctx.cull_enabled = enabled;
                    ctx.pso_key_dirty = true;
                }
                break;
            case GL_BLEND:
                if (ctx.blend_enabled != enabled)
                {
                    ctx.blend_enabled = enabled;
                    ctx.pso_key_dirty = true;
                }
                break;
            case GL_DEPTH_TEST:
                if (ctx.depth_test_enabled != enabled)
                {
                    ctx.depth_test_enabled = enabled;
                    ctx.pso_key_dirty = true;
                }
                break;
            case GL_ALPHA_TEST:
                if (ctx.alpha_test_enabled != enabled)
                {
                    ctx.alpha_test_enabled = enabled;
                    ctx.pso_key_dirty = true;
                }
                break;
            case GL_STENCIL_TEST:
                if (ctx.stencil_test_enabled != enabled)
                {
                    ctx.stencil_test_enabled = enabled;
                    ctx.pso_key_dirty = true;
                    if (enabled)
                    {
                        ctx.cmd_stencil_dirty = true;
                    }
                }
                break;
            case GL_SCISSOR_TEST:
                if (ctx.scissor_enabled != enabled)
                {
                    ctx.scissor_enabled = enabled;
                    ctx.cmd_sc_dirty = true;
                }
                break;
            case GL_POLYGON_OFFSET_FILL:
            case GL_POLYGON_OFFSET_LINE:
                if (ctx.poly_offset_enabled != enabled)
                {
                    ctx.poly_offset_enabled = enabled;
                    ctx.pso_key_dirty = true;
                    ctx.cmd_db_dirty = true;
                }
                break;
            case GL_CLIP_PLANE0:
                if (ctx.clip_plane_enabled != enabled)
                {
                    ctx.clip_plane_enabled = enabled;
                    ctx.pso_key_dirty = true;
                }
                break;
            default:
                break;
        }
    }

    void set_client_state_flag(uint32_t array, bool enabled)
    {
        if (array == GL_VERTEX_ARRAY)
        {
            ctx.va_vertex_enabled = enabled;
        }
        else if (array == GL_COLOR_ARRAY)
        {
            ctx.va_color_enabled = enabled;
        }
        else if (array == GL_TEXTURE_COORD_ARRAY)
        {
            ctx.va_texcoord_enabled[ctx.client_active_tex_unit] = enabled;
        }
    }

    void __stdcall vk_glAlphaFunc(uint32_t func, float)
    {
        if (ctx.alpha_test_func != func)
        {
            ctx.alpha_test_func = func;
            ctx.pso_key_dirty = true;
        }
    }

    void __stdcall vk_glArrayElement(int32_t i)
    {
        if (ctx.in_begin_end && i >= 0)
        {
            ctx.imm_vertices.push_back(fetch_client_vertex(static_cast<uint32_t>(i)));
        }
    }

    void __stdcall vk_glBegin(uint32_t mode)
    {
        ctx.in_begin_end = true;
        ctx.imm_prim_mode = mode;
        ctx.imm_vertices.clear();
    }

    void __stdcall vk_glEnd()
    {
        if (!ctx.in_begin_end)
        {
            return;
        }
        ctx.in_begin_end = false;
        const auto n = static_cast<uint32_t>(ctx.imm_vertices.size());
        if (n == 0)
        {
            return;
        }
        if (ctx.scratch_indices.size() < n)
        {
            ctx.scratch_indices.resize(n);
        }
        for (uint32_t i = 0; i < n; ++i)
        {
            ctx.scratch_indices[i] = i;
        }
        submit_vertices_and_indices(
            ctx.imm_prim_mode, ctx.imm_vertices.data(), n, ctx.scratch_indices.data(), n);
    }

    uint32_t get_bound_tex_for_target(uint32_t unit, uint32_t target)
    {
        return (target == GL_TEXTURE_RECTANGLE_ARB)
            ? ctx.bound_tex_rect[unit]
            : ctx.bound_tex_2d[unit];
    }

    void __stdcall vk_glBindTexture(uint32_t target, uint32_t texture)
    {
        const uint32_t u = ctx.active_tex_unit;
        if (target == GL_TEXTURE_RECTANGLE_ARB)
        {
            if (ctx.bound_tex_rect[u] != texture)
            {
                ctx.bound_tex_rect[u] = texture;
                ctx.tex_bindings_dirty = true;
                ctx.pso_key_dirty = true;
            }
        }
        else
        {
            if (ctx.bound_tex_2d[u] != texture)
            {
                ctx.bound_tex_2d[u] = texture;
                ctx.tex_bindings_dirty = true;
                ctx.pso_key_dirty = true;
            }
        }
        if (texture != 0)
        {
            auto& tex = get_or_create_texture(texture);
            const bool was_rectangle = tex.is_rectangle;
            tex.target = target;
            tex.is_rectangle = (target == GL_TEXTURE_RECTANGLE_ARB);
            if (tex.is_rectangle && (!was_rectangle || tex.image == vk_null_handle))
            {
                tex.min_filter = GL_LINEAR;
                tex.mag_filter = GL_LINEAR;
                tex.wrap_s = GL_CLAMP_TO_EDGE;
                tex.wrap_t = GL_CLAMP_TO_EDGE;
                ctx.tex_bindings_dirty = true;
            }
        }
    }

    void __stdcall vk_glBlendFunc(uint32_t sfactor, uint32_t dfactor)
    {
        if (ctx.blend_src != sfactor || ctx.blend_dst != dfactor)
        {
            ctx.blend_src = sfactor;
            ctx.blend_dst = dfactor;
            ctx.pso_key_dirty = true;
        }
    }

    void __stdcall vk_glClear(uint32_t mask)
    {
        if (mask == 0 || !ctx.context_created)
        {
            return;
        }

        // prefer render pass load_op_clear over inline attachment clears on full-target clears
        if (try_begin_render_pass_with_clear(mask))
        {
            return;
        }

        ensure_render_pass_active();
        if (!ctx.render_pass_active)
        {
            return;
        }

        VkClearAttachment atts[2]{};
        uint32_t att_count = 0;
        if ((mask & GL_COLOR_BUFFER_BIT) != 0)
        {
            atts[att_count].aspectMask = 1u;
            atts[att_count].colorAttachment = 0u;
            atts[att_count].clearValue.color.float32[0] = ctx.clear_color[0];
            atts[att_count].clearValue.color.float32[1] = ctx.clear_color[1];
            atts[att_count].clearValue.color.float32[2] = ctx.clear_color[2];
            atts[att_count].clearValue.color.float32[3] = ctx.clear_color[3];
            ++att_count;
        }
        VkFlags ds_aspect = 0u;
        if ((mask & GL_DEPTH_BUFFER_BIT) != 0)
        {
            ds_aspect |= 2u;
            if (!ctx.stencil_test_enabled)
            {
                ds_aspect |= 4u;
            }
        }
        if ((mask & GL_STENCIL_BUFFER_BIT) != 0)
        {
            ds_aspect |= 4u;
        }
        if (ds_aspect != 0u)
        {
            atts[att_count].aspectMask = ds_aspect;
            atts[att_count].colorAttachment = 0u;
            atts[att_count].clearValue.depthStencil.depth = ctx.clear_depth;
            atts[att_count].clearValue.depthStencil.stencil = ctx.clear_stencil;
            ++att_count;
        }
        if (att_count == 0)
        {
            return;
        }

        VkClearRect rect{};
        rect.rect = compute_vk_scissor_rect();
        if (rect.rect.extent.width == 0u || rect.rect.extent.height == 0u)
        {
            return;
        }
        rect.baseArrayLayer = 0u;
        rect.layerCount = 1u;
        ctx.vk.vkCmdClearAttachments(ctx.active_cmd, att_count, atts, 1u, &rect);
    }

    void __stdcall vk_glClearColor(float r, float g, float b, float a)
    {
        ctx.clear_color[0] = r;
        ctx.clear_color[1] = g;
        ctx.clear_color[2] = b;
        ctx.clear_color[3] = a;
    }

    void __stdcall vk_glClearDepth(double depth)
    {
        ctx.clear_depth = static_cast<float>(std::clamp(depth, 0.0, 1.0));
    }

    void __stdcall vk_glClearStencil(int32_t s)
    {
        ctx.clear_stencil = static_cast<uint32_t>(s & 0xFF);
    }

    void __stdcall vk_glClipPlane(uint32_t plane, const double* eq)
    {
        if (plane != GL_CLIP_PLANE0 || eq == nullptr)
        {
            return;
        }
        const mat4& mv = ctx.mv_stack.back();
        float inv_mv[16]{};
        if (!invert_mat4(mv.m, inv_mv))
        {
            return;
        }
        for (int c = 0; c < 4; ++c)
        {
            ctx.clip_plane_eye[c] = static_cast<float>(
                eq[0] * inv_mv[c * 4 + 0] +
                eq[1] * inv_mv[c * 4 + 1] +
                eq[2] * inv_mv[c * 4 + 2] +
                eq[3] * inv_mv[c * 4 + 3]);
        }
    }

    void __stdcall vk_glColor3f(float r, float g, float b)
    {
        ctx.cur_color[0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
        ctx.cur_color[1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
        ctx.cur_color[2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
        ctx.cur_color[3] = 255u;
    }

    void __stdcall vk_glColor4f(float r, float g, float b, float a)
    {
        ctx.cur_color[0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
        ctx.cur_color[1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
        ctx.cur_color[2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
        ctx.cur_color[3] = static_cast<uint8_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
    }

    void __stdcall vk_glColor4fv(const float* v)
    {
        if (v != nullptr)
        {
            vk_glColor4f(v[0], v[1], v[2], v[3]);
        }
    }

    void __stdcall vk_glColor4ubv(const uint8_t* v)
    {
        if (v != nullptr)
        {
            ctx.cur_color[0] = v[0];
            ctx.cur_color[1] = v[1];
            ctx.cur_color[2] = v[2];
            ctx.cur_color[3] = v[3];
        }
    }

    void __stdcall vk_glColorMask(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        const uint8_t mask =
            (r ? 0x01u : 0u) |
            (g ? 0x02u : 0u) |
            (b ? 0x04u : 0u) |
            (a ? 0x08u : 0u);
        if (ctx.color_write_mask != mask)
        {
            ctx.color_write_mask = mask;
            ctx.pso_key_dirty = true;
        }
    }

    void __stdcall vk_glColorPointer(int32_t size, uint32_t type, int32_t stride, const void* ptr)
    {
        ctx.va_color_size = size;
        ctx.va_color_type = type;
        ctx.va_color_stride = stride;
        ctx.va_color_ptr = ptr;
    }

    void __stdcall vk_glCopyTexSubImage2D(
        uint32_t target,
        int32_t level,
        int32_t xoffset,
        int32_t yoffset,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height)
    {
        if (width <= 0 || height <= 0 || !ctx.context_created)
        {
            return;
        }
        const uint32_t tid = get_bound_tex_for_target(ctx.active_tex_unit, target);
        vk_texture_object* const tex = find_texture(tid);
        if (tex == nullptr || tex->image == vk_null_handle)
        {
            return;
        }

        end_active_render_pass();
        begin_frame_if_needed();
        if (ctx.cur_color_img == vk_null_handle || ctx.active_cmd == nullptr || tex->image == ctx.cur_color_img)
        {
            return;
        }

        const uint32_t dst_level = std::min(
            static_cast<uint32_t>(std::max(0, level)),
            std::max(1u, tex->mip_levels) - 1u);
        const int64_t fb_w = static_cast<int64_t>(ctx.cur_fb_w);
        const int64_t fb_h = static_cast<int64_t>(ctx.cur_fb_h);
        const int dst_w = static_cast<int>(std::max(1u, tex->width >> dst_level));
        const int dst_h = static_cast<int>(std::max(1u, tex->height >> dst_level));

        const int64_t skip_i = std::max<int64_t>(
            {
                0,
                -static_cast<int64_t>(x),
                -static_cast<int64_t>(xoffset),
            });
        const int64_t skip_j = std::max<int64_t>(
            {
                0,
                -static_cast<int64_t>(y),
                -static_cast<int64_t>(yoffset),
            });
        const int64_t end_i = std::min<int64_t>(
            {
                static_cast<int64_t>(width),
                fb_w - static_cast<int64_t>(x),
                static_cast<int64_t>(dst_w) - static_cast<int64_t>(xoffset),
            });
        const int64_t end_j = std::min<int64_t>(
            {
                static_cast<int64_t>(height),
                fb_h - static_cast<int64_t>(y),
                static_cast<int64_t>(dst_h) - static_cast<int64_t>(yoffset),
            });
        if (end_i <= skip_i || end_j <= skip_j)
        {
            return;
        }

        const int copy_w = static_cast<int>(end_i - skip_i);
        const int copy_h = static_cast<int>(end_j - skip_j);
        const int src_x = static_cast<int>(static_cast<int64_t>(x) + skip_i);
        const int src_gl_y0 = static_cast<int>(static_cast<int64_t>(y) + skip_j);
        const int src_y_top = static_cast<int>(fb_h) - (src_gl_y0 + copy_h);
        const int dst_x = static_cast<int>(static_cast<int64_t>(xoffset) + skip_i);
        const int dst_y = static_cast<int>(static_cast<int64_t>(yoffset) + skip_j);

        VkImageBlit blit{};
        blit.srcSubresource = {1u, 0u, 0u, 1u};
        if (tex->is_rectangle)
        {
            const int dst_y_top = std::clamp(dst_h - (dst_y + copy_h), 0, std::max(0, dst_h - copy_h));
            blit.srcOffsets[0] = {src_x, src_y_top, 0};
            blit.srcOffsets[1] = {src_x + copy_w, src_y_top + copy_h, 1};
            blit.dstSubresource = {1u, dst_level, 0u, 1u};
            blit.dstOffsets[0] = {dst_x, dst_y_top, 0};
            blit.dstOffsets[1] = {dst_x + copy_w, dst_y_top + copy_h, 1};
        }
        else
        {
            blit.srcOffsets[0] = {src_x, src_y_top + copy_h, 0};
            blit.srcOffsets[1] = {src_x + copy_w, src_y_top, 1};
            blit.dstSubresource = {1u, dst_level, 0u, 1u};
            blit.dstOffsets[0] = {dst_x, dst_y, 0};
            blit.dstOffsets[1] = {dst_x + copy_w, dst_y + copy_h, 1};
        }

        const VkImageLayout src_rest_layout =
            (ctx.bound_fbo_id == 0) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
        cmd_transition_image(
            ctx.active_cmd, ctx.cur_color_img,
            src_rest_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1u, 1u, 0u);
        if (tex->current_layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            cmd_transition_image(
                ctx.active_cmd, tex->image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1u, tex->mip_levels, 0u);
            tex->current_layout = VK_IMAGE_LAYOUT_GENERAL;
        }
        cmd_transition_image(
            ctx.active_cmd, tex->image,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, 1u, dst_level);
        ctx.vk.vkCmdBlitImage(
            ctx.active_cmd,
            ctx.cur_color_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1u, &blit, VK_FILTER_NEAREST);
        cmd_transition_image(
            ctx.active_cmd, ctx.cur_color_img,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src_rest_layout, 1u, 1u, 0u);
        cmd_transition_image(
            ctx.active_cmd, tex->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 1u, 1u, dst_level);
    }

    void __stdcall vk_glCullFace(uint32_t mode)
    {
        if (ctx.cull_face_mode != mode)
        {
            ctx.cull_face_mode = mode;
            ctx.pso_key_dirty = true;
        }
    }

    void __stdcall vk_glFrontFace(uint32_t mode)
    {
        if (ctx.front_face_mode != mode)
        {
            ctx.front_face_mode = mode;
            ctx.pso_key_dirty = true;
        }
    }

    void __stdcall vk_glDrawArrays(uint32_t mode, int32_t first, int32_t count)
    {
        if (count <= 0 || first < 0 || !ctx.va_vertex_enabled || ctx.va_vertex_ptr == nullptr)
        {
            return;
        }
        const auto u_first = static_cast<uint32_t>(first);
        const auto u_count = static_cast<uint32_t>(count);
        if (u_count > 65536u)
        {
            return;
        }
        if (ctx.scratch_verts.size() < u_count)
        {
            ctx.scratch_verts.resize(u_count);
        }
        if (ctx.scratch_indices.size() < u_count)
        {
            ctx.scratch_indices.resize(u_count);
        }
        pack_client_vertices_range(ctx.scratch_verts.data(), u_first, u_count);
        for (uint32_t i = 0; i < u_count; ++i)
        {
            ctx.scratch_indices[i] = i;
        }
        submit_vertices_and_indices(
            mode, ctx.scratch_verts.data(), u_count, ctx.scratch_indices.data(), u_count);
    }

    void __stdcall vk_glDeleteTextures(int32_t n, const uint32_t* textures)
    {
        if (n <= 0 || textures == nullptr || !ctx.context_created)
        {
            return;
        }
        flush_pending_commands_and_wait();
        for (int32_t i = 0; i < n; ++i)
        {
            const uint32_t tid = textures[i];
            if (tid == 0)
            {
                continue;
            }
            for (int u = 0; u < 2; ++u)
            {
                if (ctx.bound_tex_2d[u] == tid)
                {
                    ctx.bound_tex_2d[u] = 0;
                }
                if (ctx.bound_tex_rect[u] == tid)
                {
                    ctx.bound_tex_rect[u] = 0;
                }
            }
            vk_texture_object* const tex = find_texture(tid);
            if (tex != nullptr)
            {
                destroy_vk_image_resources(tex->view, tex->image, tex->memory);
                *tex = vk_texture_object{};
            }
            for (auto& [fid, fbo] : ctx.fbos)
            {
                if (fbo.color_tex_id == tid)
                {
                    fbo.color_tex_id = 0u;
                    if (fbo.fb != vk_null_handle)
                    {
                        ctx.vk.vkDestroyFramebuffer(ctx.device, fbo.fb, nullptr);
                        fbo.fb = vk_null_handle;
                    }
                    if (ctx.bound_fbo_id == fid)
                    {
                        ctx.cur_vk_fb = vk_null_handle;
                        ctx.cur_color_img = vk_null_handle;
                    }
                }
            }
        }
        reset_all_descriptor_caches();
        ctx.pso_key_dirty = true;
    }

    void __stdcall vk_glDepthFunc(uint32_t func)
    {
        if (ctx.depth_func != func)
        {
            ctx.depth_func = func;
            ctx.pso_key_dirty = true;
        }
    }

    void __stdcall vk_glDepthMask(uint8_t flag)
    {
        const bool en = (flag != 0);
        if (ctx.depth_write_enabled != en)
        {
            ctx.depth_write_enabled = en;
            ctx.pso_key_dirty = true;
        }
    }

    void __stdcall vk_glDepthRange(double z_near, double z_far)
    {
        const float n = static_cast<float>(std::clamp(z_near, 0.0, 1.0));
        const float f = static_cast<float>(std::clamp(z_far, 0.0, 1.0));
        if (ctx.depth_range[0] != n || ctx.depth_range[1] != f)
        {
            ctx.depth_range[0] = n;
            ctx.depth_range[1] = f;
            ctx.cmd_vp_dirty = true;
        }
    }

    void __stdcall vk_glDisable(uint32_t cap)
    {
        set_state_flag(cap, false);
    }

    void __stdcall vk_glDisableClientState(uint32_t array)
    {
        set_client_state_flag(array, false);
    }

    void __stdcall vk_glDrawBuffer(uint32_t)
    {
    }

    void __stdcall vk_glDrawElements(
        uint32_t mode,
        int32_t count,
        uint32_t type,
        const void* indices)
    {
        if (count <= 0 || indices == nullptr || !ctx.va_vertex_enabled || ctx.va_vertex_ptr == nullptr ||
            !ctx.context_created)
        {
            return;
        }

        const auto u_count = static_cast<uint32_t>(count);

        // fast path for indexed triangle draws using compiled vertex array bounds and cached ibo slices
        if (mode == GL_TRIANGLES && type == GL_UNSIGNED_INT)
        {
            const auto* src = static_cast<const uint32_t*>(indices);
            const uint64_t idx_hash = ctx.cva_locked ? hash_index_slice(src, u_count) : 0ull;

            if (ctx.cva_locked && ctx.cva_ibo_valid &&
                indices == ctx.cva_cached_idx_ptr &&
                u_count == ctx.cva_cached_idx_count &&
                src[0] == ctx.cva_cached_idx_first_val &&
                src[u_count - 1u] == ctx.cva_cached_idx_last_val &&
                idx_hash == ctx.cva_cached_idx_hash)
            {
                const uint32_t min_idx = ctx.cva_cached_min_idx;
                const uint32_t v_count = ctx.cva_cached_v_count;
                const VkDeviceSize v_bytes = static_cast<VkDeviceSize>(v_count) * sizeof(gl_vk_vertex);
                if (ctx.vbo_offset + v_bytes <= vulkan_context::vbo_size)
                {
                    ensure_render_pass_active();
                    if (!ctx.render_pass_active)
                    {
                        return;
                    }

                    const size_t fi = ctx.cur_frame;
                    const VkDeviceSize v_off = ctx.vbo_offset;
                    pack_client_vertices_range(
                        reinterpret_cast<gl_vk_vertex*>(ctx.vbo_mapped[fi] + v_off),
                        min_idx, v_count);
                    ctx.vbo_offset += v_bytes;
                    emit_draw_indexed(
                        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                        v_off, ctx.cva_cached_ibo_off, u_count);
                    return;
                }
            }

            uint32_t min_idx = 0;
            uint32_t v_count = 0;
            if (ctx.cva_locked && ctx.cva_lock_count > 0u)
            {
                min_idx = ctx.cva_lock_first;
                v_count = ctx.cva_lock_count;
            }
            else
            {
                min_idx = src[0];
                uint32_t max_idx = min_idx;
                uint32_t i = 1u;
                if (u_count >= 8u)
                {
                    const __m128i sign_bias = _mm_set1_epi32(static_cast<int>(0x80000000u));
                    __m128i vmin_b = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i*>(src)), sign_bias);
                    __m128i vmax_b = vmin_b;
                    for (i = 4u; i + 4u <= u_count; i += 4u)
                    {
                        const __m128i v = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i)), sign_bias);
                        const __m128i lt = _mm_cmplt_epi32(v, vmin_b);
                        const __m128i gt = _mm_cmpgt_epi32(v, vmax_b);
                        vmin_b = _mm_or_si128(_mm_and_si128(lt, v), _mm_andnot_si128(lt, vmin_b));
                        vmax_b = _mm_or_si128(_mm_and_si128(gt, v), _mm_andnot_si128(gt, vmax_b));
                    }
                    alignas(16) uint32_t mins[4];
                    alignas(16) uint32_t maxs[4];
                    _mm_store_si128(reinterpret_cast<__m128i*>(mins), _mm_xor_si128(vmin_b, sign_bias));
                    _mm_store_si128(reinterpret_cast<__m128i*>(maxs), _mm_xor_si128(vmax_b, sign_bias));
                    min_idx = std::min({mins[0], mins[1], mins[2], mins[3]});
                    max_idx = std::max({maxs[0], maxs[1], maxs[2], maxs[3]});
                }
                for (; i < u_count; ++i)
                {
                    const uint32_t idx = src[i];
                    if (idx < min_idx)
                    {
                        min_idx = idx;
                    }
                    if (idx > max_idx)
                    {
                        max_idx = idx;
                    }
                }
                const uint64_t span = static_cast<uint64_t>(max_idx) - min_idx + 1ull;
                if (span == 0ull || span > 65536ull)
                {
                    return;
                }
                v_count = static_cast<uint32_t>(span);
            }
            const VkDeviceSize v_bytes = static_cast<VkDeviceSize>(v_count) * sizeof(gl_vk_vertex);
            const VkDeviceSize i_bytes = static_cast<VkDeviceSize>(u_count) * sizeof(uint32_t);
            if (v_bytes > vulkan_context::vbo_size || i_bytes > vulkan_context::ibo_size)
            {
                return;
            }
            if (ctx.vbo_offset + v_bytes > vulkan_context::vbo_size ||
                ctx.ibo_offset + i_bytes > vulkan_context::ibo_size)
            {
                flush_pending_commands_and_wait();
            }

            ensure_render_pass_active();
            if (!ctx.render_pass_active)
            {
                return;
            }

            const size_t fi = ctx.cur_frame;
            const VkDeviceSize v_off = ctx.vbo_offset;
            const VkDeviceSize i_off = ctx.ibo_offset;

            pack_client_vertices_range(
                reinterpret_cast<gl_vk_vertex*>(ctx.vbo_mapped[fi] + v_off),
                min_idx, v_count);

            auto* dst_idx = reinterpret_cast<uint32_t*>(ctx.ibo_mapped[fi] + i_off);
            if (min_idx == 0u)
            {
                std::memcpy(dst_idx, src, static_cast<size_t>(i_bytes));
            }
            else
            {
                const __m128i v_min = _mm_set1_epi32(static_cast<int>(min_idx));
                uint32_t i = 0;
                for (; i + 4u <= u_count; i += 4u)
                {
                    const __m128i v_src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
                    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst_idx + i), _mm_sub_epi32(v_src, v_min));
                }
                for (; i < u_count; ++i)
                {
                    dst_idx[i] = src[i] - min_idx;
                }
            }

            ctx.vbo_offset += v_bytes;
            ctx.ibo_offset += (i_bytes + 15ull) & ~15ull;

            if (ctx.cva_locked)
            {
                ctx.cva_ibo_valid = true;
                ctx.cva_cached_idx_ptr = indices;
                ctx.cva_cached_idx_count = u_count;
                ctx.cva_cached_idx_first_val = src[0];
                ctx.cva_cached_idx_last_val = src[u_count - 1u];
                ctx.cva_cached_idx_hash = idx_hash;
                ctx.cva_cached_min_idx = min_idx;
                ctx.cva_cached_v_count = v_count;
                ctx.cva_cached_ibo_off = i_off;
            }

            emit_draw_indexed(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, v_off, i_off, u_count);
            return;
        }

        if (ctx.scratch_indices.size() < u_count)
        {
            ctx.scratch_indices.resize(u_count);
        }

        uint32_t min_idx = 0;
        uint32_t v_count = 0;
        bool ok = false;
        if (type == GL_UNSIGNED_INT)
        {
            ok = compact_typed_indices(
                static_cast<const uint32_t*>(indices), u_count, 65536u,
                ctx.scratch_indices.data(), min_idx, v_count);
        }
        else if (type == GL_UNSIGNED_SHORT)
        {
            ok = compact_typed_indices(
                static_cast<const uint16_t*>(indices), u_count, 65536u,
                ctx.scratch_indices.data(), min_idx, v_count);
        }
        else if (type == GL_UNSIGNED_BYTE)
        {
            ok = compact_typed_indices(
                static_cast<const uint8_t*>(indices), u_count, 65536u,
                ctx.scratch_indices.data(), min_idx, v_count);
        }
        else
        {
            return;
        }

        if (!ok || v_count == 0u)
        {
            return;
        }
        if (ctx.scratch_verts.size() < v_count)
        {
            ctx.scratch_verts.resize(v_count);
        }
        pack_client_vertices_range(ctx.scratch_verts.data(), min_idx, v_count);
        submit_vertices_and_indices(
            mode, ctx.scratch_verts.data(), v_count, ctx.scratch_indices.data(), u_count);
    }

    void __stdcall vk_glEnable(uint32_t cap)
    {
        set_state_flag(cap, true);
    }

    void __stdcall vk_glEnableClientState(uint32_t array)
    {
        set_client_state_flag(array, true);
    }

    void __stdcall vk_glFinish()
    {
        flush_pending_commands_and_wait();
    }

    void __stdcall vk_glFlush()
    {
    }

    void __stdcall vk_glFrustum(
        double left,
        double right,
        double bottom,
        double top,
        double z_near,
        double z_far)
    {
        if (z_near <= 0.0 || z_far <= z_near || left == right || bottom == top)
        {
            return;
        }
        mat4 f{};
        f.m[0] = static_cast<float>((2.0 * z_near) / (right - left));
        f.m[5] = static_cast<float>((2.0 * z_near) / (top - bottom));
        f.m[8] = static_cast<float>((right + left) / (right - left));
        f.m[9] = static_cast<float>((top + bottom) / (top - bottom));
        f.m[10] = static_cast<float>(-(z_far + z_near) / (z_far - z_near));
        f.m[11] = -1.0f;
        f.m[14] = static_cast<float>(-(2.0 * z_far * z_near) / (z_far - z_near));
        auto& st = current_matrix_stack();
        st.back() = mat4::multiply(st.back(), f);
        if (ctx.matrix_mode != GL_TEXTURE)
        {
            ctx.mvp_dirty = true;
        }
    }

    void __stdcall vk_glGenTextures(int32_t n, uint32_t* textures)
    {
        if (n <= 0 || textures == nullptr)
        {
            return;
        }
        for (int32_t i = 0; i < n; ++i)
        {
            const uint32_t id = ctx.next_tex_id++;
            textures[i] = id;
            get_or_create_texture(id);
        }
    }

    void __stdcall vk_glGetBooleanv(uint32_t pname, uint8_t* params)
    {
        if (params == nullptr)
        {
            return;
        }
        if (pname == GL_DEPTH_WRITEMASK)
        {
            *params = ctx.depth_write_enabled ? 1u : 0u;
        }
        else if (pname == GL_DEPTH_TEST)
        {
            *params = ctx.depth_test_enabled ? 1u : 0u;
        }
        else
        {
            *params = 0u;
        }
    }

    uint32_t __stdcall vk_glGetError()
    {
        return GL_NO_ERROR;
    }

    void __stdcall vk_glGetIntegerv(uint32_t pname, int32_t* params)
    {
        if (params == nullptr)
        {
            return;
        }
        switch (pname)
        {
            case GL_DEPTH_FUNC:
                *params = static_cast<int32_t>(ctx.depth_func);
                break;
            case GL_MAX_TEXTURE_SIZE:
            case GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB:
                *params = 16384;
                break;
            case GL_MAX_TEXTURE_UNITS_ARB:
                *params = 8;
                break;
            case GL_UNPACK_ALIGNMENT:
                *params = ctx.unpack_alignment;
                break;
            case GL_PACK_ALIGNMENT:
                *params = ctx.pack_alignment;
                break;
            case GL_UNPACK_ROW_LENGTH:
                *params = ctx.unpack_row_length;
                break;
            case GL_UNPACK_SKIP_PIXELS:
                *params = ctx.unpack_skip_pixels;
                break;
            case GL_UNPACK_SKIP_ROWS:
                *params = ctx.unpack_skip_rows;
                break;
            default:
                *params = 0;
                break;
        }
    }

    const uint8_t* __stdcall vk_glGetString(uint32_t name)
    {
        switch (name)
        {
            case GL_VENDOR:
                return reinterpret_cast<const uint8_t*>(ctx.gl_vendor_str.c_str());
            case GL_RENDERER:
                return reinterpret_cast<const uint8_t*>(ctx.gl_renderer_str.c_str());
            case GL_VERSION:
                return reinterpret_cast<const uint8_t*>(ctx.gl_version_str.c_str());
            case GL_EXTENSIONS:
                return reinterpret_cast<const uint8_t*>(ctx.gl_extensions_str.c_str());
            default:
                return reinterpret_cast<const uint8_t*>("");
        }
    }

    void __stdcall vk_glHint(uint32_t, uint32_t)
    {
    }

    uint8_t __stdcall vk_glIsEnabled(uint32_t cap)
    {
        switch (cap)
        {
            case GL_TEXTURE_2D:
                return ctx.tex_2d_enabled[ctx.active_tex_unit] ? 1u : 0u;
            case GL_TEXTURE_RECTANGLE_ARB:
                return ctx.tex_rect_enabled[ctx.active_tex_unit] ? 1u : 0u;
            case GL_DEPTH_TEST:
                return ctx.depth_test_enabled ? 1u : 0u;
            case GL_CULL_FACE:
                return ctx.cull_enabled ? 1u : 0u;
            case GL_BLEND:
                return ctx.blend_enabled ? 1u : 0u;
            case GL_ALPHA_TEST:
                return ctx.alpha_test_enabled ? 1u : 0u;
            case GL_STENCIL_TEST:
                return ctx.stencil_test_enabled ? 1u : 0u;
            case GL_SCISSOR_TEST:
                return ctx.scissor_enabled ? 1u : 0u;
            case GL_POLYGON_OFFSET_FILL:
            case GL_POLYGON_OFFSET_LINE:
                return ctx.poly_offset_enabled ? 1u : 0u;
            case GL_CLIP_PLANE0:
                return ctx.clip_plane_enabled ? 1u : 0u;
            default:
                return 0u;
        }
    }

    void __stdcall vk_glLineWidth(float)
    {
    }

    void __stdcall vk_glLoadIdentity()
    {
        current_matrix_stack().back() = mat4::identity();
        if (ctx.matrix_mode != GL_TEXTURE)
        {
            ctx.mvp_dirty = true;
        }
    }

    void __stdcall vk_glLoadMatrixf(const float* m)
    {
        if (m != nullptr)
        {
            std::memcpy(current_matrix_stack().back().m, m, 16 * sizeof(float));
            if (ctx.matrix_mode != GL_TEXTURE)
            {
                ctx.mvp_dirty = true;
            }
        }
    }

    void __stdcall vk_glMatrixMode(uint32_t mode)
    {
        ctx.matrix_mode = mode;
    }

    void __stdcall vk_glMultMatrixf(const float* m)
    {
        if (m != nullptr)
        {
            mat4 rhs{};
            std::memcpy(rhs.m, m, 16 * sizeof(float));
            auto& st = current_matrix_stack();
            st.back() = mat4::multiply(st.back(), rhs);
            if (ctx.matrix_mode != GL_TEXTURE)
            {
                ctx.mvp_dirty = true;
            }
        }
    }

    void __stdcall vk_glOrtho(
        double left,
        double right,
        double bottom,
        double top,
        double z_near,
        double z_far)
    {
        if (left == right || bottom == top || z_near == z_far)
        {
            return;
        }
        mat4 o = mat4::identity();
        o.m[0] = static_cast<float>(2.0 / (right - left));
        o.m[5] = static_cast<float>(2.0 / (top - bottom));
        o.m[10] = static_cast<float>(-2.0 / (z_far - z_near));
        o.m[12] = static_cast<float>(-(right + left) / (right - left));
        o.m[13] = static_cast<float>(-(top + bottom) / (top - bottom));
        o.m[14] = static_cast<float>(-(z_far + z_near) / (z_far - z_near));
        auto& st = current_matrix_stack();
        st.back() = mat4::multiply(st.back(), o);
        if (ctx.matrix_mode != GL_TEXTURE)
        {
            ctx.mvp_dirty = true;
        }
    }

    void __stdcall vk_glPixelStorei(uint32_t pname, int32_t param)
    {
        switch (pname)
        {
            case GL_UNPACK_ALIGNMENT:
                if (param == 1 || param == 2 || param == 4 || param == 8)
                {
                    ctx.unpack_alignment = param;
                }
                break;
            case GL_UNPACK_ROW_LENGTH:
                ctx.unpack_row_length = param;
                break;
            case GL_UNPACK_SKIP_PIXELS:
                ctx.unpack_skip_pixels = param;
                break;
            case GL_UNPACK_SKIP_ROWS:
                ctx.unpack_skip_rows = param;
                break;
            case GL_PACK_ALIGNMENT:
                if (param == 1 || param == 2 || param == 4 || param == 8)
                {
                    ctx.pack_alignment = param;
                }
                break;
            default:
                break;
        }
    }

    void __stdcall vk_glPolygonMode(uint32_t, uint32_t mode)
    {
        if (ctx.polygon_mode != mode)
        {
            ctx.polygon_mode = mode;
            ctx.pso_key_dirty = true;
        }
    }

    void __stdcall vk_glPolygonOffset(float factor, float units)
    {
        if (ctx.poly_offset_factor != factor || ctx.poly_offset_units != units)
        {
            ctx.poly_offset_factor = factor;
            ctx.poly_offset_units = units;
            if (ctx.poly_offset_enabled)
            {
                ctx.cmd_db_dirty = true;
            }
        }
    }

    void __stdcall vk_glPushClientAttrib(uint32_t)
    {
        ctx.client_attrib_stack.push_back({
            ctx.unpack_alignment,
            ctx.unpack_row_length,
            ctx.unpack_skip_pixels,
            ctx.unpack_skip_rows,
            ctx.pack_alignment,
        });
    }

    void __stdcall vk_glPopClientAttrib()
    {
        if (!ctx.client_attrib_stack.empty())
        {
            const auto b = ctx.client_attrib_stack.back();
            ctx.client_attrib_stack.pop_back();
            ctx.unpack_alignment = b.alignment;
            ctx.unpack_row_length = b.row_length;
            ctx.unpack_skip_pixels = b.skip_pixels;
            ctx.unpack_skip_rows = b.skip_rows;
            ctx.pack_alignment = b.pack_alignment;
        }
    }

    void __stdcall vk_glPushMatrix()
    {
        auto& st = current_matrix_stack();
        st.push_back(st.back());
    }

    void __stdcall vk_glPopMatrix()
    {
        auto& st = current_matrix_stack();
        if (st.size() > 1)
        {
            st.pop_back();
            if (ctx.matrix_mode != GL_TEXTURE)
            {
                ctx.mvp_dirty = true;
            }
        }
    }

    void __stdcall vk_glReadBuffer(uint32_t)
    {
    }

    void __stdcall vk_glReadPixels(
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height,
        uint32_t format,
        uint32_t type,
        void* pixels)
    {
        if (width <= 0 || height <= 0 || pixels == nullptr || !ctx.context_created)
        {
            return;
        }
        if (format == GL_DEPTH_COMPONENT && type == GL_FLOAT)
        {
            auto* dst_f = static_cast<float*>(pixels);
            const size_t count = static_cast<size_t>(width) * static_cast<size_t>(height);
            for (size_t i = 0; i < count; ++i)
            {
                dst_f[i] = 1.0f;
            }
            return;
        }
        if (format == GL_STENCIL_INDEX && type == GL_UNSIGNED_BYTE)
        {
            std::memset(pixels, 0, static_cast<size_t>(width) * static_cast<size_t>(height));
            return;
        }
        if ((format != GL_RGB &&
             format != GL_RGBA &&
             format != GL_BGR_EXT &&
             format != GL_BGRA_EXT) ||
            type != GL_UNSIGNED_BYTE ||
            ctx.cur_color_img == vk_null_handle ||
            ctx.staging_mapped[0] == nullptr ||
            ctx.cur_fb_w == 0u ||
            ctx.cur_fb_h == 0u)
        {
            return;
        }
        flush_pending_commands_and_wait();

        const int64_t fb_w = static_cast<int64_t>(ctx.cur_fb_w);
        const int64_t fb_h = static_cast<int64_t>(ctx.cur_fb_h);
        const int64_t gl_x0 = std::max<int64_t>(0, x);
        const int64_t gl_y0 = std::max<int64_t>(0, y);
        const int64_t gl_x1 = std::min<int64_t>(fb_w, static_cast<int64_t>(x) + width);
        const int64_t gl_y1 = std::min<int64_t>(fb_h, static_cast<int64_t>(y) + height);
        if (gl_x1 <= gl_x0 || gl_y1 <= gl_y0)
        {
            return;
        }

        const uint32_t copy_w = static_cast<uint32_t>(gl_x1 - gl_x0);
        const uint32_t copy_h = static_cast<uint32_t>(gl_y1 - gl_y0);
        if (static_cast<size_t>(copy_w) * copy_h * 4u > vulkan_context::staging_size)
        {
            return;
        }

        const int top_y = static_cast<int>(fb_h - gl_y1);
        const VkImageLayout src_rest_layout =
            (ctx.bound_fbo_id == 0) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
        execute_immediate_cmd([&](VkCommandBuffer cmd)
        {
            cmd_transition_image(
                cmd, ctx.cur_color_img,
                src_rest_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1u, 1u, 0u);
            VkBufferImageCopy bic{};
            bic.bufferOffset = 0;
            bic.imageSubresource = {1u, 0u, 0u, 1u};
            bic.imageOffset = {static_cast<int32_t>(gl_x0), top_y, 0};
            bic.imageExtent = {copy_w, copy_h, 1u};
            ctx.vk.vkCmdCopyImageToBuffer(
                cmd, ctx.cur_color_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                ctx.staging_bufs[0], 1u, &bic);
            cmd_transition_image(
                cmd, ctx.cur_color_img,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src_rest_layout, 1u, 1u, 0u);
        });

        const size_t dst_bpp = (format == GL_RGB || format == GL_BGR_EXT) ? 3u : 4u;
        const size_t dst_skip_x = static_cast<size_t>(gl_x0 - x);
        const size_t dst_skip_y = static_cast<size_t>(gl_y0 - y);
        const size_t pack_align = static_cast<size_t>(std::max(1, ctx.pack_alignment));
        const size_t raw_dst_stride = static_cast<size_t>(width) * dst_bpp;
        const size_t dst_stride = (raw_dst_stride + pack_align - 1u) & ~(pack_align - 1u);
        auto* dst = static_cast<uint8_t*>(pixels);
        for (uint32_t row = 0; row < copy_h; ++row)
        {
            const uint8_t* src_row = ctx.staging_mapped[0] + static_cast<size_t>(copy_h - 1u - row) * copy_w * 4u;
            uint8_t* dst_row = dst + (dst_skip_y + row) * dst_stride + dst_skip_x * dst_bpp;
            for (uint32_t col = 0; col < copy_w; ++col)
            {
                const uint8_t r = src_row[col * 4 + 0];
                const uint8_t g = src_row[col * 4 + 1];
                const uint8_t b = src_row[col * 4 + 2];
                const uint8_t a = src_row[col * 4 + 3];
                if (format == GL_BGR_EXT)
                {
                    dst_row[col * 3 + 0] = b;
                    dst_row[col * 3 + 1] = g;
                    dst_row[col * 3 + 2] = r;
                }
                else if (format == GL_RGB)
                {
                    dst_row[col * 3 + 0] = r;
                    dst_row[col * 3 + 1] = g;
                    dst_row[col * 3 + 2] = b;
                }
                else if (format == GL_BGRA_EXT)
                {
                    dst_row[col * 4 + 0] = b;
                    dst_row[col * 4 + 1] = g;
                    dst_row[col * 4 + 2] = r;
                    dst_row[col * 4 + 3] = a;
                }
                else
                {
                    dst_row[col * 4 + 0] = r;
                    dst_row[col * 4 + 1] = g;
                    dst_row[col * 4 + 2] = b;
                    dst_row[col * 4 + 3] = a;
                }
            }
        }
    }

    void __stdcall vk_glRotatef(float angle, float x, float y, float z)
    {
        const float len = std::sqrt(x * x + y * y + z * z);
        if (len < 1e-6f)
        {
            return;
        }
        x /= len;
        y /= len;
        z /= len;
        const float rad = angle * 0.017453292519943295f;
        const float c = std::cos(rad);
        const float s = std::sin(rad);
        const float ic = 1.0f - c;

        mat4 r = mat4::identity();
        r.m[0] = x * x * ic + c;
        r.m[1] = y * x * ic + z * s;
        r.m[2] = x * z * ic - y * s;
        r.m[4] = x * y * ic - z * s;
        r.m[5] = y * y * ic + c;
        r.m[6] = y * z * ic + x * s;
        r.m[8] = x * z * ic + y * s;
        r.m[9] = y * z * ic - x * s;
        r.m[10] = z * z * ic + c;
        auto& st = current_matrix_stack();
        st.back() = mat4::multiply(st.back(), r);
        if (ctx.matrix_mode != GL_TEXTURE)
        {
            ctx.mvp_dirty = true;
        }
    }

    void __stdcall vk_glScalef(float x, float y, float z)
    {
        mat4 s = mat4::identity();
        s.m[0] = x;
        s.m[5] = y;
        s.m[10] = z;
        auto& st = current_matrix_stack();
        st.back() = mat4::multiply(st.back(), s);
        if (ctx.matrix_mode != GL_TEXTURE)
        {
            ctx.mvp_dirty = true;
        }
    }

    void __stdcall vk_glScissor(int32_t x, int32_t y, int32_t w, int32_t h)
    {
        if (ctx.scissor[0] != x || ctx.scissor[1] != y || ctx.scissor[2] != w || ctx.scissor[3] != h)
        {
            ctx.scissor[0] = x;
            ctx.scissor[1] = y;
            ctx.scissor[2] = w;
            ctx.scissor[3] = h;
            if (ctx.scissor_enabled)
            {
                ctx.cmd_sc_dirty = true;
            }
        }
    }

    void __stdcall vk_glShadeModel(uint32_t)
    {
    }

    void __stdcall vk_glStencilFunc(uint32_t func, int32_t ref, uint32_t mask)
    {
        if (ctx.stencil_func != func)
        {
            ctx.stencil_func = func;
            if (ctx.stencil_test_enabled)
            {
                ctx.pso_key_dirty = true;
            }
        }
        if (ctx.stencil_ref != ref || ctx.stencil_val_mask != mask)
        {
            ctx.stencil_ref = ref;
            ctx.stencil_val_mask = mask;
            if (ctx.stencil_test_enabled)
            {
                ctx.cmd_stencil_dirty = true;
            }
        }
    }

    void __stdcall vk_glStencilMask(uint32_t mask)
    {
        if (ctx.stencil_write_mask != mask)
        {
            ctx.stencil_write_mask = mask;
            if (ctx.stencil_test_enabled)
            {
                ctx.cmd_stencil_dirty = true;
            }
        }
    }

    void __stdcall vk_glStencilOp(uint32_t fail, uint32_t zfail, uint32_t zpass)
    {
        if (ctx.stencil_fail_op != fail || ctx.stencil_zfail_op != zfail || ctx.stencil_zpass_op != zpass)
        {
            ctx.stencil_fail_op = fail;
            ctx.stencil_zfail_op = zfail;
            ctx.stencil_zpass_op = zpass;
            if (ctx.stencil_test_enabled)
            {
                ctx.pso_key_dirty = true;
            }
        }
    }

    void __stdcall vk_glTexCoord2f(float s, float t)
    {
        ctx.cur_uv[0][0] = s;
        ctx.cur_uv[0][1] = t;
    }

    void __stdcall vk_glTexCoord2fv(const float* v)
    {
        if (v != nullptr)
        {
            ctx.cur_uv[0][0] = v[0];
            ctx.cur_uv[0][1] = v[1];
        }
    }

    void __stdcall vk_glTexCoordPointer(int32_t size, uint32_t, int32_t stride, const void* ptr)
    {
        const uint32_t u = ctx.client_active_tex_unit;
        ctx.va_tc_size[u] = size;
        ctx.va_tc_stride[u] = stride;
        ctx.va_tc_ptr[u] = ptr;
    }

    void __stdcall vk_glTexEnvf(uint32_t target, uint32_t pname, float param)
    {
        if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
        {
            const auto mode = static_cast<uint32_t>(param);
            if (ctx.tex_env_mode[ctx.active_tex_unit] != mode)
            {
                ctx.tex_env_mode[ctx.active_tex_unit] = mode;
                ctx.pso_key_dirty = true;
            }
        }
    }

    void __stdcall vk_glTexEnvi(uint32_t target, uint32_t pname, int32_t param)
    {
        if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
        {
            const auto mode = static_cast<uint32_t>(param);
            if (ctx.tex_env_mode[ctx.active_tex_unit] != mode)
            {
                ctx.tex_env_mode[ctx.active_tex_unit] = mode;
                ctx.pso_key_dirty = true;
            }
        }
    }

    void __stdcall vk_glTexImage2D(
        uint32_t target,
        int32_t level,
        int32_t,
        int32_t width,
        int32_t height,
        int32_t,
        uint32_t format,
        uint32_t type,
        const void* pixels)
    {
        if (width <= 0 || height <= 0 || level < 0 || !ctx.context_created)
        {
            return;
        }

        const uint32_t tid = get_bound_tex_for_target(ctx.active_tex_unit, target);
        if (tid == 0)
        {
            return;
        }
        auto& tex = get_or_create_texture(tid);
        tex.id = tid;
        tex.target = target;
        tex.is_rectangle = (target == GL_TEXTURE_RECTANGLE_ARB);

        const uint32_t w = static_cast<uint32_t>(width);
        const uint32_t h = static_cast<uint32_t>(height);
        uint32_t expected_mips = 1u;
        if (!tex.is_rectangle)
        {
            uint32_t dim = std::max(w, h);
            while (dim > 1u)
            {
                dim >>= 1u;
                ++expected_mips;
            }
        }

        if (level == 0 && (tex.image == vk_null_handle || tex.width != w || tex.height != h || tex.mip_levels != expected_mips))
        {
            flush_pending_commands_and_wait();
            destroy_vk_image_resources(tex.view, tex.image, tex.memory);
            tex.width = w;
            tex.height = h;
            tex.max_uploaded_level = 0u;
            tex.mip_levels = expected_mips;
            tex.current_layout = VK_IMAGE_LAYOUT_UNDEFINED;

            // only request color attachment usage when the texture actually backs an fbo
            const VkFlags usage = tex.is_fbo_attachment
                ? (0x01u | 0x02u | 0x04u | 0x10u)
                : (0x01u | 0x02u | 0x04u);
            if (create_image_2d(
                    w, h, expected_mips, VK_FORMAT_R8G8B8A8_UNORM,
                    usage, 1u,
                    tex.image, tex.memory, tex.view))
            {
                if (pixels == nullptr || tex.is_fbo_attachment || type != GL_UNSIGNED_BYTE)
                {
                    execute_immediate_cmd([&](VkCommandBuffer cmd)
                    {
                        clear_color_image(cmd, tex.image, expected_mips);
                    });
                    tex.current_layout = VK_IMAGE_LAYOUT_GENERAL;
                }
            }
            rebuild_texture_sampler(tex);
            reset_all_descriptor_caches();
            ctx.tex_bindings_dirty = true;

            if (tex.is_fbo_attachment)
            {
                for (auto& [fid, fbo] : ctx.fbos)
                {
                    if (fbo.color_tex_id == tid)
                    {
                        rebuild_offscreen_fbo(fbo);
                        if (ctx.bound_fbo_id == fid)
                        {
                            vk_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fid);
                            ctx.cmd_vp_dirty = true;
                            ctx.cmd_sc_dirty = true;
                        }
                    }
                }
            }
        }

        if (tex.image != vk_null_handle && pixels != nullptr)
        {
            upload_texture_level(tex, static_cast<uint32_t>(level), 0, 0, w, h, format, type, pixels);
        }
    }

    void __stdcall vk_glTexSubImage2D(
        uint32_t target,
        int32_t level,
        int32_t xoffset,
        int32_t yoffset,
        int32_t width,
        int32_t height,
        uint32_t format,
        uint32_t type,
        const void* pixels)
    {
        if (width <= 0 || height <= 0 || level < 0 || !ctx.context_created)
        {
            return;
        }
        const uint32_t tid = get_bound_tex_for_target(ctx.active_tex_unit, target);
        vk_texture_object* tex = find_texture(tid);
        if (tex == nullptr || tex->image == vk_null_handle)
        {
            return;
        }
        upload_texture_level(
            *tex, static_cast<uint32_t>(level),
            xoffset, yoffset,
            static_cast<uint32_t>(width), static_cast<uint32_t>(height),
            format, type, pixels);
    }

    void __stdcall vk_glTexParameterf(uint32_t target, uint32_t pname, float param)
    {
        const uint32_t tid = get_bound_tex_for_target(ctx.active_tex_unit, target);
        if (tid == 0)
        {
            return;
        }
        auto& tex = get_or_create_texture(tid);
        const auto val = static_cast<uint32_t>(param);
        bool changed = false;
        if (pname == GL_TEXTURE_MIN_FILTER)
        {
            if (tex.min_filter != val)
            {
                tex.min_filter = val;
                changed = true;
            }
        }
        else if (pname == GL_TEXTURE_MAG_FILTER)
        {
            if (tex.mag_filter != val)
            {
                tex.mag_filter = val;
                changed = true;
            }
        }
        else if (pname == GL_TEXTURE_WRAP_S)
        {
            if (tex.wrap_s != val)
            {
                tex.wrap_s = val;
                changed = true;
            }
        }
        else if (pname == GL_TEXTURE_WRAP_T)
        {
            if (tex.wrap_t != val)
            {
                tex.wrap_t = val;
                changed = true;
            }
        }
        if (changed && ctx.context_created && tex.image != vk_null_handle)
        {
            const VkSampler old_s = tex.sampler;
            rebuild_texture_sampler(tex);
            if (tex.sampler != old_s)
            {
                ctx.tex_bindings_dirty = true;
            }
        }
    }

    void __stdcall vk_glTexParameteri(uint32_t target, uint32_t pname, int32_t param)
    {
        vk_glTexParameterf(target, pname, static_cast<float>(param));
    }

    void __stdcall vk_glTranslatef(float x, float y, float z)
    {
        mat4 t = mat4::identity();
        t.m[12] = x;
        t.m[13] = y;
        t.m[14] = z;
        auto& st = current_matrix_stack();
        st.back() = mat4::multiply(st.back(), t);
        if (ctx.matrix_mode != GL_TEXTURE)
        {
            ctx.mvp_dirty = true;
        }
    }

    void __stdcall vk_glVertex2f(float x, float y)
    {
        if (!ctx.in_begin_end)
        {
            return;
        }
        gl_vk_vertex v{};
        v.pos[0] = x;
        v.pos[1] = y;
        v.pos[2] = 0.0f;
        std::memcpy(v.color, ctx.cur_color, 4);
        v.uv0[0] = ctx.cur_uv[0][0];
        v.uv0[1] = ctx.cur_uv[0][1];
        v.uv1[0] = ctx.cur_uv[1][0];
        v.uv1[1] = ctx.cur_uv[1][1];
        ctx.imm_vertices.push_back(v);
    }

    void __stdcall vk_glVertex3f(float x, float y, float z)
    {
        if (!ctx.in_begin_end)
        {
            return;
        }
        gl_vk_vertex v{};
        v.pos[0] = x;
        v.pos[1] = y;
        v.pos[2] = z;
        std::memcpy(v.color, ctx.cur_color, 4);
        v.uv0[0] = ctx.cur_uv[0][0];
        v.uv0[1] = ctx.cur_uv[0][1];
        v.uv1[0] = ctx.cur_uv[1][0];
        v.uv1[1] = ctx.cur_uv[1][1];
        ctx.imm_vertices.push_back(v);
    }

    void __stdcall vk_glVertex3fv(const float* v)
    {
        if (v != nullptr)
        {
            vk_glVertex3f(v[0], v[1], v[2]);
        }
    }

    void __stdcall vk_glVertexPointer(int32_t size, uint32_t, int32_t stride, const void* ptr)
    {
        ctx.va_vertex_size = size;
        ctx.va_vertex_stride = stride;
        ctx.va_vertex_ptr = ptr;
    }

    void __stdcall vk_glViewport(int32_t x, int32_t y, int32_t w, int32_t h)
    {
        if (ctx.viewport[0] != x || ctx.viewport[1] != y || ctx.viewport[2] != w || ctx.viewport[3] != h)
        {
            ctx.viewport[0] = x;
            ctx.viewport[1] = y;
            ctx.viewport[2] = w;
            ctx.viewport[3] = h;
            ctx.cmd_vp_dirty = true;
            if (!ctx.scissor_enabled)
            {
                ctx.cmd_sc_dirty = true;
            }
        }
    }

    void __stdcall vk_glActiveTextureARB(uint32_t texture)
    {
        if (texture >= GL_TEXTURE0_ARB && texture < GL_TEXTURE0_ARB + 2u)
        {
            ctx.active_tex_unit = texture - GL_TEXTURE0_ARB;
        }
    }

    void __stdcall vk_glClientActiveTextureARB(uint32_t texture)
    {
        if (texture >= GL_TEXTURE0_ARB && texture < GL_TEXTURE0_ARB + 2u)
        {
            ctx.client_active_tex_unit = texture - GL_TEXTURE0_ARB;
        }
    }

    void __stdcall vk_glMultiTexCoord2fARB(uint32_t target, float s, float t)
    {
        if (target >= GL_TEXTURE0_ARB && target < GL_TEXTURE0_ARB + 2u)
        {
            const uint32_t u = target - GL_TEXTURE0_ARB;
            ctx.cur_uv[u][0] = s;
            ctx.cur_uv[u][1] = t;
        }
    }

    void __stdcall vk_glLockArraysEXT(int32_t first, int32_t count)
    {
        if (first >= 0 && count > 0 && static_cast<uint32_t>(count) <= 65536u)
        {
            ctx.cva_locked = true;
            ctx.cva_lock_first = static_cast<uint32_t>(first);
            ctx.cva_lock_count = static_cast<uint32_t>(count);
        }
        else
        {
            ctx.cva_locked = false;
            ctx.cva_lock_first = 0;
            ctx.cva_lock_count = 0;
        }
        ctx.cva_ibo_valid = false;
    }

    void __stdcall vk_glUnlockArraysEXT()
    {
        ctx.cva_locked = false;
        ctx.cva_lock_first = 0;
        ctx.cva_lock_count = 0;
        ctx.cva_ibo_valid = false;
    }

    int __stdcall vk_wglGetDeviceGammaRamp3DFX(HDC, void* ramp)
    {
        if (ramp == nullptr)
        {
            return 0;
        }
        std::memcpy(ramp, ctx.gamma_ramp, sizeof(ctx.gamma_ramp));
        return 1;
    }

    int __stdcall vk_wglSetDeviceGammaRamp3DFX(HDC, const void* ramp)
    {
        if (ramp == nullptr)
        {
            return 0;
        }
        std::memcpy(ctx.gamma_ramp, ramp, sizeof(ctx.gamma_ramp));

        bool is_id = true;
        uint8_t lut_px[256 * 4]{};
        for (int i = 0; i < 256; ++i)
        {
            const uint8_t r = static_cast<uint8_t>(ctx.gamma_ramp[0][i] >> 8);
            const uint8_t g = static_cast<uint8_t>(ctx.gamma_ramp[1][i] >> 8);
            const uint8_t b = static_cast<uint8_t>(ctx.gamma_ramp[2][i] >> 8);
            if (r != i || g != i || b != i)
            {
                is_id = false;
            }
            lut_px[i * 4 + 0] = r;
            lut_px[i * 4 + 1] = g;
            lut_px[i * 4 + 2] = b;
            lut_px[i * 4 + 3] = 255u;
        }
        ctx.gamma_identity = is_id;
        if (ctx.context_created && ctx.gamma_lut_tex.image != vk_null_handle)
        {
            upload_texture_level(ctx.gamma_lut_tex, 0u, 0, 0, 256u, 1u, GL_RGBA, GL_UNSIGNED_BYTE, lut_px);
        }
        return 1;
    }

    int __stdcall vk_wglSwapIntervalEXT(int interval)
    {
        const int clamped = (interval > 0) ? 1 : 0;
        if (ctx.swap_interval != clamped)
        {
            ctx.swap_interval = clamped;
            if (ctx.context_created && ctx.surface != vk_null_handle)
            {
                flush_pending_commands_and_wait();
                create_or_recreate_swapchain();
            }
        }
        return 1;
    }

    void __stdcall vk_glGenQueriesARB(int32_t n, uint32_t* ids)
    {
        if (n <= 0 || ids == nullptr)
        {
            return;
        }
        for (int32_t i = 0; i < n; ++i)
        {
            ids[i] = ctx.next_query_id++;
        }
    }

    void __stdcall vk_glDeleteQueriesARB(int32_t, const uint32_t*)
    {
    }

    uint8_t __stdcall vk_glIsQueryARB(uint32_t id)
    {
        return (id != 0) ? 1u : 0u;
    }

    void __stdcall vk_glBeginQueryARB(uint32_t, uint32_t)
    {
    }

    void __stdcall vk_glEndQueryARB(uint32_t)
    {
    }

    void __stdcall vk_glGetQueryivARB(uint32_t, uint32_t, int32_t* params)
    {
        if (params != nullptr)
        {
            *params = 1;
        }
    }

    void __stdcall vk_glGetQueryObjectivARB(uint32_t, uint32_t pname, int32_t* params)
    {
        if (params != nullptr)
        {
            *params = (pname == GL_QUERY_RESULT_AVAILABLE) ? 1 : 64;
        }
    }

    void __stdcall vk_glGetQueryObjectuivARB(uint32_t, uint32_t pname, uint32_t* params)
    {
        if (params != nullptr)
        {
            *params = (pname == GL_QUERY_RESULT_AVAILABLE) ? 1u : 64u;
        }
    }

    uint32_t __stdcall vk_glCreateProgramObjectARB()
    {
        const uint32_t h = ctx.next_glsl_handle++;
        auto& obj = ctx.glsl_objects[h];
        obj.handle = h;
        obj.is_program = true;
        return h;
    }

    uint32_t __stdcall vk_glCreateShaderObjectARB(uint32_t shader_type)
    {
        const uint32_t h = ctx.next_glsl_handle++;
        auto& obj = ctx.glsl_objects[h];
        obj.handle = h;
        obj.shader_type = shader_type;
        obj.is_program = false;
        return h;
    }

    void __stdcall vk_glDeleteObjectARB(uint32_t obj)
    {
        ctx.glsl_objects.erase(obj);
        if (ctx.active_program == obj)
        {
            ctx.active_program = 0;
            ctx.pso_key_dirty = true;
            ctx.tex_bindings_dirty = true;
        }
    }

    void __stdcall vk_glShaderSourceARB(
        uint32_t shader_obj,
        int32_t count,
        const char** string,
        const int32_t* length)
    {
        if (count <= 0 || string == nullptr)
        {
            return;
        }
        std::string src{};
        for (int32_t i = 0; i < count; ++i)
        {
            if (string[i] == nullptr)
            {
                continue;
            }
            if (length != nullptr && length[i] >= 0)
            {
                src.append(string[i], static_cast<size_t>(length[i]));
            }
            else
            {
                src.append(string[i]);
            }
        }
        auto& obj = ctx.glsl_objects[shader_obj];
        if (obj.shader_type == GL_VERTEX_SHADER_ARB || src.find("gl_Position") != std::string::npos)
        {
            obj.kind = post_process_kind::none;
            obj.is_blur_vertical = false;
            return;
        }
        if (src.find("p_brightthreshold") != std::string::npos)
        {
            obj.kind = post_process_kind::bright_pass;
        }
        else if (src.find("p_bloomintensity") != std::string::npos)
        {
            obj.kind = post_process_kind::combine;
        }
        else if (src.find("0.042557") != std::string::npos || src.find("0.179335") != std::string::npos ||
                 src.find("0.0545") != std::string::npos ||
                 src.find("texCoord.x") != std::string::npos || src.find("texCoord.y") != std::string::npos)
        {
            obj.kind = post_process_kind::blur;
            obj.is_blur_vertical = (src.find("texCoord.y") != std::string::npos ||
                                    src.find("vec2(0,") != std::string::npos ||
                                    src.find("vec2(0.0,") != std::string::npos ||
                                    src.find("vec2( 0") != std::string::npos);
        }
        else if (src.find("p_gammaRecip") != std::string::npos)
        {
            obj.kind = post_process_kind::color_correct;
        }
        else if (src.find("gl_TexCoord[1]") != std::string::npos)
        {
            obj.kind = post_process_kind::downsample;
        }
    }

    void __stdcall vk_glCompileShaderARB(uint32_t)
    {
    }

    void __stdcall vk_glAttachObjectARB(uint32_t container_obj, uint32_t obj)
    {
        auto it_c = ctx.glsl_objects.find(container_obj);
        auto it_s = ctx.glsl_objects.find(obj);
        if (it_c != ctx.glsl_objects.end() && it_s != ctx.glsl_objects.end())
        {
            if (it_s->second.shader_type != GL_VERTEX_SHADER_ARB && it_s->second.kind != post_process_kind::none)
            {
                it_c->second.kind = it_s->second.kind;
                it_c->second.is_blur_vertical = it_s->second.is_blur_vertical;
            }
        }
    }

    void __stdcall vk_glDetachObjectARB(uint32_t, uint32_t)
    {
    }

    void __stdcall vk_glLinkProgramARB(uint32_t)
    {
    }

    void __stdcall vk_glUseProgramObjectARB(uint32_t program_obj)
    {
        if (ctx.active_program != program_obj)
        {
            ctx.active_program = program_obj;
            ctx.pso_key_dirty = true;
            ctx.tex_bindings_dirty = true;
        }
    }

    void __stdcall vk_glValidateProgramARB(uint32_t)
    {
    }

    void __stdcall vk_glGetObjectParameterivARB(uint32_t, uint32_t pname, int32_t* params)
    {
        if (params == nullptr)
        {
            return;
        }
        if (pname == GL_INFO_LOG_LENGTH)
        {
            *params = 0;
        }
        else
        {
            *params = 1;
        }
    }

    void __stdcall vk_glGetInfoLogARB(
        uint32_t,
        int32_t max_length,
        int32_t* length,
        char* info_log)
    {
        if (length != nullptr)
        {
            *length = 0;
        }
        if (info_log != nullptr && max_length > 0)
        {
            info_log[0] = '\0';
        }
    }

    int32_t __stdcall vk_glGetUniformLocationARB(uint32_t, const char* name)
    {
        if (name == nullptr)
        {
            return -1;
        }
        const std::string_view sv(name);
        if (sv == "scene" || sv == "backBufferTex")
        {
            return 1;
        }
        if (sv == "bloom" || sv == "bloomTex")
        {
            return 2;
        }
        if (sv == "p_gammaRecip")
        {
            return 10;
        }
        if (sv == "p_overbright")
        {
            return 11;
        }
        if (sv == "p_contrast")
        {
            return 12;
        }
        if (sv == "p_brightthreshold")
        {
            return 13;
        }
        if (sv == "p_bloomsaturation")
        {
            return 14;
        }
        if (sv == "p_scenesaturation")
        {
            return 15;
        }
        if (sv == "p_bloomintensity")
        {
            return 16;
        }
        if (sv == "p_sceneintensity")
        {
            return 17;
        }
        return -1;
    }

    void __stdcall vk_glUniform1iARB(int32_t, int32_t)
    {
    }

    void __stdcall vk_glUniform1fARB(int32_t location, float v0)
    {
        if (ctx.active_program == 0)
        {
            return;
        }
        auto it = ctx.glsl_objects.find(ctx.active_program);
        if (it == ctx.glsl_objects.end())
        {
            return;
        }
        switch (location)
        {
            case 10:
                it->second.gamma_recip = v0;
                break;
            case 11:
                it->second.overbright = v0;
                break;
            case 12:
                it->second.contrast = v0;
                break;
            case 13:
                it->second.bright_threshold = v0;
                break;
            case 14:
                it->second.bloom_saturation = v0;
                break;
            case 15:
                it->second.scene_saturation = v0;
                break;
            case 16:
                it->second.bloom_intensity = v0;
                break;
            case 17:
                it->second.scene_intensity = v0;
                break;
            default:
                break;
        }
    }

    uint8_t __stdcall vk_glIsRenderbufferEXT(uint32_t renderbuffer)
    {
        return ctx.rbos.contains(renderbuffer) ? 1u : 0u;
    }

    void __stdcall vk_glBindRenderbufferEXT(uint32_t, uint32_t renderbuffer)
    {
        ctx.bound_rbo_id = renderbuffer;
        if (renderbuffer != 0)
        {
            ctx.rbos[renderbuffer].id = renderbuffer;
        }
    }

    void __stdcall vk_glDeleteRenderbuffersEXT(int32_t n, const uint32_t* renderbuffers)
    {
        if (n <= 0 || renderbuffers == nullptr || !ctx.context_created)
        {
            return;
        }
        flush_pending_commands_and_wait();
        for (int32_t i = 0; i < n; ++i)
        {
            const uint32_t rbo_id = renderbuffers[i];
            if (rbo_id == 0u)
            {
                continue;
            }
            if (ctx.bound_rbo_id == rbo_id)
            {
                ctx.bound_rbo_id = 0u;
            }
            auto it = ctx.rbos.find(rbo_id);
            if (it != ctx.rbos.end())
            {
                destroy_vk_image_resources(it->second.view, it->second.image, it->second.memory);
                ctx.rbos.erase(it);
            }
            for (auto& [fid, fbo] : ctx.fbos)
            {
                if (fbo.depth_rb_id == rbo_id)
                {
                    fbo.depth_rb_id = 0u;
                    rebuild_offscreen_fbo(fbo);
                    if (ctx.bound_fbo_id == fid)
                    {
                        vk_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fid);
                    }
                }
            }
        }
    }

    void __stdcall vk_glGenRenderbuffersEXT(int32_t n, uint32_t* renderbuffers)
    {
        if (n <= 0 || renderbuffers == nullptr)
        {
            return;
        }
        for (int32_t i = 0; i < n; ++i)
        {
            const uint32_t id = ctx.next_rbo_id++;
            renderbuffers[i] = id;
            ctx.rbos[id].id = id;
        }
    }

    void __stdcall vk_glRenderbufferStorageEXT(
        uint32_t,
        uint32_t,
        int32_t width,
        int32_t height)
    {
        if (ctx.bound_rbo_id == 0 || width <= 0 || height <= 0 || !ctx.context_created)
        {
            return;
        }
        auto& rbo = ctx.rbos[ctx.bound_rbo_id];
        const auto w = static_cast<uint32_t>(width);
        const auto h = static_cast<uint32_t>(height);
        if (rbo.image != vk_null_handle && rbo.width == w && rbo.height == h)
        {
            return;
        }

        flush_pending_commands_and_wait();
        destroy_vk_image_resources(rbo.view, rbo.image, rbo.memory);
        rbo.width = w;
        rbo.height = h;
        if (create_image_2d(
                w, h, 1u, ctx.depth_stencil_format,
                0x02u | 0x20u, 0x06u,
                rbo.image, rbo.memory, rbo.view))
        {
            execute_immediate_cmd([&](VkCommandBuffer cmd)
            {
                clear_depth_stencil_image(cmd, rbo.image);
            });
        }
        else
        {
            rbo.width = 0u;
            rbo.height = 0u;
        }

        for (auto& [fid, fbo] : ctx.fbos)
        {
            if (fbo.depth_rb_id == ctx.bound_rbo_id)
            {
                rebuild_offscreen_fbo(fbo);
                if (ctx.bound_fbo_id == fid)
                {
                    vk_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fid);
                    ctx.cmd_vp_dirty = true;
                    ctx.cmd_sc_dirty = true;
                }
            }
        }
    }

    void __stdcall vk_glGetRenderbufferParameterivEXT(
        uint32_t,
        uint32_t,
        int32_t* params)
    {
        if (params != nullptr)
        {
            *params = 0;
        }
    }

    uint8_t __stdcall vk_glIsFramebufferEXT(uint32_t framebuffer)
    {
        return ctx.fbos.contains(framebuffer) ? 1u : 0u;
    }

    void rebuild_offscreen_fbo(vk_framebuffer_object& fbo)
    {
        if (!ctx.context_created)
        {
            return;
        }
        if (fbo.fb != vk_null_handle)
        {
            flush_pending_commands_and_wait();
            ctx.vk.vkDestroyFramebuffer(ctx.device, fbo.fb, nullptr);
            fbo.fb = vk_null_handle;
        }
        if (fbo.color_tex_id == 0)
        {
            return;
        }
        vk_texture_object* tex = find_texture(fbo.color_tex_id);
        if (tex == nullptr || tex->view == vk_null_handle)
        {
            return;
        }

        if (!tex->is_fbo_attachment)
        {
            flush_pending_commands_and_wait();
            destroy_vk_image_resources(tex->view, tex->image, tex->memory);
            tex->is_fbo_attachment = true;
            if (create_image_2d(
                    tex->width, tex->height, tex->mip_levels,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    0x01u | 0x02u | 0x04u | 0x10u, 1u,
                    tex->image, tex->memory, tex->view))
            {
                execute_immediate_cmd([&](VkCommandBuffer cmd)
                {
                    clear_color_image(cmd, tex->image, tex->mip_levels);
                });
                tex->current_layout = VK_IMAGE_LAYOUT_GENERAL;
            }
            else
            {
                tex->current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
                return;
            }
            reset_all_descriptor_caches();
            ctx.tex_bindings_dirty = true;
        }

        fbo.width = tex->width;
        fbo.height = tex->height;

        VkImageView ds_view = vk_null_handle;
        if (fbo.depth_rb_id != 0)
        {
            auto it_r = ctx.rbos.find(fbo.depth_rb_id);
            if (it_r != ctx.rbos.end() &&
                it_r->second.view != vk_null_handle &&
                it_r->second.width == fbo.width &&
                it_r->second.height == fbo.height)
            {
                ds_view = it_r->second.view;
            }
        }
        if (ds_view == vk_null_handle)
        {
            const uint32_t auto_rbo = 0x80000000u | fbo.id;
            auto& rbo = ctx.rbos[auto_rbo];
            if (rbo.view == vk_null_handle || rbo.width != fbo.width || rbo.height != fbo.height)
            {
                flush_pending_commands_and_wait();
                destroy_vk_image_resources(rbo.view, rbo.image, rbo.memory);
                rbo.width = fbo.width;
                rbo.height = fbo.height;
                if (create_image_2d(
                        fbo.width, fbo.height, 1u, ctx.depth_stencil_format,
                        0x02u | 0x20u, 0x06u,
                        rbo.image, rbo.memory, rbo.view))
                {
                    execute_immediate_cmd([&](VkCommandBuffer cmd)
                    {
                        clear_depth_stencil_image(cmd, rbo.image);
                    });
                }
                else
                {
                    rbo.width = 0u;
                    rbo.height = 0u;
                }
            }
            ds_view = rbo.view;
        }
        if (ds_view == vk_null_handle)
        {
            return;
        }

        VkImageView views[2] = {tex->view, ds_view};
        VkFramebufferCreateInfo fbci{};
        fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass = ctx.scene_render_pass;
        fbci.attachmentCount = 2u;
        fbci.pAttachments = views;
        fbci.width = fbo.width;
        fbci.height = fbo.height;
        fbci.layers = 1u;
        if (ctx.vk.vkCreateFramebuffer(ctx.device, &fbci, nullptr, &fbo.fb) != VK_SUCCESS)
        {
            fbo.fb = vk_null_handle;
        }
    }

    void __stdcall vk_glBindFramebufferEXT(uint32_t, uint32_t framebuffer)
    {
        uint32_t new_fb_w = ctx.cur_fb_w;
        uint32_t new_fb_h = ctx.cur_fb_h;
        VkFramebuffer new_vk_fb = vk_null_handle;
        VkImage new_color_img = vk_null_handle;

        if (framebuffer == 0)
        {
            new_fb_w = (ctx.fbo0_width > 0u) ? ctx.fbo0_width : ctx.vid_width;
            new_fb_h = (ctx.fbo0_height > 0u) ? ctx.fbo0_height : ctx.vid_height;
            new_vk_fb = ctx.fbo0_fb;
            new_color_img = ctx.fbo0_color_img;
        }
        else
        {
            auto& fbo = ctx.fbos[framebuffer];
            fbo.id = framebuffer;
            if (fbo.fb == vk_null_handle && fbo.color_tex_id != 0)
            {
                rebuild_offscreen_fbo(fbo);
            }
            vk_texture_object* tex = find_texture(fbo.color_tex_id);
            if (fbo.fb != vk_null_handle && tex != nullptr)
            {
                new_fb_w = fbo.width;
                new_fb_h = fbo.height;
                new_vk_fb = fbo.fb;
                new_color_img = tex->image;
            }
        }

        if (ctx.bound_fbo_id != framebuffer ||
            ctx.cur_vk_fb != new_vk_fb ||
            ctx.cur_fb_w != new_fb_w ||
            ctx.cur_fb_h != new_fb_h ||
            ctx.cur_color_img != new_color_img)
        {
            end_active_render_pass();
            ctx.bound_fbo_id = framebuffer;
            ctx.cur_fb_w = new_fb_w;
            ctx.cur_fb_h = new_fb_h;
            ctx.cur_vk_fb = new_vk_fb;
            ctx.cur_color_img = new_color_img;
            ctx.cmd_vp_dirty = true;
            ctx.cmd_sc_dirty = true;
        }
    }

    void __stdcall vk_glDeleteFramebuffersEXT(int32_t n, const uint32_t* framebuffers)
    {
        if (n <= 0 || framebuffers == nullptr || !ctx.context_created)
        {
            return;
        }
        flush_pending_commands_and_wait();
        for (int32_t i = 0; i < n; ++i)
        {
            const uint32_t fid = framebuffers[i];
            if (fid == 0u)
            {
                continue;
            }
            if (ctx.bound_fbo_id == fid)
            {
                vk_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0u);
            }
            auto it = ctx.fbos.find(fid);
            if (it != ctx.fbos.end())
            {
                if (it->second.fb != vk_null_handle)
                {
                    ctx.vk.vkDestroyFramebuffer(ctx.device, it->second.fb, nullptr);
                }
                ctx.fbos.erase(it);
            }
            auto it_rbo = ctx.rbos.find(0x80000000u | fid);
            if (it_rbo != ctx.rbos.end())
            {
                destroy_vk_image_resources(it_rbo->second.view, it_rbo->second.image, it_rbo->second.memory);
                ctx.rbos.erase(it_rbo);
            }
        }
    }

    void __stdcall vk_glGenFramebuffersEXT(int32_t n, uint32_t* framebuffers)
    {
        if (n <= 0 || framebuffers == nullptr)
        {
            return;
        }
        for (int32_t i = 0; i < n; ++i)
        {
            const uint32_t id = ctx.next_fbo_id++;
            framebuffers[i] = id;
            ctx.fbos[id].id = id;
        }
    }

    uint32_t __stdcall vk_glCheckFramebufferStatusEXT(uint32_t)
    {
        if (ctx.bound_fbo_id == 0u)
        {
            return (ctx.fbo0_fb != vk_null_handle)
                ? GL_FRAMEBUFFER_COMPLETE_EXT
                : GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
        }
        const auto it = ctx.fbos.find(ctx.bound_fbo_id);
        if (it != ctx.fbos.end() &&
            it->second.fb != vk_null_handle &&
            find_texture(it->second.color_tex_id) != nullptr)
        {
            return GL_FRAMEBUFFER_COMPLETE_EXT;
        }
        return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
    }

    void __stdcall vk_glFramebufferTexture1DEXT(
        uint32_t,
        uint32_t,
        uint32_t,
        uint32_t,
        int32_t)
    {
    }

    void __stdcall vk_glFramebufferTexture2DEXT(
        uint32_t,
        uint32_t attachment,
        uint32_t,
        uint32_t texture,
        int32_t)
    {
        if (ctx.bound_fbo_id == 0 || attachment != GL_COLOR_ATTACHMENT0_EXT)
        {
            return;
        }
        auto& fbo = ctx.fbos[ctx.bound_fbo_id];
        fbo.color_tex_id = texture;
        if (texture != 0u)
        {
            vk_texture_object& tex = get_or_create_texture(texture);
            if (tex.image == vk_null_handle)
            {
                tex.is_fbo_attachment = true;
            }
        }
        rebuild_offscreen_fbo(fbo);
        vk_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx.bound_fbo_id);
    }

    void __stdcall vk_glFramebufferTexture3DEXT(
        uint32_t,
        uint32_t,
        uint32_t,
        uint32_t,
        int32_t,
        int32_t)
    {
    }

    void __stdcall vk_glFramebufferRenderbufferEXT(
        uint32_t,
        uint32_t attachment,
        uint32_t,
        uint32_t renderbuffer)
    {
        if (ctx.bound_fbo_id == 0)
        {
            return;
        }
        auto& fbo = ctx.fbos[ctx.bound_fbo_id];
        if (attachment == GL_DEPTH_ATTACHMENT_EXT)
        {
            fbo.depth_rb_id = renderbuffer;
        }
        rebuild_offscreen_fbo(fbo);
        vk_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx.bound_fbo_id);
    }

    void __stdcall vk_glGetFramebufferAttachmentParameterivEXT(
        uint32_t,
        uint32_t,
        uint32_t,
        int32_t* params)
    {
        if (params != nullptr)
        {
            *params = 0;
        }
    }

    void __stdcall vk_glGenerateMipmapEXT(uint32_t)
    {
    }

    void __stdcall vk_glBlitFramebufferEXT(
        int32_t,
        int32_t,
        int32_t,
        int32_t,
        int32_t,
        int32_t,
        int32_t,
        int32_t,
        uint32_t,
        uint32_t)
    {
    }
}
