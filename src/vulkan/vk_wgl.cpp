#include "vulkan/vk_internal.hpp"

namespace qlqpc::vulkan
{
    namespace
    {
        uint32_t __stdcall noop_stdcall_0()
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_4(uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_8(uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_12(uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_16(uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_20(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_24(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_28(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_32(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_36(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_40(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_48(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        uint32_t __stdcall noop_stdcall_56(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
        {
            return 1u;
        }

        void* get_noop_for_arg_bytes(uint16_t arg_bytes)
        {
            switch (arg_bytes)
            {
                case 0:
                    return reinterpret_cast<void*>(&noop_stdcall_0);
                case 4:
                    return reinterpret_cast<void*>(&noop_stdcall_4);
                case 8:
                    return reinterpret_cast<void*>(&noop_stdcall_8);
                case 12:
                    return reinterpret_cast<void*>(&noop_stdcall_12);
                case 16:
                    return reinterpret_cast<void*>(&noop_stdcall_16);
                case 20:
                    return reinterpret_cast<void*>(&noop_stdcall_20);
                case 24:
                    return reinterpret_cast<void*>(&noop_stdcall_24);
                case 28:
                    return reinterpret_cast<void*>(&noop_stdcall_28);
                case 32:
                    return reinterpret_cast<void*>(&noop_stdcall_32);
                case 36:
                    return reinterpret_cast<void*>(&noop_stdcall_36);
                case 40:
                    return reinterpret_cast<void*>(&noop_stdcall_40);
                case 48:
                    return reinterpret_cast<void*>(&noop_stdcall_48);
                case 56:
                    return reinterpret_cast<void*>(&noop_stdcall_56);
                default:
                    return nullptr;
            }
        }

        struct ext_proc_entry
        {
            const char* name;
            PROC fn;
        };

        struct qgl_proc_entry
        {
            const char* name;
            void* fn;
        };

        void* resolve_implemented_qgl_func(std::string_view name)
        {
            static const qgl_proc_entry procs[] =
            {
                {"glAlphaFunc", reinterpret_cast<void*>(&vk_glAlphaFunc)},
                {"glArrayElement", reinterpret_cast<void*>(&vk_glArrayElement)},
                {"glBegin", reinterpret_cast<void*>(&vk_glBegin)},
                {"glBindTexture", reinterpret_cast<void*>(&vk_glBindTexture)},
                {"glBlendFunc", reinterpret_cast<void*>(&vk_glBlendFunc)},
                {"glClear", reinterpret_cast<void*>(&vk_glClear)},
                {"glClearColor", reinterpret_cast<void*>(&vk_glClearColor)},
                {"glClearDepth", reinterpret_cast<void*>(&vk_glClearDepth)},
                {"glClearStencil", reinterpret_cast<void*>(&vk_glClearStencil)},
                {"glClipPlane", reinterpret_cast<void*>(&vk_glClipPlane)},
                {"glColor3f", reinterpret_cast<void*>(&vk_glColor3f)},
                {"glColor4f", reinterpret_cast<void*>(&vk_glColor4f)},
                {"glColor4fv", reinterpret_cast<void*>(&vk_glColor4fv)},
                {"glColor4ubv", reinterpret_cast<void*>(&vk_glColor4ubv)},
                {"glColorMask", reinterpret_cast<void*>(&vk_glColorMask)},
                {"glColorPointer", reinterpret_cast<void*>(&vk_glColorPointer)},
                {"glCopyTexSubImage2D", reinterpret_cast<void*>(&vk_glCopyTexSubImage2D)},
                {"glCullFace", reinterpret_cast<void*>(&vk_glCullFace)},
                {"glDeleteTextures", reinterpret_cast<void*>(&vk_glDeleteTextures)},
                {"glDepthFunc", reinterpret_cast<void*>(&vk_glDepthFunc)},
                {"glDepthMask", reinterpret_cast<void*>(&vk_glDepthMask)},
                {"glDepthRange", reinterpret_cast<void*>(&vk_glDepthRange)},
                {"glDisable", reinterpret_cast<void*>(&vk_glDisable)},
                {"glDisableClientState", reinterpret_cast<void*>(&vk_glDisableClientState)},
                {"glDrawArrays", reinterpret_cast<void*>(&vk_glDrawArrays)},
                {"glDrawBuffer", reinterpret_cast<void*>(&vk_glDrawBuffer)},
                {"glDrawElements", reinterpret_cast<void*>(&vk_glDrawElements)},
                {"glEnable", reinterpret_cast<void*>(&vk_glEnable)},
                {"glEnableClientState", reinterpret_cast<void*>(&vk_glEnableClientState)},
                {"glEnd", reinterpret_cast<void*>(&vk_glEnd)},
                {"glFinish", reinterpret_cast<void*>(&vk_glFinish)},
                {"glFlush", reinterpret_cast<void*>(&vk_glFlush)},
                {"glFrontFace", reinterpret_cast<void*>(&vk_glFrontFace)},
                {"glFrustum", reinterpret_cast<void*>(&vk_glFrustum)},
                {"glGenTextures", reinterpret_cast<void*>(&vk_glGenTextures)},
                {"glGetBooleanv", reinterpret_cast<void*>(&vk_glGetBooleanv)},
                {"glGetError", reinterpret_cast<void*>(&vk_glGetError)},
                {"glGetIntegerv", reinterpret_cast<void*>(&vk_glGetIntegerv)},
                {"glGetString", reinterpret_cast<void*>(&vk_glGetString)},
                {"glHint", reinterpret_cast<void*>(&vk_glHint)},
                {"glIsEnabled", reinterpret_cast<void*>(&vk_glIsEnabled)},
                {"glLineWidth", reinterpret_cast<void*>(&vk_glLineWidth)},
                {"glLoadIdentity", reinterpret_cast<void*>(&vk_glLoadIdentity)},
                {"glLoadMatrixf", reinterpret_cast<void*>(&vk_glLoadMatrixf)},
                {"glMatrixMode", reinterpret_cast<void*>(&vk_glMatrixMode)},
                {"glMultMatrixf", reinterpret_cast<void*>(&vk_glMultMatrixf)},
                {"glOrtho", reinterpret_cast<void*>(&vk_glOrtho)},
                {"glPixelStorei", reinterpret_cast<void*>(&vk_glPixelStorei)},
                {"glPolygonMode", reinterpret_cast<void*>(&vk_glPolygonMode)},
                {"glPolygonOffset", reinterpret_cast<void*>(&vk_glPolygonOffset)},
                {"glPopClientAttrib", reinterpret_cast<void*>(&vk_glPopClientAttrib)},
                {"glPopMatrix", reinterpret_cast<void*>(&vk_glPopMatrix)},
                {"glPushClientAttrib", reinterpret_cast<void*>(&vk_glPushClientAttrib)},
                {"glPushMatrix", reinterpret_cast<void*>(&vk_glPushMatrix)},
                {"glReadBuffer", reinterpret_cast<void*>(&vk_glReadBuffer)},
                {"glReadPixels", reinterpret_cast<void*>(&vk_glReadPixels)},
                {"glRotatef", reinterpret_cast<void*>(&vk_glRotatef)},
                {"glScalef", reinterpret_cast<void*>(&vk_glScalef)},
                {"glScissor", reinterpret_cast<void*>(&vk_glScissor)},
                {"glShadeModel", reinterpret_cast<void*>(&vk_glShadeModel)},
                {"glStencilFunc", reinterpret_cast<void*>(&vk_glStencilFunc)},
                {"glStencilMask", reinterpret_cast<void*>(&vk_glStencilMask)},
                {"glStencilOp", reinterpret_cast<void*>(&vk_glStencilOp)},
                {"glTexCoord2f", reinterpret_cast<void*>(&vk_glTexCoord2f)},
                {"glTexCoord2fv", reinterpret_cast<void*>(&vk_glTexCoord2fv)},
                {"glTexCoordPointer", reinterpret_cast<void*>(&vk_glTexCoordPointer)},
                {"glTexEnvf", reinterpret_cast<void*>(&vk_glTexEnvf)},
                {"glTexEnvi", reinterpret_cast<void*>(&vk_glTexEnvi)},
                {"glTexImage2D", reinterpret_cast<void*>(&vk_glTexImage2D)},
                {"glTexParameterf", reinterpret_cast<void*>(&vk_glTexParameterf)},
                {"glTexParameteri", reinterpret_cast<void*>(&vk_glTexParameteri)},
                {"glTexSubImage2D", reinterpret_cast<void*>(&vk_glTexSubImage2D)},
                {"glTranslatef", reinterpret_cast<void*>(&vk_glTranslatef)},
                {"glVertex2f", reinterpret_cast<void*>(&vk_glVertex2f)},
                {"glVertex3f", reinterpret_cast<void*>(&vk_glVertex3f)},
                {"glVertex3fv", reinterpret_cast<void*>(&vk_glVertex3fv)},
                {"glVertexPointer", reinterpret_cast<void*>(&vk_glVertexPointer)},
                {"glViewport", reinterpret_cast<void*>(&vk_glViewport)},
                {"wglChoosePixelFormat", reinterpret_cast<void*>(&vk_wglChoosePixelFormat)},
                {"wglDescribePixelFormat", reinterpret_cast<void*>(&vk_wglDescribePixelFormat)},
                {"wglSetPixelFormat", reinterpret_cast<void*>(&vk_wglSetPixelFormat)},
                {"wglCreateContext", reinterpret_cast<void*>(&vk_wglCreateContext)},
                {"wglMakeCurrent", reinterpret_cast<void*>(&vk_wglMakeCurrent)},
                {"wglDeleteContext", reinterpret_cast<void*>(&vk_wglDeleteContext)},
                {"wglShareLists", reinterpret_cast<void*>(&vk_wglShareLists)},
                {"wglSwapBuffers", reinterpret_cast<void*>(&vk_wglSwapBuffers)},
                {"wglGetProcAddress", reinterpret_cast<void*>(&vk_wglGetProcAddress)},
            };

            for (const auto& entry : procs)
            {
                if (name == entry.name)
                {
                    return entry.fn;
                }
            }
            return nullptr;
        }
    }

    int __stdcall vk_wglChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR*)
    {
        return 1;
    }

    int __stdcall vk_wglDescribePixelFormat(
        HDC,
        int,
        UINT n_bytes,
        LPPIXELFORMATDESCRIPTOR ppfd)
    {
        if (ppfd != nullptr && n_bytes >= sizeof(PIXELFORMATDESCRIPTOR))
        {
            std::memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
            ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
            ppfd->nVersion = 1;
            ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            ppfd->iPixelType = PFD_TYPE_RGBA;
            ppfd->cColorBits = 32;
            ppfd->cRedBits = 8;
            ppfd->cGreenBits = 8;
            ppfd->cBlueBits = 8;
            ppfd->cAlphaBits = 8;
            ppfd->cDepthBits = 24;
            ppfd->cStencilBits = 8;
        }
        return 1;
    }

    int __stdcall vk_wglSetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR*)
    {
        return 1;
    }

    void teardown_context_and_surface()
    {
        if (!ctx.context_created || ctx.device == nullptr)
        {
            return;
        }
        flush_pending_commands_and_wait();
        ctx.vk.vkDeviceWaitIdle(ctx.device);
        stop_present_worker();
        ctx.frames_in_flight_busy = false;
        ctx.frame_fence_pending[0] = false;
        ctx.frame_fence_pending[1] = false;
        save_pipeline_cache();

        for (auto& [id, fbo] : ctx.fbos)
        {
            if (fbo.fb != vk_null_handle)
            {
                ctx.vk.vkDestroyFramebuffer(ctx.device, fbo.fb, nullptr);
            }
        }
        ctx.fbos.clear();
        ctx.next_fbo_id = 1u;

        for (auto& [id, rbo] : ctx.rbos)
        {
            destroy_vk_image_resources(rbo.view, rbo.image, rbo.memory);
        }
        ctx.rbos.clear();
        ctx.next_rbo_id = 1u;

        for (auto& tex : ctx.textures)
        {
            destroy_vk_image_resources(tex.view, tex.image, tex.memory);
        }
        ctx.textures.clear();
        ctx.textures.resize(4096);
        ctx.next_tex_id = 1u;

        ctx.glsl_objects.clear();
        ctx.next_glsl_handle = 1u;
        ctx.active_program = 0u;
        ctx.next_query_id = 1u;

        ctx.bound_fbo_id = 0u;
        ctx.bound_rbo_id = 0u;
        ctx.cur_fb_w = ctx.fbo0_width > 0 ? ctx.fbo0_width : ctx.vid_width;
        ctx.cur_fb_h = ctx.fbo0_height > 0 ? ctx.fbo0_height : ctx.vid_height;
        ctx.cur_vk_fb = ctx.fbo0_fb;
        ctx.cur_color_img = ctx.fbo0_color_img;

        ctx.matrix_mode = GL_MODELVIEW;
        ctx.mv_stack = {mat4::identity()};
        ctx.proj_stack = {mat4::identity()};
        ctx.tex_stack = {mat4::identity()};

        ctx.active_tex_unit = 0u;
        ctx.client_active_tex_unit = 0u;
        ctx.bound_tex_2d[0] = 0u;
        ctx.bound_tex_2d[1] = 0u;
        ctx.bound_tex_rect[0] = 0u;
        ctx.bound_tex_rect[1] = 0u;
        ctx.tex_2d_enabled[0] = false;
        ctx.tex_2d_enabled[1] = false;
        ctx.tex_rect_enabled[0] = false;
        ctx.tex_rect_enabled[1] = false;
        ctx.tex_env_mode[0] = GL_MODULATE;
        ctx.tex_env_mode[1] = GL_MODULATE;

        ctx.clear_color[0] = 0.0f;
        ctx.clear_color[1] = 0.0f;
        ctx.clear_color[2] = 0.0f;
        ctx.clear_color[3] = 1.0f;
        ctx.clear_depth = 1.0f;
        ctx.clear_stencil = 0u;
        ctx.viewport[0] = 0;
        ctx.viewport[1] = 0;
        ctx.viewport[2] = static_cast<int>(ctx.cur_fb_w);
        ctx.viewport[3] = static_cast<int>(ctx.cur_fb_h);
        ctx.scissor[0] = 0;
        ctx.scissor[1] = 0;
        ctx.scissor[2] = static_cast<int>(ctx.cur_fb_w);
        ctx.scissor[3] = static_cast<int>(ctx.cur_fb_h);
        ctx.depth_range[0] = 0.0f;
        ctx.depth_range[1] = 1.0f;
        ctx.scissor_enabled = false;
        ctx.cull_enabled = false;
        ctx.cull_face_mode = GL_BACK;
        ctx.front_face_mode = GL_CCW;
        ctx.polygon_mode = GL_FILL;
        ctx.poly_offset_enabled = false;
        ctx.poly_offset_factor = 0.0f;
        ctx.poly_offset_units = 0.0f;
        ctx.depth_test_enabled = false;
        ctx.depth_write_enabled = true;
        ctx.depth_func = GL_LEQUAL;
        ctx.blend_enabled = false;
        ctx.blend_src = GL_ONE;
        ctx.blend_dst = GL_ZERO;
        ctx.color_write_mask = 0x0Fu;
        ctx.alpha_test_enabled = false;
        ctx.alpha_test_func = GL_GREATER;
        ctx.stencil_test_enabled = false;
        ctx.stencil_func = GL_ALWAYS;
        ctx.stencil_ref = 0;
        ctx.stencil_val_mask = 0xFFu;
        ctx.stencil_write_mask = 0xFFu;
        ctx.stencil_fail_op = GL_KEEP;
        ctx.stencil_zfail_op = GL_KEEP;
        ctx.stencil_zpass_op = GL_KEEP;
        ctx.clip_plane_enabled = false;
        ctx.clip_plane_eye[0] = 0.0f;
        ctx.clip_plane_eye[1] = 0.0f;
        ctx.clip_plane_eye[2] = 0.0f;
        ctx.clip_plane_eye[3] = 1.0f;
        ctx.unpack_alignment = 4;
        ctx.unpack_row_length = 0;
        ctx.unpack_skip_pixels = 0;
        ctx.unpack_skip_rows = 0;
        ctx.pack_alignment = 4;
        ctx.client_attrib_stack.clear();
        ctx.va_vertex_enabled = false;
        ctx.va_color_enabled = false;
        ctx.va_texcoord_enabled[0] = false;
        ctx.va_texcoord_enabled[1] = false;
        ctx.va_vertex_size = 3;
        ctx.va_vertex_stride = 0;
        ctx.va_vertex_ptr = nullptr;
        ctx.va_color_size = 4;
        ctx.va_color_type = GL_UNSIGNED_BYTE;
        ctx.va_color_stride = 0;
        ctx.va_color_ptr = nullptr;
        ctx.va_tc_size[0] = 2;
        ctx.va_tc_size[1] = 2;
        ctx.va_tc_stride[0] = 0;
        ctx.va_tc_stride[1] = 0;
        ctx.va_tc_ptr[0] = nullptr;
        ctx.va_tc_ptr[1] = nullptr;
        ctx.in_begin_end = false;
        ctx.imm_prim_mode = GL_TRIANGLES;
        ctx.cur_color[0] = 255u;
        ctx.cur_color[1] = 255u;
        ctx.cur_color[2] = 255u;
        ctx.cur_color[3] = 255u;
        ctx.cur_uv[0][0] = 0.0f;
        ctx.cur_uv[0][1] = 0.0f;
        ctx.cur_uv[1][0] = 0.0f;
        ctx.cur_uv[1][1] = 0.0f;
        ctx.imm_vertices.clear();
        reset_all_descriptor_caches();
        invalidate_cmd_draw_state();
        ctx.pso_key_dirty = true;
        ctx.cached_pipeline = vk_null_handle;
        ctx.cached_pso_topo = 0xFFu;
        ctx.mvp_dirty = true;
        ctx.cva_locked = false;
        ctx.cva_lock_first = 0u;
        ctx.cva_lock_count = 0u;
        ctx.cva_ibo_valid = false;
        ctx.cva_cached_idx_ptr = nullptr;
        ctx.cva_cached_idx_count = 0u;
        ctx.cva_cached_idx_first_val = 0u;
        ctx.cva_cached_idx_last_val = 0u;
        ctx.cva_cached_idx_hash = 0ull;
        ctx.cva_cached_min_idx = 0u;
        ctx.cva_cached_v_count = 0u;
        ctx.cva_cached_ibo_off = 0ull;
        ctx.window_state_cached_this_frame = false;

        // destroy swapchain and surface before the engine tears down its window
        destroy_swapchain();
        if (ctx.surface != vk_null_handle)
        {
            ctx.vk.vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
            ctx.surface = vk_null_handle;
        }
        ctx.hdc = nullptr;
        ctx.hwnd = nullptr;
        ctx.last_client_w = 0u;
        ctx.last_client_h = 0u;
        ctx.cached_client_w = 0u;
        ctx.cached_client_h = 0u;
        ctx.cached_window_valid = false;
        ctx.cached_window_minimized = false;
        ctx.cached_window_focused = true;
        ctx.last_polled_hwnd = nullptr;
        ctx.last_window_poll_qpc = 0;
        ctx.last_window_focused = true;
        ctx.window_focused_atomic.store(true, std::memory_order_relaxed);
        ctx.suboptimal_recreate_attempted = false;
        ctx.swapchain_needs_recreate = false;
        ctx.swapchain_out_of_date = false;
    }

    bool bind_hdc_and_ensure_swapchain(HDC hdc)
    {
        if (hdc == nullptr)
        {
            return false;
        }
        if (!ctx.context_created)
        {
            return ensure_vulkan_device_created(hdc);
        }

        const HWND new_hwnd = WindowFromDC(hdc);
        if (new_hwnd == nullptr || !IsWindow(new_hwnd))
        {
            return false;
        }

        ctx.hdc = hdc;
        if (ctx.surface == vk_null_handle ||
            ctx.swapchain == vk_null_handle ||
            new_hwnd != ctx.hwnd)
        {
            flush_pending_commands_and_wait();
            ctx.vk.vkDeviceWaitIdle(ctx.device);
            ctx.frames_in_flight_busy = false;
            ctx.frame_fence_pending[0] = false;
            ctx.frame_fence_pending[1] = false;
            destroy_swapchain();
            if (ctx.surface != vk_null_handle)
            {
                ctx.vk.vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
                ctx.surface = vk_null_handle;
            }
            ctx.hwnd = new_hwnd;
            RECT rc{};
            if (GetClientRect(ctx.hwnd, &rc) && rc.right > rc.left && rc.bottom > rc.top)
            {
                ctx.vid_width = static_cast<uint32_t>(rc.right - rc.left);
                ctx.vid_height = static_cast<uint32_t>(rc.bottom - rc.top);
                ctx.last_client_w = ctx.vid_width;
                ctx.last_client_h = ctx.vid_height;
            }
            VkWin32SurfaceCreateInfoKHR wsci{};
            wsci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            wsci.hinstance = GetModuleHandleW(nullptr);
            wsci.hwnd = ctx.hwnd;
            if (ctx.vk.vkCreateWin32SurfaceKHR(ctx.instance, &wsci, nullptr, &ctx.surface) != VK_SUCCESS)
            {
                return false;
            }
            if (!create_or_recreate_swapchain())
            {
                return false;
            }
        }
        return true;
    }

    HGLRC __stdcall vk_wglCreateContext(HDC hdc)
    {
        if (!bind_hdc_and_ensure_swapchain(hdc))
        {
            return nullptr;
        }
        return reinterpret_cast<HGLRC>(0x564B4354u);
    }

    int __stdcall vk_wglMakeCurrent(HDC hdc, HGLRC hglrc)
    {
        if (hglrc == nullptr)
        {
            return 1;
        }
        if (!bind_hdc_and_ensure_swapchain(hdc))
        {
            return 0;
        }
        return 1;
    }

    int __stdcall vk_wglDeleteContext(HGLRC)
    {
        teardown_context_and_surface();
        return 1;
    }

    int __stdcall vk_wglShareLists(HGLRC, HGLRC)
    {
        return 1;
    }

    int __stdcall vk_wglSwapBuffers(HDC hdc)
    {
        if (!ctx.context_created)
        {
            return 1;
        }

        struct reset_window_cache_on_exit
        {
            ~reset_window_cache_on_exit()
            {
                ctx.window_state_cached_this_frame = false;
            }
        } reset_window_cache_guard{};

        if (!ctx.window_state_cached_this_frame && ctx.hwnd != nullptr)
        {
            poll_hwnd_state_throttled(ctx.hwnd);
        }
        const bool minimized = (ctx.hwnd != nullptr) && ctx.cached_window_minimized;
        const bool focused = !minimized &&
            ((ctx.hwnd == nullptr) || (ctx.cached_window_valid && ctx.cached_window_focused));
        const bool focus_restored = focused && !ctx.last_window_focused;
        ctx.last_window_focused = focused;
        ctx.window_focused_atomic.store(focused, std::memory_order_relaxed);
        if (focus_restored)
        {
            ctx.suboptimal_recreate_attempted = false;
        }

        const auto handle_surface_lost = [&]()
        {
            end_active_render_pass();
            flush_pending_commands_and_wait();
            destroy_swapchain();
            if (ctx.surface != vk_null_handle)
            {
                ctx.vk.vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
                ctx.surface = vk_null_handle;
            }
            ctx.swapchain_out_of_date = true;
            ctx.swapchain_needs_recreate = true;
            if (focused && !minimized)
            {
                bind_hdc_and_ensure_swapchain((hdc != nullptr) ? hdc : ctx.hdc);
            }
        };

        const auto handle_present_outcome = [&](VkResult sub_res, VkResult pres_res, VkResult acq_res)
        {
            if (sub_res == VK_SUCCESS && pres_res == VK_SUCCESS && acq_res != VK_SUBOPTIMAL_KHR)
            {
                ctx.swapchain_needs_recreate = false;
                ctx.swapchain_out_of_date = false;
                ctx.suboptimal_recreate_attempted = false;
            }
            else if (sub_res == VK_ERROR_SURFACE_LOST_KHR || pres_res == VK_ERROR_SURFACE_LOST_KHR)
            {
                handle_surface_lost();
            }
            else if (pres_res == VK_ERROR_OUT_OF_DATE_KHR)
            {
                ctx.swapchain_out_of_date = true;
                ctx.swapchain_needs_recreate = true;
                if (focused && !minimized)
                {
                    end_active_render_pass();
                    flush_pending_commands_and_wait();
                    create_or_recreate_swapchain();
                }
            }
            else if (pres_res == VK_SUBOPTIMAL_KHR || acq_res == VK_SUBOPTIMAL_KHR)
            {
                // skip recreate on suboptimal if the client extent still matches to avoid dwm transition stalls
                const bool extent_mismatch =
                    ctx.cached_client_w > 0u &&
                    ctx.cached_client_h > 0u &&
                    (ctx.cached_client_w != ctx.swapchain_extent.width ||
                     ctx.cached_client_h != ctx.swapchain_extent.height);
                if (extent_mismatch &&
                    should_recreate_on_suboptimal(minimized, focused, ctx.suboptimal_recreate_attempted))
                {
                    ctx.swapchain_needs_recreate = true;
                    ctx.suboptimal_recreate_attempted = true;
                    end_active_render_pass();
                    flush_pending_commands_and_wait();
                    create_or_recreate_swapchain();
                }
            }
        };

        wait_for_present_worker_idle();
        if (ctx.has_present_result)
        {
            ctx.has_present_result = false;
            handle_present_outcome(
                ctx.last_submit_result,
                ctx.last_present_result,
                ctx.last_present_acq_result);
        }

        const swapchain_frame_action pre_action = evaluate_pre_present_action({
            minimized,
            focused,
            focus_restored,
            ctx.swapchain != vk_null_handle && ctx.surface != vk_null_handle,
            ctx.swapchain_out_of_date,
            ctx.swapchain_needs_recreate,
            ctx.suboptimal_recreate_attempted,
        });

        if (pre_action == swapchain_frame_action::skip_frame)
        {
            end_active_render_pass();
            flush_pending_commands_and_wait();
            if (ctx.has_preacquired_image)
            {
                ctx.has_preacquired_image = false;
                destroy_swapchain();
                ctx.swapchain_needs_recreate = true;
            }
            return 1;
        }

        if (pre_action == swapchain_frame_action::recreate_and_proceed)
        {
            end_active_render_pass();
            flush_pending_commands_and_wait();
            if (ctx.swapchain == vk_null_handle || ctx.surface == vk_null_handle)
            {
                const HDC target_hdc = (hdc != nullptr) ? hdc : ctx.hdc;
                if (target_hdc != nullptr)
                {
                    bind_hdc_and_ensure_swapchain(target_hdc);
                }
            }
            else
            {
                create_or_recreate_swapchain();
            }
            if (ctx.swapchain == vk_null_handle || ctx.swapchain_out_of_date)
            {
                end_active_render_pass();
                flush_pending_commands_and_wait();
                return 1;
            }
        }

        end_active_render_pass();
        if (!ctx.gamma_identity && ctx.present_render_pass != vk_null_handle &&
            (ctx.vbo_offset + 128u > vulkan_context::vbo_size ||
             ctx.ibo_offset + 32u > vulkan_context::ibo_size))
        {
            flush_pending_commands_and_wait();
        }
        begin_frame_if_needed();
        if (ctx.swapchain == vk_null_handle || !ctx.cmd_recording || ctx.active_cmd == nullptr)
        {
            flush_pending_commands_and_wait();
            return 1;
        }

        uint32_t sc_idx = 0;
        uint32_t cur_sem_idx = 0;
        VkResult acq = VK_SUCCESS;
        if (ctx.has_preacquired_image)
        {
            ctx.has_preacquired_image = false;
            sc_idx = ctx.preacquired_sc_idx;
            cur_sem_idx = ctx.preacquired_sem_idx;
            acq = ctx.preacquired_result;
        }
        else
        {
            cur_sem_idx = ctx.next_sem_idx % vulkan_context::sem_ring_size;
            ctx.next_sem_idx = (cur_sem_idx + 1u) % vulkan_context::sem_ring_size;
            acq = ctx.vk.vkAcquireNextImageKHR(
                ctx.device, ctx.swapchain, ~0ull,
                ctx.img_avail_sems[cur_sem_idx], vk_null_handle, &sc_idx);
        }

        if (acq < 0)
        {
            // submit active_cmd without waiting on the failed semaphore so the fence signals cleanly
            flush_pending_commands_and_wait();
            ctx.swapchain_out_of_date = true;
            ctx.swapchain_needs_recreate = true;
            if (acq == VK_ERROR_SURFACE_LOST_KHR)
            {
                handle_surface_lost();
            }
            else if (focused && !minimized)
            {
                create_or_recreate_swapchain();
            }
            return 1;
        }
        if (sc_idx >= ctx.swapchain_fbs.size() ||
            sc_idx >= ctx.swapchain_images.size() ||
            sc_idx >= vulkan_context::sem_ring_size)
        {
            flush_pending_commands_and_wait();
            destroy_swapchain();
            ctx.swapchain_needs_recreate = true;
            return 1;
        }

        const uint32_t w = ctx.swapchain_extent.width;
        const uint32_t h = ctx.swapchain_extent.height;

        const bool used_gamma_pass = !ctx.gamma_identity;
        if (!ctx.gamma_identity)
        {
            if (ctx.present_render_pass == vk_null_handle)
            {
                flush_pending_commands_and_wait();
                destroy_swapchain();
                ctx.swapchain_needs_recreate = true;
                return 0;
            }
            pipeline_state_key pkey{};
            pkey.topology = static_cast<uint8_t>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            pkey.polygon_mode = static_cast<uint8_t>(VK_POLYGON_MODE_FILL);
            pkey.cull_mode = VK_CULL_MODE_NONE;
            pkey.color_mask = 0x0Fu;
            pkey.post = post_process_kind::gamma_present;
            pkey.is_present_pass = 1u;

            const VkPipeline pipe = get_or_create_pipeline(pkey);
            const VkDescriptorSet ds = (pipe != vk_null_handle)
                ? get_or_alloc_descriptor_set(
                    ctx.fbo0_color_view, ctx.gamma_lut_tex.sampler,
                    ctx.gamma_lut_tex.view, ctx.gamma_lut_tex.sampler)
                : vk_null_handle;

            if (pipe == vk_null_handle || ds == vk_null_handle ||
                ctx.vbo_offset + 128u > vulkan_context::vbo_size ||
                ctx.ibo_offset + 32u > vulkan_context::ibo_size)
            {
                flush_pending_commands_and_wait();
                destroy_swapchain();
                ctx.swapchain_needs_recreate = true;
                return 0;
            }

            cmd_transition_image(
                ctx.active_cmd, ctx.fbo0_color_img,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 1u, 1u);
            VkRenderPassBeginInfo rpbi{};
            rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpbi.renderPass = ctx.present_render_pass;
            rpbi.framebuffer = ctx.swapchain_fbs[sc_idx];
            rpbi.renderArea = {{0, 0}, {w, h}};
            ctx.vk.vkCmdBeginRenderPass(ctx.active_cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

            ctx.vk.vkCmdBindPipeline(ctx.active_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
            VkViewport vp{0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f};
            VkRect2D sc{{0, 0}, {w, h}};
            ctx.vk.vkCmdSetViewport(ctx.active_cmd, 0u, 1u, &vp);
            ctx.vk.vkCmdSetScissor(ctx.active_cmd, 0u, 1u, &sc);
            ctx.vk.vkCmdSetDepthBias(ctx.active_cmd, 0.0f, 0.0f, 0.0f);
            ctx.vk.vkCmdSetStencilCompareMask(ctx.active_cmd, 3u, 0xFFu);
            ctx.vk.vkCmdSetStencilWriteMask(ctx.active_cmd, 3u, 0xFFu);
            ctx.vk.vkCmdSetStencilReference(ctx.active_cmd, 3u, 0u);

            ctx.vk.vkCmdBindDescriptorSets(
                ctx.active_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                ctx.pipeline_layout, 0u, 1u, &ds, 0u, nullptr);

            shader_push_constants pc{};
            const mat4 id = mat4::identity();
            std::memcpy(pc.mvp, id.m, sizeof(pc.mvp));
            pc.uv_scale[0] = 1.0f;
            pc.uv_scale[1] = 1.0f;
            pc.uv_scale[2] = 1.0f;
            pc.uv_scale[3] = 1.0f;
            ctx.vk.vkCmdPushConstants(
                ctx.active_cmd, ctx.pipeline_layout,
                0x01u | 0x10u, 0u, sizeof(shader_push_constants), &pc);

            // vulkan ndc y = -1 maps directly to the top edge of the swapchain image
            const gl_vk_vertex quad_v[4] =
            {
                {{-1.0f, -1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f}},
                {{ 1.0f, -1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f}},
                {{ 1.0f,  1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f}},
                {{-1.0f,  1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f}},
            };
            const uint32_t quad_i[6] = {0, 1, 2, 0, 2, 3};
            const size_t present_fi = ctx.cur_frame;
            const VkDeviceSize v_off = ctx.vbo_offset;
            const VkDeviceSize i_off = ctx.ibo_offset;
            std::memcpy(ctx.vbo_mapped[present_fi] + v_off, quad_v, sizeof(quad_v));
            std::memcpy(ctx.ibo_mapped[present_fi] + i_off, quad_i, sizeof(quad_i));
            ctx.vbo_offset += 128u;
            ctx.ibo_offset += 32u;
            ctx.vk.vkCmdBindVertexBuffers(ctx.active_cmd, 0u, 1u, &ctx.vbo_bufs[present_fi], &v_off);
            ctx.vk.vkCmdBindIndexBuffer(ctx.active_cmd, ctx.ibo_bufs[present_fi], i_off, VK_INDEX_TYPE_UINT32);
            ctx.vk.vkCmdDrawIndexed(ctx.active_cmd, 6u, 1u, 0u, 0, 0u);
            ctx.vk.vkCmdEndRenderPass(ctx.active_cmd);
            cmd_transition_image(
                ctx.active_cmd, ctx.fbo0_color_img,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1u, 1u);
        }
        else
        {
            const int32_t src_w = static_cast<int32_t>(ctx.fbo0_width > 0 ? ctx.fbo0_width : ctx.vid_width);
            const int32_t src_h = static_cast<int32_t>(ctx.fbo0_height > 0 ? ctx.fbo0_height : ctx.vid_height);
            cmd_transition_image(
                ctx.active_cmd, ctx.fbo0_color_img,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1u, 1u);
            cmd_transition_image(
                ctx.active_cmd, ctx.swapchain_images[sc_idx],
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, 1u);
            VkImageBlit blit{};
            blit.srcSubresource = {1u, 0u, 0u, 1u};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {src_w, src_h, 1};
            blit.dstSubresource = {1u, 0u, 0u, 1u};
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {static_cast<int32_t>(w), static_cast<int32_t>(h), 1};
            const VkFilter blit_filter =
                (src_w == static_cast<int32_t>(w) && src_h == static_cast<int32_t>(h))
                    ? VK_FILTER_NEAREST
                    : VK_FILTER_LINEAR;
            ctx.vk.vkCmdBlitImage(
                ctx.active_cmd,
                ctx.fbo0_color_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                ctx.swapchain_images[sc_idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1u, &blit, blit_filter);
            cmd_transition_image(
                ctx.active_cmd, ctx.fbo0_color_img,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1u, 1u);
            cmd_transition_image(
                ctx.active_cmd, ctx.swapchain_images[sc_idx],
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1u, 1u);
        }

        const VkResult end_res = ctx.vk.vkEndCommandBuffer(ctx.active_cmd);
        ctx.cmd_recording = false;
        const VkCommandBuffer submit_cmd = ctx.active_cmd;
        ctx.active_cmd = nullptr;
        if (end_res != VK_SUCCESS)
        {
            flush_pending_commands_and_wait();
            destroy_swapchain();
            ctx.swapchain_needs_recreate = true;
            return 1;
        }

        const size_t fi = ctx.cur_frame;
        const VkFlags wait_stage = used_gamma_pass
            ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            : VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (focused)
        {
            start_present_worker_if_needed();
            if (ctx.present_worker_thread == nullptr)
            {
                flush_pending_commands_and_wait();
                destroy_swapchain();
                ctx.swapchain_needs_recreate = true;
                return 0;
            }
            // keep the present worker off the main thread's core pair so submit/present does not preempt the frame loop
            const DWORD main_cpu = GetCurrentProcessorNumber();
            if (main_cpu != ctx.last_main_thread_cpu && main_cpu < 32u)
            {
                DWORD_PTR proc_mask = 0;
                DWORD_PTR sys_mask = 0;
                if (GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask) != 0 && proc_mask != 0)
                {
                    const DWORD_PTR pair_mask = static_cast<DWORD_PTR>(3u) << (main_cpu & ~1u);
                    const DWORD_PTR worker_mask =
                        (proc_mask & ~pair_mask) != 0 ? (proc_mask & ~pair_mask) : proc_mask;
                    SetThreadAffinityMask(ctx.present_worker_thread, worker_mask);
                }
                ctx.last_main_thread_cpu = main_cpu;
            }

            uint32_t next_sem = 0u;
            VkSemaphore next_acquire_sem = vk_null_handle;
            if (acq == VK_SUCCESS || acq == VK_SUBOPTIMAL_KHR)
            {
                next_sem = ctx.next_sem_idx % vulkan_context::sem_ring_size;
                ctx.next_sem_idx = (next_sem + 1u) % vulkan_context::sem_ring_size;
                next_acquire_sem = ctx.img_avail_sems[next_sem];
            }

            ctx.pending_present_job.cmd = submit_cmd;
            ctx.pending_present_job.fence = ctx.frame_fences[fi];
            ctx.pending_present_job.wait_sem = ctx.img_avail_sems[cur_sem_idx];
            ctx.pending_present_job.signal_sem = ctx.render_done_sems[sc_idx];
            ctx.pending_present_job.wait_stage = wait_stage;
            ctx.pending_present_job.frame_idx = fi;
            ctx.pending_present_job.sc_idx = sc_idx;
            ctx.pending_present_job.acq_result = acq;
            ctx.pending_present_job.next_sem_idx = next_sem;
            ctx.pending_present_job.next_acquire_sem = next_acquire_sem;

            ctx.frame_fence_pending[fi] = true;
            ctx.frames_in_flight_busy = true;
            ctx.cur_frame = (fi + 1u) % vulkan_context::frames_in_flight;

            ctx.present_worker_state.store(1u, std::memory_order_seq_cst);
            if (ctx.present_worker_sleeping.load(std::memory_order_seq_cst) &&
                ctx.present_worker_wake_event != nullptr)
            {
                SetEvent(ctx.present_worker_wake_event);
            }
            return 1;
        }

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = 1u;
        si.pWaitSemaphores = &ctx.img_avail_sems[cur_sem_idx];
        si.pWaitDstStageMask = &wait_stage;
        si.commandBufferCount = 1u;
        si.pCommandBuffers = &submit_cmd;
        si.signalSemaphoreCount = 1u;
        si.pSignalSemaphores = &ctx.render_done_sems[sc_idx];
        ctx.vk.vkResetFences(ctx.device, 1u, &ctx.frame_fences[fi]);
        const VkResult sub = ctx.vk.vkQueueSubmit(ctx.queue, 1u, &si, ctx.frame_fences[fi]);
        ctx.frame_fence_pending[fi] = (sub == VK_SUCCESS);
        if (sub == VK_SUCCESS)
        {
            ctx.frames_in_flight_busy = true;
        }

        VkResult pres = sub;
        if (sub == VK_SUCCESS && ctx.swapchain != vk_null_handle)
        {
            VkPresentInfoKHR pi{};
            pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            pi.waitSemaphoreCount = 1u;
            pi.pWaitSemaphores = &ctx.render_done_sems[sc_idx];
            pi.swapchainCount = 1u;
            pi.pSwapchains = &ctx.swapchain;
            pi.pImageIndices = &sc_idx;
            pres = ctx.vk.vkQueuePresentKHR(ctx.queue, &pi);
        }
        handle_present_outcome(sub, pres, acq);
        if (!focused && ctx.frames_in_flight_busy && sub == VK_SUCCESS)
        {
            for (size_t i = 0; i < vulkan_context::frames_in_flight; ++i)
            {
                if (ctx.frame_fence_pending[i])
                {
                    ctx.vk.vkWaitForFences(ctx.device, 1u, &ctx.frame_fences[i], VK_TRUE, ~0ull);
                    ctx.frame_fence_pending[i] = false;
                }
            }
            ctx.frames_in_flight_busy = false;
        }

        ctx.cur_frame = (fi + 1u) % vulkan_context::frames_in_flight;
        return 1;
    }

    PROC __stdcall vk_wglGetProcAddress(LPCSTR lpsz_proc)
    {
        if (lpsz_proc == nullptr)
        {
            return nullptr;
        }
        const std::string_view name(lpsz_proc);

        static const ext_proc_entry procs[] =
        {
            {"glActiveTextureARB", reinterpret_cast<PROC>(&vk_glActiveTextureARB)},
            {"glClientActiveTextureARB", reinterpret_cast<PROC>(&vk_glClientActiveTextureARB)},
            {"glMultiTexCoord2fARB", reinterpret_cast<PROC>(&vk_glMultiTexCoord2fARB)},
            {"glLockArraysEXT", reinterpret_cast<PROC>(&vk_glLockArraysEXT)},
            {"glUnlockArraysEXT", reinterpret_cast<PROC>(&vk_glUnlockArraysEXT)},
            {"wglGetDeviceGammaRamp3DFX", reinterpret_cast<PROC>(&vk_wglGetDeviceGammaRamp3DFX)},
            {"wglSetDeviceGammaRamp3DFX", reinterpret_cast<PROC>(&vk_wglSetDeviceGammaRamp3DFX)},
            {"wglSwapIntervalEXT", reinterpret_cast<PROC>(&vk_wglSwapIntervalEXT)},
            {"glGenQueriesARB", reinterpret_cast<PROC>(&vk_glGenQueriesARB)},
            {"glDeleteQueriesARB", reinterpret_cast<PROC>(&vk_glDeleteQueriesARB)},
            {"glIsQueryARB", reinterpret_cast<PROC>(&vk_glIsQueryARB)},
            {"glBeginQueryARB", reinterpret_cast<PROC>(&vk_glBeginQueryARB)},
            {"glEndQueryARB", reinterpret_cast<PROC>(&vk_glEndQueryARB)},
            {"glGetQueryivARB", reinterpret_cast<PROC>(&vk_glGetQueryivARB)},
            {"glGetQueryObjectivARB", reinterpret_cast<PROC>(&vk_glGetQueryObjectivARB)},
            {"glGetQueryObjectuivARB", reinterpret_cast<PROC>(&vk_glGetQueryObjectuivARB)},
            {"glDeleteObjectARB", reinterpret_cast<PROC>(&vk_glDeleteObjectARB)},
            {"glDeleteShader", reinterpret_cast<PROC>(&vk_glDeleteObjectARB)},
            {"glDeleteProgram", reinterpret_cast<PROC>(&vk_glDeleteObjectARB)},
            {"glDetachObjectARB", reinterpret_cast<PROC>(&vk_glDetachObjectARB)},
            {"glDetachShader", reinterpret_cast<PROC>(&vk_glDetachObjectARB)},
            {"glCreateShaderObjectARB", reinterpret_cast<PROC>(&vk_glCreateShaderObjectARB)},
            {"glCreateShader", reinterpret_cast<PROC>(&vk_glCreateShaderObjectARB)},
            {"glShaderSourceARB", reinterpret_cast<PROC>(&vk_glShaderSourceARB)},
            {"glShaderSource", reinterpret_cast<PROC>(&vk_glShaderSourceARB)},
            {"glCompileShaderARB", reinterpret_cast<PROC>(&vk_glCompileShaderARB)},
            {"glCompileShader", reinterpret_cast<PROC>(&vk_glCompileShaderARB)},
            {"glCreateProgramObjectARB", reinterpret_cast<PROC>(&vk_glCreateProgramObjectARB)},
            {"glCreateProgram", reinterpret_cast<PROC>(&vk_glCreateProgramObjectARB)},
            {"glAttachObjectARB", reinterpret_cast<PROC>(&vk_glAttachObjectARB)},
            {"glAttachShader", reinterpret_cast<PROC>(&vk_glAttachObjectARB)},
            {"glLinkProgramARB", reinterpret_cast<PROC>(&vk_glLinkProgramARB)},
            {"glLinkProgram", reinterpret_cast<PROC>(&vk_glLinkProgramARB)},
            {"glUseProgramObjectARB", reinterpret_cast<PROC>(&vk_glUseProgramObjectARB)},
            {"glUseProgram", reinterpret_cast<PROC>(&vk_glUseProgramObjectARB)},
            {"glValidateProgramARB", reinterpret_cast<PROC>(&vk_glValidateProgramARB)},
            {"glValidateProgram", reinterpret_cast<PROC>(&vk_glValidateProgramARB)},
            {"glUniform1fARB", reinterpret_cast<PROC>(&vk_glUniform1fARB)},
            {"glUniform1f", reinterpret_cast<PROC>(&vk_glUniform1fARB)},
            {"glUniform1iARB", reinterpret_cast<PROC>(&vk_glUniform1iARB)},
            {"glUniform1i", reinterpret_cast<PROC>(&vk_glUniform1iARB)},
            {"glGetObjectParameterivARB", reinterpret_cast<PROC>(&vk_glGetObjectParameterivARB)},
            {"glGetShaderiv", reinterpret_cast<PROC>(&vk_glGetObjectParameterivARB)},
            {"glGetProgramiv", reinterpret_cast<PROC>(&vk_glGetObjectParameterivARB)},
            {"glGetInfoLogARB", reinterpret_cast<PROC>(&vk_glGetInfoLogARB)},
            {"glGetShaderInfoLog", reinterpret_cast<PROC>(&vk_glGetInfoLogARB)},
            {"glGetProgramInfoLog", reinterpret_cast<PROC>(&vk_glGetInfoLogARB)},
            {"glGetUniformLocationARB", reinterpret_cast<PROC>(&vk_glGetUniformLocationARB)},
            {"glGetUniformLocation", reinterpret_cast<PROC>(&vk_glGetUniformLocationARB)},
            {"glIsRenderbufferEXT", reinterpret_cast<PROC>(&vk_glIsRenderbufferEXT)},
            {"glBindRenderbufferEXT", reinterpret_cast<PROC>(&vk_glBindRenderbufferEXT)},
            {"glDeleteRenderbuffersEXT", reinterpret_cast<PROC>(&vk_glDeleteRenderbuffersEXT)},
            {"glGenRenderbuffersEXT", reinterpret_cast<PROC>(&vk_glGenRenderbuffersEXT)},
            {"glRenderbufferStorageEXT", reinterpret_cast<PROC>(&vk_glRenderbufferStorageEXT)},
            {"glGetRenderbufferParameterivEXT", reinterpret_cast<PROC>(&vk_glGetRenderbufferParameterivEXT)},
            {"glIsFramebufferEXT", reinterpret_cast<PROC>(&vk_glIsFramebufferEXT)},
            {"glBindFramebufferEXT", reinterpret_cast<PROC>(&vk_glBindFramebufferEXT)},
            {"glDeleteFramebuffersEXT", reinterpret_cast<PROC>(&vk_glDeleteFramebuffersEXT)},
            {"glGenFramebuffersEXT", reinterpret_cast<PROC>(&vk_glGenFramebuffersEXT)},
            {"glCheckFramebufferStatusEXT", reinterpret_cast<PROC>(&vk_glCheckFramebufferStatusEXT)},
            {"glFramebufferTexture1DEXT", reinterpret_cast<PROC>(&vk_glFramebufferTexture1DEXT)},
            {"glFramebufferTexture2DEXT", reinterpret_cast<PROC>(&vk_glFramebufferTexture2DEXT)},
            {"glFramebufferTexture3DEXT", reinterpret_cast<PROC>(&vk_glFramebufferTexture3DEXT)},
            {"glFramebufferRenderbufferEXT", reinterpret_cast<PROC>(&vk_glFramebufferRenderbufferEXT)},
            {"glGetFramebufferAttachmentParameterivEXT", reinterpret_cast<PROC>(&vk_glGetFramebufferAttachmentParameterivEXT)},
            {"glGenerateMipmapEXT", reinterpret_cast<PROC>(&vk_glGenerateMipmapEXT)},
            {"glBlitFramebufferEXT", reinterpret_cast<PROC>(&vk_glBlitFramebufferEXT)},
        };

        for (const auto& entry : procs)
        {
            if (name == entry.name)
            {
                return entry.fn;
            }
        }
        return nullptr;
    }

    bool init_qgl_vulkan(uintptr_t engine_base)
    {
        if (!ctx.vk.load())
        {
            return false;
        }

        ctx.engine_base = (engine_base != 0) ? engine_base : preferred_base;
        const ptrdiff_t slide = static_cast<ptrdiff_t>(ctx.engine_base) -
                                static_cast<ptrdiff_t>(preferred_base);

        for (const auto& entry : qgl_slot_table)
        {
            if (resolve_implemented_qgl_func(entry.name) == nullptr &&
                get_noop_for_arg_bytes(static_cast<uint16_t>(entry.arg_bytes)) == nullptr)
            {
                return false;
            }
        }

        // mirror QGL_Init markers so the engine routes rendering through the populated qgl table
        auto* hinst_opengl = reinterpret_cast<HMODULE*>(
            static_cast<uintptr_t>(va_hinst_opengl + slide));
        *hinst_opengl = GetModuleHandleW(nullptr);

        auto* driver_type = reinterpret_cast<int*>(
            static_cast<uintptr_t>(va_glconfig_driver_type + slide));
        *driver_type = 1;

        for (const auto& entry : qgl_slot_table)
        {
            void* fn = resolve_implemented_qgl_func(entry.name);
            if (fn == nullptr)
            {
                fn = get_noop_for_arg_bytes(static_cast<uint16_t>(entry.arg_bytes));
            }
            auto* p0 = reinterpret_cast<void**>(static_cast<uintptr_t>(entry.slot0_va + slide));
            *p0 = fn;
            if (entry.slot1_va != 0)
            {
                auto* p1 = reinterpret_cast<void**>(static_cast<uintptr_t>(entry.slot1_va + slide));
                *p1 = fn;
            }
        }

        ctx.active = true;
        return true;
    }

    void shutdown_qgl_vulkan(uintptr_t engine_base)
    {
        const uintptr_t base = (engine_base != 0) ? engine_base : ctx.engine_base;
        const ptrdiff_t slide = static_cast<ptrdiff_t>(base) -
                                static_cast<ptrdiff_t>(preferred_base);

        // flush pending gpu work and save pipeline cache before clearing qgl slots
        if (ctx.context_created)
        {
            flush_pending_commands_and_wait();
            save_pipeline_cache();
        }

        auto* hinst_opengl = reinterpret_cast<HMODULE*>(
            static_cast<uintptr_t>(va_hinst_opengl + slide));
        *hinst_opengl = nullptr;

        auto* driver_type = reinterpret_cast<int*>(
            static_cast<uintptr_t>(va_glconfig_driver_type + slide));
        *driver_type = 0;

        for (const auto& entry : qgl_slot_table)
        {
            auto* p0 = reinterpret_cast<void**>(static_cast<uintptr_t>(entry.slot0_va + slide));
            *p0 = nullptr;
            if (entry.slot1_va != 0)
            {
                auto* p1 = reinterpret_cast<void**>(static_cast<uintptr_t>(entry.slot1_va + slide));
                *p1 = nullptr;
            }
        }
        ctx.active = false;
    }

    bool is_vulkan_backend_active()
    {
        return ctx.active;
    }
}
