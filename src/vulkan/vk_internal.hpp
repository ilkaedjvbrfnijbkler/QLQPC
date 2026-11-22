#pragma once

#include "vulkan/qgl_slots.hpp"
#include "vulkan/vk_c_abi.hpp"
#include "vulkan/vk_spirv.hpp"
#include "vulkan/vulkan_backend.hpp"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace qlqpc::vulkan
{
    inline constexpr uint32_t GL_NO_ERROR = 0x0000;
    inline constexpr uint32_t GL_ZERO = 0x0000;
    inline constexpr uint32_t GL_ONE = 0x0001;
    inline constexpr uint32_t GL_POINTS = 0x0000;
    inline constexpr uint32_t GL_LINES = 0x0001;
    inline constexpr uint32_t GL_LINE_LOOP = 0x0002;
    inline constexpr uint32_t GL_LINE_STRIP = 0x0003;
    inline constexpr uint32_t GL_TRIANGLES = 0x0004;
    inline constexpr uint32_t GL_TRIANGLE_STRIP = 0x0005;
    inline constexpr uint32_t GL_TRIANGLE_FAN = 0x0006;
    inline constexpr uint32_t GL_QUADS = 0x0007;
    inline constexpr uint32_t GL_QUAD_STRIP = 0x0008;
    inline constexpr uint32_t GL_POLYGON = 0x0009;

    inline constexpr uint32_t GL_DEPTH_BUFFER_BIT = 0x00000100;
    inline constexpr uint32_t GL_STENCIL_BUFFER_BIT = 0x00000400;
    inline constexpr uint32_t GL_COLOR_BUFFER_BIT = 0x00004000;

    inline constexpr uint32_t GL_NEVER = 0x0200;
    inline constexpr uint32_t GL_LESS = 0x0201;
    inline constexpr uint32_t GL_EQUAL = 0x0202;
    inline constexpr uint32_t GL_LEQUAL = 0x0203;
    inline constexpr uint32_t GL_GREATER = 0x0204;
    inline constexpr uint32_t GL_NOTEQUAL = 0x0205;
    inline constexpr uint32_t GL_GEQUAL = 0x0206;
    inline constexpr uint32_t GL_ALWAYS = 0x0207;

    inline constexpr uint32_t GL_SRC_COLOR = 0x0300;
    inline constexpr uint32_t GL_ONE_MINUS_SRC_COLOR = 0x0301;
    inline constexpr uint32_t GL_SRC_ALPHA = 0x0302;
    inline constexpr uint32_t GL_ONE_MINUS_SRC_ALPHA = 0x0303;
    inline constexpr uint32_t GL_DST_ALPHA = 0x0304;
    inline constexpr uint32_t GL_ONE_MINUS_DST_ALPHA = 0x0305;
    inline constexpr uint32_t GL_DST_COLOR = 0x0306;
    inline constexpr uint32_t GL_ONE_MINUS_DST_COLOR = 0x0307;

    inline constexpr uint32_t GL_FRONT = 0x0404;
    inline constexpr uint32_t GL_BACK = 0x0405;
    inline constexpr uint32_t GL_FRONT_AND_BACK = 0x0408;

    inline constexpr uint32_t GL_CW = 0x0900;
    inline constexpr uint32_t GL_CCW = 0x0901;

    inline constexpr uint32_t GL_CULL_FACE = 0x0B44;
    inline constexpr uint32_t GL_DEPTH_TEST = 0x0B71;
    inline constexpr uint32_t GL_DEPTH_WRITEMASK = 0x0B72;
    inline constexpr uint32_t GL_DEPTH_FUNC = 0x0B74;
    inline constexpr uint32_t GL_STENCIL_TEST = 0x0B90;
    inline constexpr uint32_t GL_ALPHA_TEST = 0x0BC0;
    inline constexpr uint32_t GL_BLEND = 0x0BE2;
    inline constexpr uint32_t GL_SCISSOR_TEST = 0x0C11;
    inline constexpr uint32_t GL_UNPACK_ROW_LENGTH = 0x0CF2;
    inline constexpr uint32_t GL_UNPACK_SKIP_ROWS = 0x0CF3;
    inline constexpr uint32_t GL_UNPACK_SKIP_PIXELS = 0x0CF4;
    inline constexpr uint32_t GL_UNPACK_ALIGNMENT = 0x0CF5;
    inline constexpr uint32_t GL_PACK_ALIGNMENT = 0x0D05;
    inline constexpr uint32_t GL_MAX_TEXTURE_SIZE = 0x0D33;
    inline constexpr uint32_t GL_TEXTURE_2D = 0x0DE1;

    inline constexpr uint32_t GL_UNSIGNED_BYTE = 0x1401;
    inline constexpr uint32_t GL_UNSIGNED_SHORT = 0x1403;
    inline constexpr uint32_t GL_UNSIGNED_INT = 0x1405;
    inline constexpr uint32_t GL_FLOAT = 0x1406;

    inline constexpr uint32_t GL_MODELVIEW = 0x1700;
    inline constexpr uint32_t GL_PROJECTION = 0x1701;
    inline constexpr uint32_t GL_TEXTURE = 0x1702;

    inline constexpr uint32_t GL_STENCIL_INDEX = 0x1901;
    inline constexpr uint32_t GL_DEPTH_COMPONENT = 0x1902;
    inline constexpr uint32_t GL_ALPHA = 0x1906;
    inline constexpr uint32_t GL_RGB = 0x1907;
    inline constexpr uint32_t GL_RGBA = 0x1908;
    inline constexpr uint32_t GL_BGR_EXT = 0x80E0;
    inline constexpr uint32_t GL_BGRA_EXT = 0x80E1;

    inline constexpr uint32_t GL_LINE = 0x1B01;
    inline constexpr uint32_t GL_FILL = 0x1B02;

    inline constexpr uint32_t GL_KEEP = 0x1E00;
    inline constexpr uint32_t GL_REPLACE = 0x1E01;
    inline constexpr uint32_t GL_INCR = 0x1E02;
    inline constexpr uint32_t GL_DECR = 0x1E03;
    inline constexpr uint32_t GL_INVERT = 0x150A;
    inline constexpr uint32_t GL_INCR_WRAP = 0x8507;
    inline constexpr uint32_t GL_DECR_WRAP = 0x8508;

    inline constexpr uint32_t GL_VENDOR = 0x1F00;
    inline constexpr uint32_t GL_RENDERER = 0x1F01;
    inline constexpr uint32_t GL_VERSION = 0x1F02;
    inline constexpr uint32_t GL_EXTENSIONS = 0x1F03;

    inline constexpr uint32_t GL_MODULATE = 0x2100;
    inline constexpr uint32_t GL_DECAL = 0x2101;
    inline constexpr uint32_t GL_TEXTURE_ENV_MODE = 0x2200;
    inline constexpr uint32_t GL_TEXTURE_ENV = 0x2300;
    inline constexpr uint32_t GL_ADD = 0x0104;

    inline constexpr uint32_t GL_NEAREST = 0x2600;
    inline constexpr uint32_t GL_LINEAR = 0x2601;
    inline constexpr uint32_t GL_NEAREST_MIPMAP_NEAREST = 0x2700;
    inline constexpr uint32_t GL_NEAREST_MIPMAP_LINEAR = 0x2702;
    inline constexpr uint32_t GL_LINEAR_MIPMAP_LINEAR = 0x2703;

    inline constexpr uint32_t GL_TEXTURE_MAG_FILTER = 0x2800;
    inline constexpr uint32_t GL_TEXTURE_MIN_FILTER = 0x2801;
    inline constexpr uint32_t GL_TEXTURE_WRAP_S = 0x2802;
    inline constexpr uint32_t GL_TEXTURE_WRAP_T = 0x2803;
    inline constexpr uint32_t GL_CLAMP = 0x2900;
    inline constexpr uint32_t GL_REPEAT = 0x2901;
    inline constexpr uint32_t GL_CLAMP_TO_EDGE = 0x812F;

    inline constexpr uint32_t GL_POLYGON_OFFSET_LINE = 0x2A02;
    inline constexpr uint32_t GL_POLYGON_OFFSET_FILL = 0x8037;
    inline constexpr uint32_t GL_VERTEX_ARRAY = 0x8074;
    inline constexpr uint32_t GL_COLOR_ARRAY = 0x8076;
    inline constexpr uint32_t GL_TEXTURE_COORD_ARRAY = 0x8078;

    inline constexpr uint32_t GL_CLIP_PLANE0 = 0x3000;
    inline constexpr uint32_t GL_TEXTURE0_ARB = 0x84C0;
    inline constexpr uint32_t GL_MAX_TEXTURE_UNITS_ARB = 0x84E2;
    inline constexpr uint32_t GL_TEXTURE_RECTANGLE_ARB = 0x84F5;
    inline constexpr uint32_t GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB = 0x84F8;
    inline constexpr uint32_t GL_FRAGMENT_SHADER_ARB = 0x8B30;
    inline constexpr uint32_t GL_VERTEX_SHADER_ARB = 0x8B31;

    inline constexpr uint32_t GL_INFO_LOG_LENGTH = 0x8B84;
    inline constexpr uint32_t GL_FRAMEBUFFER_COMPLETE_EXT = 0x8CD5;
    inline constexpr uint32_t GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT = 0x8CD6;
    inline constexpr uint32_t GL_COLOR_ATTACHMENT0_EXT = 0x8CE0;
    inline constexpr uint32_t GL_DEPTH_ATTACHMENT_EXT = 0x8D00;
    inline constexpr uint32_t GL_FRAMEBUFFER_EXT = 0x8D40;
    inline constexpr uint32_t GL_RENDERBUFFER_EXT = 0x8D41;
    inline constexpr uint32_t GL_QUERY_RESULT_AVAILABLE = 0x8867;

    // engine global virtual addresses at preferred base 0x00400000
    inline constexpr uintptr_t preferred_base = 0x00400000u;
    inline constexpr uintptr_t va_hinst_opengl = 0x016E410Cu;
    inline constexpr uintptr_t va_glconfig_driver_type = 0x01743BB4u;
    inline constexpr uintptr_t va_glconfig_width = 0x01743BCCu;
    inline constexpr uintptr_t va_glconfig_height = 0x01743BD0u;

    struct gl_vk_vertex
    {
        float pos[3];
        uint8_t color[4];
        float uv0[2];
        float uv1[2];
    };
    static_assert(sizeof(gl_vk_vertex) == 32, "gl_vk_vertex must be 32 bytes");

    struct pipeline_state_key
    {
        post_process_kind post{post_process_kind::none};
        frag_combiner combiner{frag_combiner::single_modulate};
        alpha_test_mode alpha_test{alpha_test_mode::none};
        uint8_t clip_plane{0};
        uint8_t topology{static_cast<uint8_t>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)};
        uint8_t polygon_mode{static_cast<uint8_t>(VK_POLYGON_MODE_FILL)};
        uint8_t cull_mode{VK_CULL_MODE_NONE};
        uint8_t front_face{static_cast<uint8_t>(VK_FRONT_FACE_COUNTER_CLOCKWISE)};
        uint8_t depth_test{0};
        uint8_t depth_write{1};
        uint8_t depth_func{static_cast<uint8_t>(VK_COMPARE_OP_LESS_OR_EQUAL)};
        uint8_t blend_enable{0};
        uint8_t src_blend{static_cast<uint8_t>(VK_BLEND_FACTOR_ONE)};
        uint8_t dst_blend{static_cast<uint8_t>(VK_BLEND_FACTOR_ZERO)};
        uint8_t color_mask{0x0Fu};
        uint8_t depth_bias{0};
        uint8_t stencil_test{0};
        uint8_t stencil_func{static_cast<uint8_t>(VK_COMPARE_OP_ALWAYS)};
        uint8_t stencil_fail{static_cast<uint8_t>(VK_STENCIL_OP_KEEP)};
        uint8_t stencil_zfail{static_cast<uint8_t>(VK_STENCIL_OP_KEEP)};
        uint8_t stencil_zpass{static_cast<uint8_t>(VK_STENCIL_OP_KEEP)};
        uint8_t is_present_pass{0};

        bool operator==(const pipeline_state_key&) const = default;
    };

    struct pipeline_state_key_hash
    {
        size_t operator()(const pipeline_state_key& k) const
        {
            const auto* bytes = reinterpret_cast<const uint8_t*>(&k);
            size_t h = 2166136261u;
            for (size_t i = 0; i < sizeof(pipeline_state_key); ++i)
            {
                h ^= bytes[i];
                h *= 16777619u;
            }
            return h;
        }
    };

    struct frag_shader_desc_hash
    {
        size_t operator()(const frag_shader_desc& d) const
        {
            size_t h = static_cast<size_t>(d.post);
            h = (h * 31u) + static_cast<size_t>(d.combiner);
            h = (h * 31u) + static_cast<size_t>(d.alpha_test);
            h = (h * 31u) + (d.clip_plane_enabled ? 1u : 0u);
            return h;
        }
    };

    struct vk_texture_object
    {
        uint32_t id{0};
        uint32_t target{GL_TEXTURE_2D};
        bool is_rectangle{false};
        bool is_fbo_attachment{false};
        uint32_t width{0};
        uint32_t height{0};
        uint32_t mip_levels{1};
        uint32_t max_uploaded_level{0};
        VkImage image{vk_null_handle};
        VkDeviceMemory memory{vk_null_handle};
        VkImageView view{vk_null_handle};
        VkImageLayout current_layout{VK_IMAGE_LAYOUT_UNDEFINED};

        uint32_t min_filter{GL_LINEAR};
        uint32_t mag_filter{GL_LINEAR};
        uint32_t wrap_s{GL_REPEAT};
        uint32_t wrap_t{GL_REPEAT};
        VkSampler sampler{vk_null_handle};
    };

    struct vk_renderbuffer_object
    {
        uint32_t id{0};
        uint32_t width{0};
        uint32_t height{0};
        VkImage image{vk_null_handle};
        VkDeviceMemory memory{vk_null_handle};
        VkImageView view{vk_null_handle};
    };

    struct vk_framebuffer_object
    {
        uint32_t id{0};
        uint32_t color_tex_id{0};
        uint32_t depth_rb_id{0};
        uint32_t width{0};
        uint32_t height{0};
        VkFramebuffer fb{vk_null_handle};
    };

    struct glsl_shader_object
    {
        uint32_t handle{0};
        uint32_t shader_type{0};
        bool is_program{false};
        post_process_kind kind{post_process_kind::none};
        bool is_blur_vertical{false};
        float gamma_recip{1.0f};
        float overbright{1.0f};
        float contrast{1.0f};
        float bright_threshold{0.5f};
        float bloom_saturation{1.0f};
        float scene_saturation{1.0f};
        float bloom_intensity{0.75f};
        float scene_intensity{1.0f};
    };

    struct mat4
    {
        float m[16];

        static mat4 identity()
        {
            mat4 r{};
            r.m[0] = 1.0f;
            r.m[5] = 1.0f;
            r.m[10] = 1.0f;
            r.m[15] = 1.0f;
            return r;
        }

        static mat4 multiply(const mat4& a, const mat4& b)
        {
            mat4 out{};
            for (int c = 0; c < 4; ++c)
            {
                for (int r = 0; r < 4; ++r)
                {
                    out.m[c * 4 + r] =
                        a.m[0 * 4 + r] * b.m[c * 4 + 0] +
                        a.m[1 * 4 + r] * b.m[c * 4 + 1] +
                        a.m[2 * 4 + r] * b.m[c * 4 + 2] +
                        a.m[3 * 4 + r] * b.m[c * 4 + 3];
                }
            }
            return out;
        }
    };

    // invert a 4x4 matrix; returns false if singular
    inline bool invert_mat4(const float m[16], float inv_out[16])
    {
        float inv[16];
        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
                 m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
                 m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
                 m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
                  m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
                 m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
                 m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
                 m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
                  m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] +
                 m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
                 m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
                  m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
                  m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
                 m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
                 m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
                  m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
                  m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        if (std::abs(det) < 1e-12f)
        {
            return false;
        }
        det = 1.0f / det;
        for (int i = 0; i < 16; ++i)
        {
            inv_out[i] = inv[i] * det;
        }
        return true;
    }

    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4324) // suppress padding warning for cache-line aligned worker atomics
    #endif
    struct vulkan_context
    {
        uintptr_t engine_base{preferred_base};
        bool active{false};
        bool context_created{false};
        vulkan_dispatch vk{};

        VkInstance instance{nullptr};
        VkPhysicalDevice phys_dev{nullptr};
        VkPhysicalDeviceProperties phys_props{};
        VkPhysicalDeviceMemoryProperties mem_props{};
        uint32_t graphics_queue_family{0};
        VkDevice device{nullptr};
        VkQueue queue{nullptr};

        std::string gpu_name_str{"Vulkan GPU"};
        std::string gl_renderer_str{"Vulkan GPU"};
        std::string gl_vendor_str{"Vulkan"};
        std::string gl_version_str{"2.1 QLQPC Vulkan 1.0"};
        std::string gl_extensions_str
        {
            "GL_S3_s3tc GL_EXT_texture_compression_s3tc GL_ARB_multitexture "
            "GL_EXT_texture_env_add GL_EXT_compiled_vertex_array WGL_3DFX_gamma_control "
            "GL_ARB_occlusion_query GL_ARB_shader_objects GL_ARB_vertex_shader "
            "GL_ARB_fragment_shader GL_ARB_texture_rectangle GL_EXT_framebuffer_object "
            "GL_ARB_texture_non_power_of_two WGL_EXT_swap_control"
        };

        // window and swapchain
        HWND hwnd{nullptr};
        HDC hdc{nullptr};
        uint32_t vid_width{1920};
        uint32_t vid_height{1080};
        int swap_interval{0};
        VkSurfaceKHR surface{vk_null_handle};
        VkSwapchainKHR swapchain{vk_null_handle};
        VkFormat swapchain_format{VK_FORMAT_B8G8R8A8_UNORM};
        int32_t swapchain_color_space{0};
        VkExtent2D swapchain_extent{0, 0};
        bool swapchain_needs_recreate{false};
        bool swapchain_out_of_date{false};
        bool last_window_focused{true};
        bool suboptimal_recreate_attempted{false};
        std::vector<VkImage> swapchain_images{};
        std::vector<VkImageView> swapchain_views{};
        std::vector<VkFramebuffer> swapchain_fbs{};
        VkRenderPass present_render_pass{vk_null_handle};
        VkFormat present_render_pass_format{VK_FORMAT_B8G8R8A8_UNORM};

        // scene render pass and default fbo0
        VkFormat depth_stencil_format{VK_FORMAT_D24_UNORM_S8_UINT};
        VkRenderPass scene_render_pass{vk_null_handle};
        VkRenderPass scene_render_pass_clear_c{vk_null_handle};
        VkRenderPass scene_render_pass_clear_d{vk_null_handle};
        VkRenderPass scene_render_pass_clear_cd{vk_null_handle};
        VkImage fbo0_color_img{vk_null_handle};
        VkDeviceMemory fbo0_color_mem{vk_null_handle};
        VkImageView fbo0_color_view{vk_null_handle};
        VkImage fbo0_ds_img{vk_null_handle};
        VkDeviceMemory fbo0_ds_mem{vk_null_handle};
        VkImageView fbo0_ds_view{vk_null_handle};
        VkFramebuffer fbo0_fb{vk_null_handle};
        uint32_t fbo0_width{0};
        uint32_t fbo0_height{0};
        uint32_t last_client_w{0};
        uint32_t last_client_h{0};
        uint32_t cached_client_w{0};
        uint32_t cached_client_h{0};
        bool window_state_cached_this_frame{false};
        bool cached_window_valid{false};
        bool cached_window_minimized{false};
        bool cached_window_focused{true};
        HWND last_polled_hwnd{nullptr};
        int64_t last_window_poll_qpc{0};
        int64_t window_poll_interval_qpc{0};

        // command recording and synchronization
        static constexpr size_t frames_in_flight = 2;
        static constexpr uint32_t sem_ring_size = 8;
        VkCommandPool frame_cmd_pools[frames_in_flight]{};
        VkCommandPool upload_cmd_pool{vk_null_handle};
        VkCommandBuffer frame_cmds[frames_in_flight]{};
        VkFence frame_fences[frames_in_flight]{};
        VkSemaphore img_avail_sems[sem_ring_size]{};
        VkSemaphore render_done_sems[sem_ring_size]{};
        uint32_t next_sem_idx{0};
        VkDescriptorPool desc_pools[frames_in_flight]{};
        size_t cur_frame{0};
        bool cmd_recording{false};
        bool render_pass_active{false};
        bool frames_in_flight_busy{false};
        bool frame_fence_pending[frames_in_flight]{false, false};
        VkCommandBuffer active_cmd{nullptr};

        struct present_job
        {
            VkCommandBuffer cmd{nullptr};
            VkFence fence{vk_null_handle};
            VkSemaphore wait_sem{vk_null_handle};
            VkSemaphore signal_sem{vk_null_handle};
            VkFlags wait_stage{0x1000u};
            size_t frame_idx{0};
            uint32_t sc_idx{0};
            VkResult acq_result{VK_SUCCESS};
            uint32_t next_sem_idx{0};
            VkSemaphore next_acquire_sem{vk_null_handle};
        };

        alignas(64) std::atomic<uint32_t> present_worker_state{0};
        alignas(64) std::atomic<bool> present_worker_sleeping{false};
        alignas(64) std::atomic<bool> main_waiting_on_present_worker{false};
        std::atomic<bool> window_focused_atomic{true};
        HANDLE present_worker_thread{nullptr};
        HANDLE present_worker_wake_event{nullptr};
        HANDLE present_worker_done_event{nullptr};
        DWORD last_main_thread_cpu{~0u};
        present_job pending_present_job{};

        bool has_present_result{false};
        VkResult last_submit_result{VK_SUCCESS};
        VkResult last_present_result{VK_SUCCESS};
        VkResult last_present_acq_result{VK_SUCCESS};

        bool has_preacquired_image{false};
        uint32_t preacquired_sc_idx{0};
        uint32_t preacquired_sem_idx{0};
        VkResult preacquired_result{VK_SUCCESS};

        VkCommandBuffer upload_cmd{nullptr};
        VkFence upload_fence{vk_null_handle};

        // persistent mapped ring buffers per frame in flight
        static constexpr VkDeviceSize vbo_size = 16ull * 1024ull * 1024ull;
        static constexpr VkDeviceSize ibo_size = 8ull * 1024ull * 1024ull;
        VkBuffer vbo_bufs[frames_in_flight]{};
        VkDeviceMemory vbo_mems[frames_in_flight]{};
        uint8_t* vbo_mapped[frames_in_flight]{};
        VkDeviceSize vbo_offset{0};

        VkBuffer ibo_bufs[frames_in_flight]{};
        VkDeviceMemory ibo_mems[frames_in_flight]{};
        uint8_t* ibo_mapped[frames_in_flight]{};
        VkDeviceSize ibo_offset{0};

        static constexpr VkDeviceSize staging_size = 32ull * 1024ull * 1024ull;
        VkBuffer staging_bufs[frames_in_flight]{};
        VkDeviceMemory staging_mems[frames_in_flight]{};
        uint8_t* staging_mapped[frames_in_flight]{};
        VkDeviceSize staging_offset{0};

        // pipeline and descriptor layouts
        VkDescriptorSetLayout desc_set_layout{vk_null_handle};
        VkPipelineLayout pipeline_layout{vk_null_handle};
        VkPipelineCache pipeline_cache{vk_null_handle};
        bool pipeline_cache_dirty{false};
        VkShaderModule vert_shader_mod{vk_null_handle};
        std::unordered_map<frag_shader_desc, VkShaderModule, frag_shader_desc_hash> frag_shader_mods{};
        std::unordered_map<pipeline_state_key, VkPipeline, pipeline_state_key_hash> pipelines{};
        VkPipeline bound_pipeline{vk_null_handle};

        // textures and hardware gamma lut
        static constexpr uint32_t max_texture_id = 131072u;
        uint32_t next_tex_id{1};
        std::vector<vk_texture_object> textures{};
        vk_texture_object white_tex{};
        vk_texture_object gamma_lut_tex{};
        uint16_t gamma_ramp[3][256]{};
        bool gamma_identity{true};

        // offscreen fbos and renderbuffers
        uint32_t next_fbo_id{1};
        uint32_t next_rbo_id{1};
        uint32_t next_query_id{1};
        std::unordered_map<uint32_t, vk_framebuffer_object> fbos{};
        std::unordered_map<uint32_t, vk_renderbuffer_object> rbos{};
        uint32_t bound_fbo_id{0};
        uint32_t bound_rbo_id{0};
        uint32_t cur_fb_w{1920};
        uint32_t cur_fb_h{1080};
        VkFramebuffer cur_vk_fb{vk_null_handle};
        VkImage cur_color_img{vk_null_handle};

        // glsl shader and program objects
        uint32_t next_glsl_handle{1};
        std::unordered_map<uint32_t, glsl_shader_object> glsl_objects{};
        uint32_t active_program{0};

        // opengl state machine
        uint32_t matrix_mode{GL_MODELVIEW};
        std::vector<mat4> mv_stack{mat4::identity()};
        std::vector<mat4> proj_stack{mat4::identity()};
        std::vector<mat4> tex_stack{mat4::identity()};

        uint32_t active_tex_unit{0};
        uint32_t client_active_tex_unit{0};
        uint32_t bound_tex_2d[2]{0, 0};
        uint32_t bound_tex_rect[2]{0, 0};
        bool tex_2d_enabled[2]{false, false};
        bool tex_rect_enabled[2]{false, false};
        uint32_t tex_env_mode[2]{GL_MODULATE, GL_MODULATE};

        struct sampler_key
        {
            uint32_t min_filter;
            uint32_t mag_filter;
            uint32_t wrap_s;
            uint32_t wrap_t;
            uint32_t max_lod_level;

            bool operator==(const sampler_key&) const = default;
        };

        struct sampler_key_hash
        {
            size_t operator()(const sampler_key& k) const
            {
                size_t h = k.min_filter;
                h = (h * 31u) + k.mag_filter;
                h = (h * 31u) + k.wrap_s;
                h = (h * 31u) + k.wrap_t;
                h = (h * 31u) + k.max_lod_level;
                return h;
            }
        };
        std::unordered_map<sampler_key, VkSampler, sampler_key_hash> sampler_cache{};

        float clear_color[4]{0.0f, 0.0f, 0.0f, 1.0f};
        float clear_depth{1.0f};
        uint32_t clear_stencil{0};

        int viewport[4]{0, 0, 1920, 1080};
        int scissor[4]{0, 0, 1920, 1080};
        float depth_range[2]{0.0f, 1.0f};
        bool scissor_enabled{false};

        bool cull_enabled{false};
        uint32_t cull_face_mode{GL_BACK};
        uint32_t front_face_mode{GL_CCW};
        uint32_t polygon_mode{GL_FILL};
        bool poly_offset_enabled{false};
        float poly_offset_factor{0.0f};
        float poly_offset_units{0.0f};

        bool depth_test_enabled{false};
        bool depth_write_enabled{true};
        uint32_t depth_func{GL_LEQUAL};

        bool blend_enabled{false};
        uint32_t blend_src{GL_ONE};
        uint32_t blend_dst{GL_ZERO};
        uint8_t color_write_mask{0x0Fu};

        bool alpha_test_enabled{false};
        uint32_t alpha_test_func{GL_GREATER};

        bool stencil_test_enabled{false};
        uint32_t stencil_func{GL_ALWAYS};
        int32_t stencil_ref{0};
        uint32_t stencil_val_mask{0xFFu};
        uint32_t stencil_write_mask{0xFFu};
        uint32_t stencil_fail_op{GL_KEEP};
        uint32_t stencil_zfail_op{GL_KEEP};
        uint32_t stencil_zpass_op{GL_KEEP};

        bool clip_plane_enabled{false};
        float clip_plane_eye[4]{0.0f, 0.0f, 0.0f, 1.0f};

        int32_t unpack_alignment{4};
        int32_t unpack_row_length{0};
        int32_t unpack_skip_pixels{0};
        int32_t unpack_skip_rows{0};
        int32_t pack_alignment{4};

        struct client_pixel_store_backup
        {
            int32_t alignment{4};
            int32_t row_length{0};
            int32_t skip_pixels{0};
            int32_t skip_rows{0};
            int32_t pack_alignment{4};
        };
        std::vector<client_pixel_store_backup> client_attrib_stack{};

        // client vertex arrays
        bool va_vertex_enabled{false};
        bool va_color_enabled{false};
        bool va_texcoord_enabled[2]{false, false};
        int32_t va_vertex_size{3};
        int32_t va_vertex_stride{0};
        const void* va_vertex_ptr{nullptr};
        int32_t va_color_size{4};
        uint32_t va_color_type{GL_UNSIGNED_BYTE};
        int32_t va_color_stride{0};
        const void* va_color_ptr{nullptr};
        int32_t va_tc_size[2]{2, 2};
        int32_t va_tc_stride[2]{0, 0};
        const void* va_tc_ptr[2]{nullptr, nullptr};

        // immediate mode
        bool in_begin_end{false};
        uint32_t imm_prim_mode{GL_TRIANGLES};
        uint8_t cur_color[4]{255, 255, 255, 255};
        float cur_uv[2][2]{{0.0f, 0.0f}, {0.0f, 0.0f}};
        std::vector<gl_vk_vertex> imm_vertices{};
        std::vector<gl_vk_vertex> scratch_verts{};
        std::vector<uint32_t> scratch_indices{};
        std::vector<uint32_t> scratch_topo_indices{};

        struct bound_pair_key
        {
            VkImageView v0;
            VkSampler s0;
            VkImageView v1;
            VkSampler s1;

            bool operator==(const bound_pair_key&) const = default;
        };

        struct bound_pair_hash
        {
            size_t operator()(const bound_pair_key& k) const
            {
                size_t h = static_cast<size_t>(k.v0);
                h ^= static_cast<size_t>(k.s0) + 0x9e3779b9u + (h << 6) + (h >> 2);
                h ^= static_cast<size_t>(k.v1) + 0x9e3779b9u + (h << 6) + (h >> 2);
                h ^= static_cast<size_t>(k.s1) + 0x9e3779b9u + (h << 6) + (h >> 2);
                return h;
            }
        };
        std::unordered_map<bound_pair_key, VkDescriptorSet, bound_pair_hash> desc_caches[frames_in_flight]{};

        // dirty tracking and cached draw state
        bool pso_key_dirty{true};
        pipeline_state_key cached_pso_key{};
        VkPipeline cached_pipeline{vk_null_handle};
        uint8_t cached_pso_topo{0xFFu};

        bool tex_bindings_dirty{true};
        VkDescriptorSet bound_desc_set{vk_null_handle};
        VkDescriptorSet cached_desc_set{vk_null_handle};
        float cached_inv_w0{1.0f};
        float cached_inv_h0{1.0f};
        float cached_inv_w1{1.0f};
        float cached_inv_h1{1.0f};

        bool vbo_ibo_bound{false};
        bool cmd_vp_dirty{true};
        bool cmd_sc_dirty{true};
        bool cmd_db_dirty{true};
        bool cmd_stencil_dirty{true};

        bool mvp_dirty{true};
        mat4 cached_mvp{};
        bool cmd_pc_valid{false};
        shader_push_constants last_pc{};

        // compiled vertex array lock and rebased ibo slice cache
        bool cva_locked{false};
        uint32_t cva_lock_first{0};
        uint32_t cva_lock_count{0};
        bool cva_ibo_valid{false};
        const void* cva_cached_idx_ptr{nullptr};
        uint32_t cva_cached_idx_count{0};
        uint32_t cva_cached_idx_first_val{0};
        uint32_t cva_cached_idx_last_val{0};
        uint64_t cva_cached_idx_hash{0};
        uint32_t cva_cached_min_idx{0};
        uint32_t cva_cached_v_count{0};
        VkDeviceSize cva_cached_ibo_off{0};
    };
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    extern vulkan_context ctx;

    enum class swapchain_frame_action : uint8_t
    {
        proceed = 0,
        skip_frame = 1,
        recreate_and_proceed = 2,
    };

    struct swapchain_focus_policy_input
    {
        bool window_minimized{false};
        bool window_focused{true};
        bool focus_restored{false};
        bool swapchain_valid{true};
        bool swapchain_out_of_date{false};
        bool swapchain_needs_recreate{false};
        bool suboptimal_recreate_attempted{false};
    };

    inline vk_texture_object* find_texture(uint32_t id)
    {
        if (id == 0 || id >= ctx.textures.size())
        {
            return nullptr;
        }
        auto& t = ctx.textures[id];
        return (t.id != 0) ? &t : nullptr;
    }

    inline vk_texture_object& get_or_create_texture(uint32_t id)
    {
        if (id >= vulkan_context::max_texture_id)
        {
            id = vulkan_context::max_texture_id - 1u;
        }
        if (id >= ctx.textures.size())
        {
            const size_t new_cap = std::max<size_t>(
                static_cast<size_t>(id) + 1u,
                std::max<size_t>(4096u, ctx.textures.size() * 2u));
            ctx.textures.resize(std::min<size_t>(new_cap, vulkan_context::max_texture_id));
        }
        auto& t = ctx.textures[id];
        t.id = id;
        return t;
    }

    inline void invalidate_cmd_draw_state()
    {
        ctx.bound_pipeline = vk_null_handle;
        ctx.bound_desc_set = vk_null_handle;
        ctx.cached_desc_set = vk_null_handle;
        ctx.tex_bindings_dirty = true;
        ctx.vbo_ibo_bound = false;
        ctx.cmd_vp_dirty = true;
        ctx.cmd_sc_dirty = true;
        ctx.cmd_db_dirty = true;
        ctx.cmd_stencil_dirty = true;
        ctx.cmd_pc_valid = false;
    }

    inline void destroy_vk_image_resources(
        VkImageView& view,
        VkImage& image,
        VkDeviceMemory& memory)
    {
        if (view != vk_null_handle)
        {
            ctx.vk.vkDestroyImageView(ctx.device, view, nullptr);
            view = vk_null_handle;
        }
        if (image != vk_null_handle)
        {
            ctx.vk.vkDestroyImage(ctx.device, image, nullptr);
            image = vk_null_handle;
        }
        if (memory != vk_null_handle)
        {
            ctx.vk.vkFreeMemory(ctx.device, memory, nullptr);
            memory = vk_null_handle;
        }
    }

    inline void reset_all_descriptor_caches()
    {
        for (size_t i = 0; i < vulkan_context::frames_in_flight; ++i)
        {
            if (ctx.device != nullptr && ctx.desc_pools[i] != vk_null_handle)
            {
                ctx.vk.vkResetDescriptorPool(ctx.device, ctx.desc_pools[i], 0u);
            }
            ctx.desc_caches[i].clear();
        }
        ctx.bound_desc_set = vk_null_handle;
        ctx.cached_desc_set = vk_null_handle;
        ctx.tex_bindings_dirty = true;
    }

    inline VkRect2D compute_vk_scissor_rect()
    {
        if (!ctx.scissor_enabled)
        {
            return {{0, 0}, {ctx.cur_fb_w, ctx.cur_fb_h}};
        }
        const int64_t fb_w = static_cast<int64_t>(ctx.cur_fb_w);
        const int64_t fb_h = static_cast<int64_t>(ctx.cur_fb_h);
        const int64_t gx = ctx.scissor[0];
        const int64_t gy = ctx.scissor[1];
        const int64_t gw = ctx.scissor[2];
        const int64_t gh = ctx.scissor[3];
        if (gw <= 0 || gh <= 0 || fb_w <= 0 || fb_h <= 0)
        {
            return {{0, 0}, {0u, 0u}};
        }
        const int64_t x0 = std::clamp<int64_t>(gx, 0, fb_w);
        const int64_t x1 = std::clamp<int64_t>(gx + gw, 0, fb_w);
        const int64_t y0 = std::clamp<int64_t>(fb_h - (gy + gh), 0, fb_h);
        const int64_t y1 = std::clamp<int64_t>(fb_h - gy, 0, fb_h);
        if (x1 <= x0 || y1 <= y0)
        {
            return {{0, 0}, {0u, 0u}};
        }
        return
        {
            {static_cast<int32_t>(x0), static_cast<int32_t>(y0)},
            {static_cast<uint32_t>(x1 - x0), static_cast<uint32_t>(y1 - y0)},
        };
    }

    void start_present_worker_if_needed();
    void stop_present_worker();
    void wait_for_present_worker_idle();
    void reset_swapchain_semaphores();

    inline bool execute_immediate_cmd(auto&& record_fn)
    {
        wait_for_present_worker_idle();
        ctx.vk.vkResetFences(ctx.device, 1u, &ctx.upload_fence);
        ctx.vk.vkResetCommandBuffer(ctx.upload_cmd, 0u);
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = 1u;
        if (ctx.vk.vkBeginCommandBuffer(ctx.upload_cmd, &bi) != VK_SUCCESS)
        {
            return false;
        }
        record_fn(ctx.upload_cmd);
        if (ctx.vk.vkEndCommandBuffer(ctx.upload_cmd) != VK_SUCCESS)
        {
            return false;
        }
        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1u;
        si.pCommandBuffers = &ctx.upload_cmd;
        if (ctx.vk.vkQueueSubmit(ctx.queue, 1u, &si, ctx.upload_fence) != VK_SUCCESS)
        {
            return false;
        }
        ctx.vk.vkWaitForFences(ctx.device, 1u, &ctx.upload_fence, VK_TRUE, ~0ull);
        return true;
    }

    uint32_t find_memory_type(uint32_t type_bits, VkFlags req_props);
    bool create_buffer(
        VkDeviceSize size,
        VkFlags usage,
        VkFlags mem_props,
        VkBuffer& out_buf,
        VkDeviceMemory& out_mem);
    bool create_buffer(
        VkDeviceSize size,
        VkFlags usage,
        VkFlags mem_props,
        VkBuffer& out_buf,
        VkDeviceMemory& out_mem,
        void** out_mapped);
    bool create_image_2d(
        uint32_t width,
        uint32_t height,
        uint32_t mip_levels,
        VkFormat format,
        VkFlags usage,
        VkFlags aspect_mask,
        VkImage& out_img,
        VkDeviceMemory& out_mem,
        VkImageView& out_view);
    void cmd_transition_image(
        VkCommandBuffer cmd,
        VkImage image,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        VkFlags aspect_mask = 1u,
        uint32_t mip_levels = 1u,
        uint32_t base_mip = 0u);
    void clear_color_image(
        VkCommandBuffer cmd,
        VkImage image,
        uint32_t mip_levels = 1u,
        VkImageLayout final_layout = VK_IMAGE_LAYOUT_GENERAL);
    void clear_depth_stencil_image(VkCommandBuffer cmd, VkImage image);
    void rebuild_texture_sampler(vk_texture_object& tex);
    void upload_texture_level(
        vk_texture_object& tex,
        uint32_t level,
        int32_t xoffset,
        int32_t yoffset,
        uint32_t width,
        uint32_t height,
        uint32_t format,
        uint32_t type,
        const void* pixels);

    std::optional<VkCompareOp> map_gl_compare(uint32_t gl_func);
    std::optional<VkStencilOp> map_gl_stencil_op(uint32_t op);
    std::optional<VkBlendFactor> map_gl_blend(uint32_t f);
    VkShaderModule get_or_create_frag_module(const frag_shader_desc& desc);
    VkPipeline get_or_create_pipeline(const pipeline_state_key& key);
    bool create_render_passes();
    void destroy_fbo0();
    bool create_fbo0(uint32_t w, uint32_t h);
    void destroy_swapchain();
    bool create_or_recreate_swapchain();
    bool ensure_vulkan_device_created(HDC hdc);
    void end_active_render_pass();
    void sync_engine_and_window_dimensions();
    void begin_frame_if_needed();
    void ensure_render_pass_active();
    bool try_begin_render_pass_with_clear(uint32_t mask);
    void flush_pending_commands_and_wait();
    VkDescriptorSet get_or_alloc_descriptor_set(
        VkImageView v0,
        VkSampler s0,
        VkImageView v1,
        VkSampler s1);
    void resolve_bound_texture(
        uint32_t unit,
        bool is_glsl_prog,
        VkImageView& out_view,
        VkSampler& out_sampler,
        float& out_inv_w,
        float& out_inv_h);
    void emit_draw_indexed(
        VkPrimitiveTopology vk_topo,
        VkDeviceSize v_off,
        VkDeviceSize i_off,
        uint32_t final_index_count);
    void pack_client_vertices_range(
        gl_vk_vertex* dst,
        uint32_t first_vert,
        uint32_t vert_count);
    void submit_vertices_and_indices(
        uint32_t gl_mode,
        const gl_vk_vertex* verts,
        uint32_t vert_count,
        const uint32_t* indices,
        uint32_t index_count);
    gl_vk_vertex fetch_client_vertex(uint32_t idx);
    void init_pipeline_cache();
    void save_pipeline_cache();

    swapchain_frame_action evaluate_pre_present_action(const swapchain_focus_policy_input& in);
    bool should_recreate_on_suboptimal(
        bool window_minimized,
        bool window_focused,
        bool suboptimal_recreate_attempted);
    bool is_window_minimized_or_zero(HWND hwnd);
    bool is_window_focused(HWND hwnd);
    void poll_hwnd_state_throttled(HWND hwnd);
    void teardown_context_and_surface();
    bool bind_hdc_and_ensure_swapchain(HDC hdc);

    std::vector<mat4>& current_matrix_stack();
    void set_state_flag(uint32_t cap, bool enabled);
    void set_client_state_flag(uint32_t array, bool enabled);
    uint32_t get_bound_tex_for_target(uint32_t unit, uint32_t target);
    void rebuild_offscreen_fbo(vk_framebuffer_object& fbo);

    void __stdcall vk_glAlphaFunc(uint32_t func, float ref);
    void __stdcall vk_glArrayElement(int32_t i);
    void __stdcall vk_glBegin(uint32_t mode);
    void __stdcall vk_glEnd();
    void __stdcall vk_glBindTexture(uint32_t target, uint32_t texture);
    void __stdcall vk_glBlendFunc(uint32_t sfactor, uint32_t dfactor);
    void __stdcall vk_glClear(uint32_t mask);
    void __stdcall vk_glClearColor(float r, float g, float b, float a);
    void __stdcall vk_glClearDepth(double depth);
    void __stdcall vk_glClearStencil(int32_t s);
    void __stdcall vk_glClipPlane(uint32_t plane, const double* eq);
    void __stdcall vk_glColor3f(float r, float g, float b);
    void __stdcall vk_glColor4f(float r, float g, float b, float a);
    void __stdcall vk_glColor4fv(const float* v);
    void __stdcall vk_glColor4ubv(const uint8_t* v);
    void __stdcall vk_glColorMask(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void __stdcall vk_glColorPointer(int32_t size, uint32_t type, int32_t stride, const void* ptr);
    void __stdcall vk_glCopyTexSubImage2D(
        uint32_t target,
        int32_t level,
        int32_t xoffset,
        int32_t yoffset,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height);
    void __stdcall vk_glCullFace(uint32_t mode);
    void __stdcall vk_glFrontFace(uint32_t mode);
    void __stdcall vk_glDrawArrays(uint32_t mode, int32_t first, int32_t count);
    void __stdcall vk_glDeleteTextures(int32_t n, const uint32_t* textures);
    void __stdcall vk_glDepthFunc(uint32_t func);
    void __stdcall vk_glDepthMask(uint8_t flag);
    void __stdcall vk_glDepthRange(double z_near, double z_far);
    void __stdcall vk_glDisable(uint32_t cap);
    void __stdcall vk_glDisableClientState(uint32_t array);
    void __stdcall vk_glDrawBuffer(uint32_t mode);
    void __stdcall vk_glDrawElements(
        uint32_t mode,
        int32_t count,
        uint32_t type,
        const void* indices);
    void __stdcall vk_glEnable(uint32_t cap);
    void __stdcall vk_glEnableClientState(uint32_t array);
    void __stdcall vk_glFinish();
    void __stdcall vk_glFlush();
    void __stdcall vk_glFrustum(
        double left,
        double right,
        double bottom,
        double top,
        double z_near,
        double z_far);
    void __stdcall vk_glGenTextures(int32_t n, uint32_t* textures);
    void __stdcall vk_glGetBooleanv(uint32_t pname, uint8_t* params);
    uint32_t __stdcall vk_glGetError();
    void __stdcall vk_glGetIntegerv(uint32_t pname, int32_t* params);
    const uint8_t* __stdcall vk_glGetString(uint32_t name);
    void __stdcall vk_glHint(uint32_t target, uint32_t mode);
    uint8_t __stdcall vk_glIsEnabled(uint32_t cap);
    void __stdcall vk_glLineWidth(float width);
    void __stdcall vk_glLoadIdentity();
    void __stdcall vk_glLoadMatrixf(const float* m);
    void __stdcall vk_glMatrixMode(uint32_t mode);
    void __stdcall vk_glMultMatrixf(const float* m);
    void __stdcall vk_glOrtho(
        double left,
        double right,
        double bottom,
        double top,
        double z_near,
        double z_far);
    void __stdcall vk_glPixelStorei(uint32_t pname, int32_t param);
    void __stdcall vk_glPolygonMode(uint32_t face, uint32_t mode);
    void __stdcall vk_glPolygonOffset(float factor, float units);
    void __stdcall vk_glPushClientAttrib(uint32_t mask);
    void __stdcall vk_glPopClientAttrib();
    void __stdcall vk_glPushMatrix();
    void __stdcall vk_glPopMatrix();
    void __stdcall vk_glReadBuffer(uint32_t mode);
    void __stdcall vk_glReadPixels(
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height,
        uint32_t format,
        uint32_t type,
        void* pixels);
    void __stdcall vk_glRotatef(float angle, float x, float y, float z);
    void __stdcall vk_glScalef(float x, float y, float z);
    void __stdcall vk_glScissor(int32_t x, int32_t y, int32_t w, int32_t h);
    void __stdcall vk_glShadeModel(uint32_t mode);
    void __stdcall vk_glStencilFunc(uint32_t func, int32_t ref, uint32_t mask);
    void __stdcall vk_glStencilMask(uint32_t mask);
    void __stdcall vk_glStencilOp(uint32_t fail, uint32_t zfail, uint32_t zpass);
    void __stdcall vk_glTexCoord2f(float s, float t);
    void __stdcall vk_glTexCoord2fv(const float* v);
    void __stdcall vk_glTexCoordPointer(int32_t size, uint32_t type, int32_t stride, const void* ptr);
    void __stdcall vk_glTexEnvf(uint32_t target, uint32_t pname, float param);
    void __stdcall vk_glTexEnvi(uint32_t target, uint32_t pname, int32_t param);
    void __stdcall vk_glTexImage2D(
        uint32_t target,
        int32_t level,
        int32_t internalformat,
        int32_t width,
        int32_t height,
        int32_t border,
        uint32_t format,
        uint32_t type,
        const void* pixels);
    void __stdcall vk_glTexSubImage2D(
        uint32_t target,
        int32_t level,
        int32_t xoffset,
        int32_t yoffset,
        int32_t width,
        int32_t height,
        uint32_t format,
        uint32_t type,
        const void* pixels);
    void __stdcall vk_glTexParameterf(uint32_t target, uint32_t pname, float param);
    void __stdcall vk_glTexParameteri(uint32_t target, uint32_t pname, int32_t param);
    void __stdcall vk_glTranslatef(float x, float y, float z);
    void __stdcall vk_glVertex2f(float x, float y);
    void __stdcall vk_glVertex3f(float x, float y, float z);
    void __stdcall vk_glVertex3fv(const float* v);
    void __stdcall vk_glVertexPointer(int32_t size, uint32_t type, int32_t stride, const void* ptr);
    void __stdcall vk_glViewport(int32_t x, int32_t y, int32_t w, int32_t h);
    void __stdcall vk_glActiveTextureARB(uint32_t texture);
    void __stdcall vk_glClientActiveTextureARB(uint32_t texture);
    void __stdcall vk_glMultiTexCoord2fARB(uint32_t target, float s, float t);
    void __stdcall vk_glLockArraysEXT(int32_t first, int32_t count);
    void __stdcall vk_glUnlockArraysEXT();
    int __stdcall vk_wglGetDeviceGammaRamp3DFX(HDC hdc, void* ramp);
    int __stdcall vk_wglSetDeviceGammaRamp3DFX(HDC hdc, const void* ramp);
    int __stdcall vk_wglSwapIntervalEXT(int interval);
    void __stdcall vk_glGenQueriesARB(int32_t n, uint32_t* ids);
    void __stdcall vk_glDeleteQueriesARB(int32_t n, const uint32_t* ids);
    uint8_t __stdcall vk_glIsQueryARB(uint32_t id);
    void __stdcall vk_glBeginQueryARB(uint32_t target, uint32_t id);
    void __stdcall vk_glEndQueryARB(uint32_t target);
    void __stdcall vk_glGetQueryivARB(uint32_t target, uint32_t pname, int32_t* params);
    void __stdcall vk_glGetQueryObjectivARB(uint32_t id, uint32_t pname, int32_t* params);
    void __stdcall vk_glGetQueryObjectuivARB(uint32_t id, uint32_t pname, uint32_t* params);
    uint32_t __stdcall vk_glCreateProgramObjectARB();
    uint32_t __stdcall vk_glCreateShaderObjectARB(uint32_t shader_type);
    void __stdcall vk_glDeleteObjectARB(uint32_t obj);
    void __stdcall vk_glShaderSourceARB(
        uint32_t shader_obj,
        int32_t count,
        const char** string,
        const int32_t* length);
    void __stdcall vk_glCompileShaderARB(uint32_t shader_obj);
    void __stdcall vk_glAttachObjectARB(uint32_t container_obj, uint32_t obj);
    void __stdcall vk_glDetachObjectARB(uint32_t container_obj, uint32_t attached_obj);
    void __stdcall vk_glLinkProgramARB(uint32_t program_obj);
    void __stdcall vk_glUseProgramObjectARB(uint32_t program_obj);
    void __stdcall vk_glValidateProgramARB(uint32_t program_obj);
    void __stdcall vk_glGetObjectParameterivARB(uint32_t obj, uint32_t pname, int32_t* params);
    void __stdcall vk_glGetInfoLogARB(
        uint32_t obj,
        int32_t max_length,
        int32_t* length,
        char* info_log);
    int32_t __stdcall vk_glGetUniformLocationARB(uint32_t program_obj, const char* name);
    void __stdcall vk_glUniform1iARB(int32_t location, int32_t v0);
    void __stdcall vk_glUniform1fARB(int32_t location, float v0);
    uint8_t __stdcall vk_glIsRenderbufferEXT(uint32_t renderbuffer);
    void __stdcall vk_glBindRenderbufferEXT(uint32_t target, uint32_t renderbuffer);
    void __stdcall vk_glDeleteRenderbuffersEXT(int32_t n, const uint32_t* renderbuffers);
    void __stdcall vk_glGenRenderbuffersEXT(int32_t n, uint32_t* renderbuffers);
    void __stdcall vk_glRenderbufferStorageEXT(
        uint32_t target,
        uint32_t internalformat,
        int32_t width,
        int32_t height);
    void __stdcall vk_glGetRenderbufferParameterivEXT(
        uint32_t target,
        uint32_t pname,
        int32_t* params);
    uint8_t __stdcall vk_glIsFramebufferEXT(uint32_t framebuffer);
    void __stdcall vk_glBindFramebufferEXT(uint32_t target, uint32_t framebuffer);
    void __stdcall vk_glDeleteFramebuffersEXT(int32_t n, const uint32_t* framebuffers);
    void __stdcall vk_glGenFramebuffersEXT(int32_t n, uint32_t* framebuffers);
    uint32_t __stdcall vk_glCheckFramebufferStatusEXT(uint32_t target);
    void __stdcall vk_glFramebufferTexture1DEXT(
        uint32_t target,
        uint32_t attachment,
        uint32_t textarget,
        uint32_t texture,
        int32_t level);
    void __stdcall vk_glFramebufferTexture2DEXT(
        uint32_t target,
        uint32_t attachment,
        uint32_t textarget,
        uint32_t texture,
        int32_t level);
    void __stdcall vk_glFramebufferTexture3DEXT(
        uint32_t target,
        uint32_t attachment,
        uint32_t textarget,
        uint32_t texture,
        int32_t level,
        int32_t zoffset);
    void __stdcall vk_glFramebufferRenderbufferEXT(
        uint32_t target,
        uint32_t attachment,
        uint32_t renderbuffertarget,
        uint32_t renderbuffer);
    void __stdcall vk_glGetFramebufferAttachmentParameterivEXT(
        uint32_t target,
        uint32_t attachment,
        uint32_t pname,
        int32_t* params);
    void __stdcall vk_glGenerateMipmapEXT(uint32_t target);
    void __stdcall vk_glBlitFramebufferEXT(
        int32_t src_x0,
        int32_t src_y0,
        int32_t src_x1,
        int32_t src_y1,
        int32_t dst_x0,
        int32_t dst_y0,
        int32_t dst_x1,
        int32_t dst_y1,
        uint32_t mask,
        uint32_t filter);
    int __stdcall vk_wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd);
    int __stdcall vk_wglDescribePixelFormat(
        HDC hdc,
        int pixel_format,
        UINT n_bytes,
        LPPIXELFORMATDESCRIPTOR ppfd);
    int __stdcall vk_wglSetPixelFormat(HDC hdc, int pixel_format, const PIXELFORMATDESCRIPTOR* ppfd);
    HGLRC __stdcall vk_wglCreateContext(HDC hdc);
    int __stdcall vk_wglMakeCurrent(HDC hdc, HGLRC hglrc);
    int __stdcall vk_wglDeleteContext(HGLRC hglrc);
    int __stdcall vk_wglShareLists(HGLRC hglrc1, HGLRC hglrc2);
    int __stdcall vk_wglSwapBuffers(HDC hdc);
    PROC __stdcall vk_wglGetProcAddress(LPCSTR lpsz_proc);
}
