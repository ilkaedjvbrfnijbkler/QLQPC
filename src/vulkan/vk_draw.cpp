#include "vulkan/vk_internal.hpp"

#include <emmintrin.h>

namespace qlqpc::vulkan
{
    namespace
    {
        std::wstring get_pipeline_cache_file_path()
        {
            static constexpr std::wstring_view suffix = L"qlqpc_vk_pipeline_cache.bin";
            DWORD cap = GetTempPathW(0u, nullptr);
            if (cap == 0u)
            {
                return {};
            }
            std::wstring dir(static_cast<size_t>(cap), L'\0');
            const DWORD len = GetTempPathW(cap, dir.data());
            if (len == 0u || len >= cap)
            {
                return {};
            }
            dir.resize(len);
            if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/')
            {
                dir.push_back(L'\\');
            }
            dir.append(suffix);
            return dir;
        }
    }

    std::optional<VkCompareOp> map_gl_compare(uint32_t gl_func)
    {
        switch (gl_func)
        {
            case GL_NEVER:
                return VK_COMPARE_OP_NEVER;
            case GL_LESS:
                return VK_COMPARE_OP_LESS;
            case GL_EQUAL:
                return VK_COMPARE_OP_EQUAL;
            case GL_LEQUAL:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case GL_GREATER:
                return VK_COMPARE_OP_GREATER;
            case GL_NOTEQUAL:
                return VK_COMPARE_OP_NOT_EQUAL;
            case GL_GEQUAL:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case GL_ALWAYS:
                return VK_COMPARE_OP_ALWAYS;
            default:
                return std::nullopt;
        }
    }

    std::optional<VkStencilOp> map_gl_stencil_op(uint32_t op)
    {
        switch (op)
        {
            case GL_ZERO:
                return VK_STENCIL_OP_ZERO;
            case GL_REPLACE:
                return VK_STENCIL_OP_REPLACE;
            case GL_INCR:
                return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            case GL_DECR:
                return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            case GL_INVERT:
                return VK_STENCIL_OP_INVERT;
            case GL_INCR_WRAP:
                return VK_STENCIL_OP_INCREMENT_AND_WRAP;
            case GL_DECR_WRAP:
                return VK_STENCIL_OP_DECREMENT_AND_WRAP;
            case GL_KEEP:
                return VK_STENCIL_OP_KEEP;
            default:
                return std::nullopt;
        }
    }

    std::optional<VkBlendFactor> map_gl_blend(uint32_t f)
    {
        switch (f)
        {
            case GL_ZERO:
                return VK_BLEND_FACTOR_ZERO;
            case GL_ONE:
                return VK_BLEND_FACTOR_ONE;
            case GL_SRC_COLOR:
                return VK_BLEND_FACTOR_SRC_COLOR;
            case GL_ONE_MINUS_SRC_COLOR:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case GL_DST_COLOR:
                return VK_BLEND_FACTOR_DST_COLOR;
            case GL_ONE_MINUS_DST_COLOR:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case GL_SRC_ALPHA:
                return VK_BLEND_FACTOR_SRC_ALPHA;
            case GL_ONE_MINUS_SRC_ALPHA:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case GL_DST_ALPHA:
                return VK_BLEND_FACTOR_DST_ALPHA;
            case GL_ONE_MINUS_DST_ALPHA:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case 0x0308u:
                return static_cast<VkBlendFactor>(14);
            default:
                return std::nullopt;
        }
    }

    VkShaderModule get_or_create_frag_module(const frag_shader_desc& desc)
    {
        if (auto it = ctx.frag_shader_mods.find(desc); it != ctx.frag_shader_mods.end())
        {
            return it->second;
        }
        const auto spv = build_fragment_shader_spirv(desc);
        VkShaderModuleCreateInfo smci{};
        smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smci.codeSize = spv.size() * sizeof(uint32_t);
        smci.pCode = spv.data();
        VkShaderModule mod = vk_null_handle;
        if (ctx.vk.vkCreateShaderModule(ctx.device, &smci, nullptr, &mod) != VK_SUCCESS)
        {
            return vk_null_handle;
        }
        ctx.frag_shader_mods.emplace(desc, mod);
        return mod;
    }

    VkPipeline get_or_create_pipeline(const pipeline_state_key& key)
    {
        if (auto it = ctx.pipelines.find(key); it != ctx.pipelines.end())
        {
            return it->second;
        }

        frag_shader_desc fdesc{};
        fdesc.post = key.post;
        fdesc.combiner = key.combiner;
        fdesc.alpha_test = key.alpha_test;
        fdesc.clip_plane_enabled = (key.clip_plane != 0);

        const VkShaderModule frag_mod = get_or_create_frag_module(fdesc);
        if (frag_mod == vk_null_handle || ctx.vert_shader_mod == vk_null_handle)
        {
            return vk_null_handle;
        }

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = 0x01u;
        stages[0].module = ctx.vert_shader_mod;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = 0x10u;
        stages[1].module = frag_mod;
        stages[1].pName = "main";

        VkVertexInputBindingDescription bind_desc{};
        bind_desc.binding = 0u;
        bind_desc.stride = sizeof(gl_vk_vertex);
        bind_desc.inputRate = 0;

        VkVertexInputAttributeDescription attrs[4]{};
        attrs[0] = {0u, 0u, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gl_vk_vertex, pos)};
        attrs[1] = {1u, 0u, VK_FORMAT_R8G8B8A8_UNORM, offsetof(gl_vk_vertex, color)};
        attrs[2] = {2u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof(gl_vk_vertex, uv0)};
        attrs[3] = {3u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof(gl_vk_vertex, uv1)};

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 1u;
        vi.pVertexBindingDescriptions = &bind_desc;
        vi.vertexAttributeDescriptionCount = 4u;
        vi.pVertexAttributeDescriptions = attrs;

        VkPipelineInputAssemblyStateCreateInfo ia{};
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = static_cast<VkPrimitiveTopology>(key.topology);

        VkPipelineViewportStateCreateInfo vp{};
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.viewportCount = 1u;
        vp.scissorCount = 1u;

        VkPipelineRasterizationStateCreateInfo rs{};
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.polygonMode = static_cast<VkPolygonMode>(key.polygon_mode);
        rs.cullMode = static_cast<VkFlags>(key.cull_mode);
        rs.frontFace = static_cast<VkFrontFace>(key.front_face);
        rs.depthBiasEnable = (key.depth_bias != 0) ? VK_TRUE : VK_FALSE;
        rs.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = 1;

        VkStencilOpState st_op{};
        st_op.failOp = static_cast<VkStencilOp>(key.stencil_fail);
        st_op.passOp = static_cast<VkStencilOp>(key.stencil_zpass);
        st_op.depthFailOp = static_cast<VkStencilOp>(key.stencil_zfail);
        st_op.compareOp = static_cast<VkCompareOp>(key.stencil_func);
        st_op.compareMask = 0xFFu;
        st_op.writeMask = 0xFFu;
        st_op.reference = 0u;

        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable = (key.depth_test != 0 && key.is_present_pass == 0) ? VK_TRUE : VK_FALSE;
        ds.depthWriteEnable = (key.depth_write != 0 && key.is_present_pass == 0) ? VK_TRUE : VK_FALSE;
        ds.depthCompareOp = static_cast<VkCompareOp>(key.depth_func);
        ds.stencilTestEnable = (key.stencil_test != 0 && key.is_present_pass == 0) ? VK_TRUE : VK_FALSE;
        ds.front = st_op;
        ds.back = st_op;

        VkPipelineColorBlendAttachmentState cb_att{};
        cb_att.blendEnable = (key.blend_enable != 0) ? VK_TRUE : VK_FALSE;
        cb_att.srcColorBlendFactor = static_cast<VkBlendFactor>(key.src_blend);
        cb_att.dstColorBlendFactor = static_cast<VkBlendFactor>(key.dst_blend);
        cb_att.colorBlendOp = VK_BLEND_OP_ADD;
        cb_att.srcAlphaBlendFactor = static_cast<VkBlendFactor>(key.src_blend);
        cb_att.dstAlphaBlendFactor = static_cast<VkBlendFactor>(key.dst_blend);
        cb_att.alphaBlendOp = VK_BLEND_OP_ADD;
        cb_att.colorWriteMask = static_cast<VkFlags>(key.color_mask);

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1u;
        cb.pAttachments = &cb_att;

        const VkDynamicState dyn_states[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_DEPTH_BIAS,
            VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
            VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
            VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        };
        VkPipelineDynamicStateCreateInfo dyn{};
        dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dyn.dynamicStateCount = 6u;
        dyn.pDynamicStates = dyn_states;

        VkGraphicsPipelineCreateInfo gpci{};
        gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gpci.stageCount = 2u;
        gpci.pStages = stages;
        gpci.pVertexInputState = &vi;
        gpci.pInputAssemblyState = &ia;
        gpci.pViewportState = &vp;
        gpci.pRasterizationState = &rs;
        gpci.pMultisampleState = &ms;
        gpci.pDepthStencilState = &ds;
        gpci.pColorBlendState = &cb;
        gpci.pDynamicState = &dyn;
        gpci.layout = ctx.pipeline_layout;
        gpci.renderPass = (key.is_present_pass != 0) ? ctx.present_render_pass : ctx.scene_render_pass;
        gpci.subpass = 0u;

        VkPipeline pipe = vk_null_handle;
        if (ctx.vk.vkCreateGraphicsPipelines(
                ctx.device, ctx.pipeline_cache, 1u, &gpci, nullptr, &pipe) != VK_SUCCESS)
        {
            return vk_null_handle;
        }
        ctx.pipelines.emplace(key, pipe);
        ctx.pipeline_cache_dirty = true;
        if (ctx.pipelines.size() == 32u || ctx.pipelines.size() == 64u)
        {
            save_pipeline_cache();
        }
        return pipe;
    }

    void init_pipeline_cache()
    {
        if (ctx.device == nullptr || ctx.vk.vkCreatePipelineCache == nullptr ||
            ctx.pipeline_cache != vk_null_handle)
        {
            return;
        }
        std::vector<uint8_t> initial_data{};
        const std::wstring path = get_pipeline_cache_file_path();
        if (!path.empty())
        {
            const HANDLE hf = CreateFileW(
                path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hf != INVALID_HANDLE_VALUE)
            {
                LARGE_INTEGER sz{};
                if (GetFileSizeEx(hf, &sz) && sz.QuadPart > 32 && sz.QuadPart <= (16ll * 1024ll * 1024ll))
                {
                    initial_data.resize(static_cast<size_t>(sz.QuadPart));
                    DWORD bytes_read = 0;
                    if (!ReadFile(hf, initial_data.data(), static_cast<DWORD>(initial_data.size()), &bytes_read, nullptr) ||
                        bytes_read != initial_data.size())
                    {
                        initial_data.clear();
                    }
                }
                CloseHandle(hf);
            }
        }

        VkPipelineCacheCreateInfo pcci{};
        pcci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pcci.initialDataSize = initial_data.size();
        pcci.pInitialData = initial_data.empty() ? nullptr : initial_data.data();
        if (ctx.vk.vkCreatePipelineCache(ctx.device, &pcci, nullptr, &ctx.pipeline_cache) != VK_SUCCESS &&
            !initial_data.empty())
        {
            DeleteFileW(path.c_str());
            pcci.initialDataSize = 0;
            pcci.pInitialData = nullptr;
            ctx.vk.vkCreatePipelineCache(ctx.device, &pcci, nullptr, &ctx.pipeline_cache);
        }
    }

    void save_pipeline_cache()
    {
        if (!ctx.pipeline_cache_dirty || ctx.device == nullptr ||
            ctx.pipeline_cache == vk_null_handle || ctx.vk.vkGetPipelineCacheData == nullptr)
        {
            return;
        }
        size_t data_size = 0;
        if (ctx.vk.vkGetPipelineCacheData(ctx.device, ctx.pipeline_cache, &data_size, nullptr) != VK_SUCCESS ||
            data_size == 0 || data_size > (16ull * 1024ull * 1024ull))
        {
            return;
        }
        std::vector<uint8_t> data(data_size);
        if (ctx.vk.vkGetPipelineCacheData(ctx.device, ctx.pipeline_cache, &data_size, data.data()) != VK_SUCCESS)
        {
            return;
        }
        const std::wstring path = get_pipeline_cache_file_path();
        if (path.empty())
        {
            return;
        }
        const HANDLE hf = CreateFileW(
            path.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hf != INVALID_HANDLE_VALUE)
        {
            DWORD written = 0;
            WriteFile(hf, data.data(), static_cast<DWORD>(data_size), &written, nullptr);
            CloseHandle(hf);
            ctx.pipeline_cache_dirty = false;
        }
    }

    VkDescriptorSet get_or_alloc_descriptor_set(
        VkImageView v0,
        VkSampler s0,
        VkImageView v1,
        VkSampler s1)
    {
        const size_t fi = ctx.cur_frame;
        auto& cache = ctx.desc_caches[fi];
        const vulkan_context::bound_pair_key key{v0, s0, v1, s1};
        if (auto it = cache.find(key); it != cache.end())
        {
            return it->second;
        }
        VkDescriptorSetAllocateInfo dsai{};
        dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsai.descriptorPool = ctx.desc_pools[fi];
        dsai.descriptorSetCount = 1u;
        dsai.pSetLayouts = &ctx.desc_set_layout;
        VkDescriptorSet ds = vk_null_handle;
        if (ctx.vk.vkAllocateDescriptorSets(ctx.device, &dsai, &ds) != VK_SUCCESS)
        {
            return vk_null_handle;
        }

        VkDescriptorImageInfo ii0{s0, v0, VK_IMAGE_LAYOUT_GENERAL};
        VkDescriptorImageInfo ii1{s1, v1, VK_IMAGE_LAYOUT_GENERAL};
        VkWriteDescriptorSet writes[2]{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = ds;
        writes[0].dstBinding = 0u;
        writes[0].descriptorCount = 1u;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &ii0;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = ds;
        writes[1].dstBinding = 1u;
        writes[1].descriptorCount = 1u;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &ii1;

        ctx.vk.vkUpdateDescriptorSets(ctx.device, 2u, writes, 0u, nullptr);
        cache.emplace(key, ds);
        return ds;
    }

    void resolve_bound_texture(
        uint32_t unit,
        bool is_glsl_prog,
        VkImageView& out_view,
        VkSampler& out_sampler,
        float& out_inv_w,
        float& out_inv_h)
    {
        out_view = ctx.white_tex.view;
        out_sampler = ctx.white_tex.sampler;
        out_inv_w = 1.0f;
        out_inv_h = 1.0f;

        uint32_t tid = 0;
        if (is_glsl_prog)
        {
            tid = (ctx.bound_tex_rect[unit] != 0)
                ? ctx.bound_tex_rect[unit]
                : ctx.bound_tex_2d[unit];
        }
        else if (ctx.tex_rect_enabled[unit] && ctx.bound_tex_rect[unit] != 0)
        {
            tid = ctx.bound_tex_rect[unit];
        }
        else if (ctx.tex_2d_enabled[unit] && ctx.bound_tex_2d[unit] != 0)
        {
            tid = ctx.bound_tex_2d[unit];
        }
        if (tid == 0)
        {
            return;
        }
        vk_texture_object* const tex = find_texture(tid);
        if (tex == nullptr || tex->view == vk_null_handle)
        {
            return;
        }
        if (tex->sampler == vk_null_handle)
        {
            rebuild_texture_sampler(*tex);
        }
        out_view = tex->view;
        out_sampler = tex->sampler;
        if (tex->is_rectangle)
        {
            out_inv_w = 1.0f / static_cast<float>(std::max(1u, tex->width));
            out_inv_h = -1.0f / static_cast<float>(std::max(1u, tex->height));
        }
    }

    void emit_draw_indexed(
        VkPrimitiveTopology vk_topo,
        VkDeviceSize v_off,
        VkDeviceSize i_off,
        uint32_t final_index_count)
    {
        const size_t fi = ctx.cur_frame;
        const glsl_shader_object* active_prog = nullptr;
        if (ctx.active_program != 0)
        {
            if (auto it = ctx.glsl_objects.find(ctx.active_program); it != ctx.glsl_objects.end())
            {
                active_prog = &it->second;
            }
        }

        VkPipeline pipe = ctx.cached_pipeline;
        if (ctx.pso_key_dirty ||
            ctx.cached_pso_topo != static_cast<uint8_t>(vk_topo) ||
            pipe == vk_null_handle)
        {
            pipeline_state_key pkey{};
            pkey.topology = static_cast<uint8_t>(vk_topo);
            if (ctx.polygon_mode == GL_FILL)
            {
                pkey.polygon_mode = static_cast<uint8_t>(VK_POLYGON_MODE_FILL);
            }
            else if (ctx.polygon_mode == GL_LINE)
            {
                pkey.polygon_mode = static_cast<uint8_t>(VK_POLYGON_MODE_LINE);
            }
            else
            {
                return;
            }

            if (!ctx.cull_enabled)
            {
                pkey.cull_mode = VK_CULL_MODE_NONE;
            }
            else if (ctx.cull_face_mode == GL_FRONT_AND_BACK)
            {
                pkey.cull_mode = VK_CULL_MODE_FRONT_AND_BACK;
            }
            else if (ctx.cull_face_mode == GL_FRONT)
            {
                pkey.cull_mode = VK_CULL_MODE_FRONT_BIT;
            }
            else
            {
                pkey.cull_mode = VK_CULL_MODE_BACK_BIT;
            }
            pkey.front_face = (ctx.front_face_mode == GL_CW)
                ? static_cast<uint8_t>(VK_FRONT_FACE_CLOCKWISE)
                : static_cast<uint8_t>(VK_FRONT_FACE_COUNTER_CLOCKWISE);

            const auto depth_cmp = map_gl_compare(ctx.depth_func);
            const auto src_blend = map_gl_blend(ctx.blend_src);
            const auto dst_blend = map_gl_blend(ctx.blend_dst);
            const auto stencil_cmp = map_gl_compare(ctx.stencil_func);
            const auto stencil_fail = map_gl_stencil_op(ctx.stencil_fail_op);
            const auto stencil_zfail = map_gl_stencil_op(ctx.stencil_zfail_op);
            const auto stencil_zpass = map_gl_stencil_op(ctx.stencil_zpass_op);
            if (!depth_cmp || !src_blend || !dst_blend ||
                !stencil_cmp || !stencil_fail || !stencil_zfail || !stencil_zpass)
            {
                return;
            }

            pkey.depth_test = ctx.depth_test_enabled ? 1u : 0u;
            pkey.depth_write = ctx.depth_write_enabled ? 1u : 0u;
            pkey.depth_func = static_cast<uint8_t>(*depth_cmp);
            pkey.depth_bias = ctx.poly_offset_enabled ? 1u : 0u;

            pkey.blend_enable = ctx.blend_enabled ? 1u : 0u;
            pkey.src_blend = static_cast<uint8_t>(*src_blend);
            pkey.dst_blend = static_cast<uint8_t>(*dst_blend);
            pkey.color_mask = ctx.color_write_mask;

            pkey.stencil_test = ctx.stencil_test_enabled ? 1u : 0u;
            pkey.stencil_func = static_cast<uint8_t>(*stencil_cmp);
            pkey.stencil_fail = static_cast<uint8_t>(*stencil_fail);
            pkey.stencil_zfail = static_cast<uint8_t>(*stencil_zfail);
            pkey.stencil_zpass = static_cast<uint8_t>(*stencil_zpass);

            pkey.post = (active_prog != nullptr) ? active_prog->kind : post_process_kind::none;

            const bool tmu0_enabled =
                (ctx.tex_rect_enabled[0] && ctx.bound_tex_rect[0] != 0) ||
                (ctx.tex_2d_enabled[0] && ctx.bound_tex_2d[0] != 0);
            const bool tmu1_enabled =
                (ctx.tex_rect_enabled[1] && ctx.bound_tex_rect[1] != 0) ||
                (ctx.tex_2d_enabled[1] && ctx.bound_tex_2d[1] != 0);

            if (!tmu0_enabled)
            {
                pkey.combiner = frag_combiner::untextured;
            }
            else if (!tmu1_enabled)
            {
                pkey.combiner = (ctx.tex_env_mode[0] == GL_REPLACE)
                    ? frag_combiner::single_replace
                    : frag_combiner::single_modulate;
            }
            else
            {
                if (ctx.tex_env_mode[1] == GL_ADD)
                {
                    pkey.combiner = frag_combiner::multi_add;
                }
                else if (ctx.tex_env_mode[1] == GL_REPLACE)
                {
                    pkey.combiner = frag_combiner::multi_replace;
                }
                else if (ctx.tex_env_mode[1] == GL_DECAL)
                {
                    pkey.combiner = frag_combiner::multi_decal;
                }
                else
                {
                    pkey.combiner = frag_combiner::multi_modulate;
                }
            }

            if (!ctx.alpha_test_enabled)
            {
                pkey.alpha_test = alpha_test_mode::none;
            }
            else if (ctx.alpha_test_func == GL_GREATER)
            {
                pkey.alpha_test = alpha_test_mode::gt0;
            }
            else if (ctx.alpha_test_func == GL_LESS)
            {
                pkey.alpha_test = alpha_test_mode::lt05;
            }
            else if (ctx.alpha_test_func == GL_GEQUAL)
            {
                pkey.alpha_test = alpha_test_mode::ge05;
            }
            else
            {
                return;
            }
            pkey.clip_plane = ctx.clip_plane_enabled ? 1u : 0u;
            pkey.is_present_pass = 0u;

            pipe = get_or_create_pipeline(pkey);
            ctx.cached_pso_key = pkey;
            ctx.cached_pso_topo = pkey.topology;
            ctx.cached_pipeline = pipe;
            ctx.pso_key_dirty = false;
        }

        if (pipe == vk_null_handle)
        {
            return;
        }
        if (ctx.bound_pipeline != pipe)
        {
            ctx.vk.vkCmdBindPipeline(ctx.active_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
            ctx.bound_pipeline = pipe;
        }

        if (ctx.cmd_vp_dirty)
        {
            const float vp_x = static_cast<float>(ctx.viewport[0]);
            const float vp_w = static_cast<float>(std::max(1, ctx.viewport[2]));
            const float vp_h = static_cast<float>(std::max(1, ctx.viewport[3]));
            const float vp_y = static_cast<float>(static_cast<int>(ctx.cur_fb_h) - (ctx.viewport[1] + ctx.viewport[3]));
            VkViewport vk_vp{vp_x, vp_y, vp_w, vp_h, ctx.depth_range[0], ctx.depth_range[1]};
            ctx.vk.vkCmdSetViewport(ctx.active_cmd, 0u, 1u, &vk_vp);
            ctx.cmd_vp_dirty = false;
        }

        if (ctx.cmd_sc_dirty)
        {
            const VkRect2D vk_sc = compute_vk_scissor_rect();
            ctx.vk.vkCmdSetScissor(ctx.active_cmd, 0u, 1u, &vk_sc);
            ctx.cmd_sc_dirty = false;
        }

        if (ctx.cmd_db_dirty)
        {
            ctx.vk.vkCmdSetDepthBias(
                ctx.active_cmd,
                ctx.poly_offset_enabled ? ctx.poly_offset_units : 0.0f,
                0.0f,
                ctx.poly_offset_enabled ? ctx.poly_offset_factor : 0.0f);
            ctx.cmd_db_dirty = false;
        }

        if (ctx.stencil_test_enabled && ctx.cmd_stencil_dirty)
        {
            ctx.vk.vkCmdSetStencilCompareMask(ctx.active_cmd, 3u, ctx.stencil_val_mask);
            ctx.vk.vkCmdSetStencilWriteMask(ctx.active_cmd, 3u, ctx.stencil_write_mask);
            ctx.vk.vkCmdSetStencilReference(ctx.active_cmd, 3u, static_cast<uint32_t>(ctx.stencil_ref));
            ctx.cmd_stencil_dirty = false;
        }

        const post_process_kind post_kind = ctx.cached_pso_key.post;
        if (ctx.tex_bindings_dirty || ctx.cached_desc_set == vk_null_handle)
        {
            const bool is_glsl_prog = (active_prog != nullptr && post_kind != post_process_kind::none);
            VkImageView v0{};
            VkImageView v1{};
            VkSampler s0{};
            VkSampler s1{};
            float inv_w0 = 1.0f;
            float inv_h0 = 1.0f;
            float inv_w1 = 1.0f;
            float inv_h1 = 1.0f;
            resolve_bound_texture(0u, is_glsl_prog, v0, s0, inv_w0, inv_h0);
            if (is_glsl_prog && post_kind != post_process_kind::combine)
            {
                v1 = ctx.white_tex.view;
                s1 = ctx.white_tex.sampler;
            }
            else
            {
                resolve_bound_texture(1u, is_glsl_prog, v1, s1, inv_w1, inv_h1);
            }
            ctx.cached_desc_set = get_or_alloc_descriptor_set(v0, s0, v1, s1);
            ctx.cached_inv_w0 = inv_w0;
            ctx.cached_inv_h0 = inv_h0;
            ctx.cached_inv_w1 = inv_w1;
            ctx.cached_inv_h1 = inv_h1;
            ctx.tex_bindings_dirty = false;
        }

        const VkDescriptorSet ds = ctx.cached_desc_set;
        if (ds == vk_null_handle)
        {
            return;
        }
        if (ds != ctx.bound_desc_set)
        {
            ctx.vk.vkCmdBindDescriptorSets(
                ctx.active_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                ctx.pipeline_layout, 0u, 1u, &ds, 0u, nullptr);
            ctx.bound_desc_set = ds;
        }

        if (ctx.mvp_dirty)
        {
            const mat4& mv = ctx.mv_stack.back();
            const mat4& proj = ctx.proj_stack.back();
            mat4 mvp = mat4::multiply(proj, mv);
            for (int c = 0; c < 4; ++c)
            {
                mvp.m[c * 4 + 1] = -mvp.m[c * 4 + 1];
                mvp.m[c * 4 + 2] = 0.5f * (mvp.m[c * 4 + 2] + mvp.m[c * 4 + 3]);
            }
            ctx.cached_mvp = mvp;
            ctx.mvp_dirty = false;
        }

        shader_push_constants pc{};
        std::memcpy(pc.mvp, ctx.cached_mvp.m, sizeof(pc.mvp));

        if (ctx.clip_plane_enabled)
        {
            const mat4& mv = ctx.mv_stack.back();
            for (int c = 0; c < 4; ++c)
            {
                pc.clip_plane[c] =
                    mv.m[c * 4 + 0] * ctx.clip_plane_eye[0] +
                    mv.m[c * 4 + 1] * ctx.clip_plane_eye[1] +
                    mv.m[c * 4 + 2] * ctx.clip_plane_eye[2] +
                    mv.m[c * 4 + 3] * ctx.clip_plane_eye[3];
            }
        }

        pc.uv_scale[0] = ctx.cached_inv_w0;
        pc.uv_scale[1] = ctx.cached_inv_h0;
        if (post_kind == post_process_kind::downsample)
        {
            pc.uv_scale[2] = ctx.cached_inv_w0;
            pc.uv_scale[3] = ctx.cached_inv_h0;
        }
        else
        {
            pc.uv_scale[2] = ctx.cached_inv_w1;
            pc.uv_scale[3] = ctx.cached_inv_h1;
        }

        if (active_prog != nullptr)
        {
            if (active_prog->kind == post_process_kind::color_correct)
            {
                pc.post_params[0] = active_prog->gamma_recip;
                pc.post_params[1] = active_prog->overbright;
                pc.post_params[2] = active_prog->contrast;
            }
            else if (active_prog->kind == post_process_kind::bright_pass)
            {
                pc.post_params[0] = active_prog->bright_threshold;
            }
            else if (active_prog->kind == post_process_kind::blur)
            {
                pc.post_params[0] = active_prog->is_blur_vertical ? 0.0f : std::abs(ctx.cached_inv_w0);
                pc.post_params[1] = active_prog->is_blur_vertical ? std::abs(ctx.cached_inv_h0) : 0.0f;
            }
            else if (active_prog->kind == post_process_kind::combine)
            {
                pc.post_params[0] = active_prog->bloom_saturation;
                pc.post_params[1] = active_prog->scene_saturation;
                pc.post_params[2] = active_prog->bloom_intensity;
                pc.post_params[3] = active_prog->scene_intensity;
            }
        }

        if (!ctx.cmd_pc_valid ||
            std::memcmp(&pc.clip_plane, &ctx.last_pc.clip_plane, sizeof(shader_push_constants) - sizeof(pc.mvp)) != 0)
        {
            ctx.vk.vkCmdPushConstants(
                ctx.active_cmd, ctx.pipeline_layout,
                0x01u | 0x10u, 0u, sizeof(shader_push_constants), &pc);
            ctx.last_pc = pc;
            ctx.cmd_pc_valid = true;
        }
        else if (std::memcmp(pc.mvp, ctx.last_pc.mvp, sizeof(pc.mvp)) != 0)
        {
            ctx.vk.vkCmdPushConstants(
                ctx.active_cmd, ctx.pipeline_layout,
                0x01u | 0x10u, 0u, sizeof(pc.mvp), &pc.mvp);
            std::memcpy(ctx.last_pc.mvp, pc.mvp, sizeof(pc.mvp));
        }

        if (!ctx.vbo_ibo_bound)
        {
            const VkDeviceSize zero_off = 0;
            ctx.vk.vkCmdBindVertexBuffers(ctx.active_cmd, 0u, 1u, &ctx.vbo_bufs[fi], &zero_off);
            ctx.vk.vkCmdBindIndexBuffer(ctx.active_cmd, ctx.ibo_bufs[fi], 0u, VK_INDEX_TYPE_UINT32);
            ctx.vbo_ibo_bound = true;
        }

        const auto first_index = static_cast<uint32_t>(i_off / sizeof(uint32_t));
        const auto first_vertex = static_cast<int32_t>(v_off / sizeof(gl_vk_vertex));
        ctx.vk.vkCmdDrawIndexed(ctx.active_cmd, final_index_count, 1u, first_index, first_vertex, 0u);
    }

    void submit_vertices_and_indices(
        uint32_t gl_mode,
        const gl_vk_vertex* verts,
        uint32_t vert_count,
        const uint32_t* indices,
        uint32_t index_count)
    {
        if (vert_count == 0 || index_count == 0 || !ctx.context_created)
        {
            return;
        }

        VkPrimitiveTopology vk_topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        auto& converted_indices = ctx.scratch_topo_indices;
        const uint32_t* final_indices = indices;
        uint32_t final_index_count = index_count;

        auto emit_quad = [&](size_t q, uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3)
        {
            const size_t base = q * 6u;
            converted_indices[base + 0u] = i0;
            converted_indices[base + 1u] = i1;
            converted_indices[base + 2u] = i2;
            converted_indices[base + 3u] = i0;
            converted_indices[base + 4u] = i2;
            converted_indices[base + 5u] = i3;
        };

        if (gl_mode == GL_QUADS)
        {
            const uint32_t quad_count = index_count / 4u;
            const size_t needed = static_cast<size_t>(quad_count) * 6u;
            if (needed > (vulkan_context::ibo_size / sizeof(uint32_t)))
            {
                return;
            }
            if (converted_indices.size() < needed)
            {
                converted_indices.resize(needed);
            }
            for (uint32_t q = 0; q < quad_count; ++q)
            {
                emit_quad(q, indices[q * 4u + 0u], indices[q * 4u + 1u], indices[q * 4u + 2u], indices[q * 4u + 3u]);
            }
            final_indices = converted_indices.data();
            final_index_count = static_cast<uint32_t>(needed);
            vk_topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
        else if (gl_mode == GL_QUAD_STRIP)
        {
            if (index_count < 4u)
            {
                return;
            }
            const uint32_t quad_count = (index_count - 2u) / 2u;
            const size_t needed = static_cast<size_t>(quad_count) * 6u;
            if (needed > (vulkan_context::ibo_size / sizeof(uint32_t)))
            {
                return;
            }
            if (converted_indices.size() < needed)
            {
                converted_indices.resize(needed);
            }
            for (uint32_t q = 0; q < quad_count; ++q)
            {
                const size_t s = static_cast<size_t>(q) * 2u;
                emit_quad(q, indices[s + 0u], indices[s + 1u], indices[s + 3u], indices[s + 2u]);
            }
            final_indices = converted_indices.data();
            final_index_count = static_cast<uint32_t>(needed);
            vk_topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
        else if (gl_mode == GL_POLYGON || gl_mode == GL_TRIANGLE_FAN)
        {
            if (index_count < 3u)
            {
                return;
            }
            const size_t needed = static_cast<size_t>(index_count - 2u) * 3u;
            if (needed > (vulkan_context::ibo_size / sizeof(uint32_t)))
            {
                return;
            }
            if (converted_indices.size() < needed)
            {
                converted_indices.resize(needed);
            }
            for (uint32_t i = 1u; i + 1u < index_count; ++i)
            {
                const size_t base = static_cast<size_t>(i - 1u) * 3u;
                converted_indices[base + 0u] = indices[0];
                converted_indices[base + 1u] = indices[i];
                converted_indices[base + 2u] = indices[i + 1u];
            }
            final_indices = converted_indices.data();
            final_index_count = static_cast<uint32_t>(needed);
            vk_topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
        else if (gl_mode == GL_TRIANGLE_STRIP)
        {
            vk_topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        }
        else if (gl_mode == GL_LINES)
        {
            vk_topo = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        }
        else if (gl_mode == GL_LINE_STRIP)
        {
            vk_topo = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        }
        else if (gl_mode == GL_LINE_LOOP)
        {
            if (index_count < 2u)
            {
                return;
            }
            const size_t needed = static_cast<size_t>(index_count) + 1u;
            if (needed > (vulkan_context::ibo_size / sizeof(uint32_t)))
            {
                return;
            }
            if (converted_indices.size() < needed)
            {
                converted_indices.resize(needed);
            }
            for (uint32_t i = 0; i < index_count; ++i)
            {
                converted_indices[i] = indices[i];
            }
            converted_indices[index_count] = indices[0];
            final_indices = converted_indices.data();
            final_index_count = static_cast<uint32_t>(needed);
            vk_topo = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        }
        else if (gl_mode == GL_POINTS)
        {
            vk_topo = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        }

        if (final_index_count == 0)
        {
            return;
        }

        const VkDeviceSize v_bytes = static_cast<VkDeviceSize>(vert_count) * sizeof(gl_vk_vertex);
        const VkDeviceSize i_bytes = static_cast<VkDeviceSize>(final_index_count) * sizeof(uint32_t);
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
        std::memcpy(ctx.vbo_mapped[fi] + v_off, verts, static_cast<size_t>(v_bytes));
        std::memcpy(ctx.ibo_mapped[fi] + i_off, final_indices, static_cast<size_t>(i_bytes));
        ctx.vbo_offset += v_bytes;
        ctx.ibo_offset += (i_bytes + 15ull) & ~15ull;

        emit_draw_indexed(vk_topo, v_off, i_off, final_index_count);
    }

    gl_vk_vertex fetch_client_vertex(uint32_t idx)
    {
        gl_vk_vertex v{};
        v.color[0] = ctx.cur_color[0];
        v.color[1] = ctx.cur_color[1];
        v.color[2] = ctx.cur_color[2];
        v.color[3] = ctx.cur_color[3];
        v.uv0[0] = ctx.cur_uv[0][0];
        v.uv0[1] = ctx.cur_uv[0][1];
        v.uv1[0] = ctx.cur_uv[1][0];
        v.uv1[1] = ctx.cur_uv[1][1];

        if (ctx.va_vertex_enabled && ctx.va_vertex_ptr != nullptr)
        {
            const size_t stride = (ctx.va_vertex_stride > 0)
                ? static_cast<size_t>(ctx.va_vertex_stride)
                : static_cast<size_t>(ctx.va_vertex_size) * sizeof(float);
            const auto* pf = reinterpret_cast<const float*>(
                static_cast<const uint8_t*>(ctx.va_vertex_ptr) + idx * stride);
            v.pos[0] = pf[0];
            v.pos[1] = (ctx.va_vertex_size >= 2) ? pf[1] : 0.0f;
            v.pos[2] = (ctx.va_vertex_size >= 3) ? pf[2] : 0.0f;
        }

        if (ctx.va_color_enabled && ctx.va_color_ptr != nullptr)
        {
            if (ctx.va_color_type == GL_UNSIGNED_BYTE)
            {
                const size_t stride = (ctx.va_color_stride > 0)
                    ? static_cast<size_t>(ctx.va_color_stride)
                    : static_cast<size_t>(ctx.va_color_size);
                const auto* pc = static_cast<const uint8_t*>(ctx.va_color_ptr) + idx * stride;
                v.color[0] = pc[0];
                v.color[1] = pc[1];
                v.color[2] = pc[2];
                v.color[3] = (ctx.va_color_size >= 4) ? pc[3] : 255u;
            }
            else if (ctx.va_color_type == GL_FLOAT)
            {
                const size_t stride = (ctx.va_color_stride > 0)
                    ? static_cast<size_t>(ctx.va_color_stride)
                    : static_cast<size_t>(ctx.va_color_size) * sizeof(float);
                const auto* pf = reinterpret_cast<const float*>(
                    static_cast<const uint8_t*>(ctx.va_color_ptr) + idx * stride);
                v.color[0] = static_cast<uint8_t>(std::clamp(pf[0], 0.0f, 1.0f) * 255.0f + 0.5f);
                v.color[1] = static_cast<uint8_t>(std::clamp(pf[1], 0.0f, 1.0f) * 255.0f + 0.5f);
                v.color[2] = static_cast<uint8_t>(std::clamp(pf[2], 0.0f, 1.0f) * 255.0f + 0.5f);
                v.color[3] = (ctx.va_color_size >= 4)
                    ? static_cast<uint8_t>(std::clamp(pf[3], 0.0f, 1.0f) * 255.0f + 0.5f)
                    : 255u;
            }
        }

        for (int u = 0; u < 2; ++u)
        {
            if (ctx.va_texcoord_enabled[u] && ctx.va_tc_ptr[u] != nullptr)
            {
                const size_t stride = (ctx.va_tc_stride[u] > 0)
                    ? static_cast<size_t>(ctx.va_tc_stride[u])
                    : static_cast<size_t>(ctx.va_tc_size[u]) * sizeof(float);
                const auto* pt = reinterpret_cast<const float*>(
                    static_cast<const uint8_t*>(ctx.va_tc_ptr[u]) + idx * stride);
                if (u == 0)
                {
                    v.uv0[0] = pt[0];
                    v.uv0[1] = (ctx.va_tc_size[0] >= 2) ? pt[1] : 0.0f;
                }
                else
                {
                    v.uv1[0] = pt[0];
                    v.uv1[1] = (ctx.va_tc_size[1] >= 2) ? pt[1] : 0.0f;
                }
            }
        }
        return v;
    }

    void pack_client_vertices_range(
        gl_vk_vertex* dst,
        uint32_t first_vert,
        uint32_t vert_count)
    {
        const bool fast_pos = ctx.va_vertex_enabled && ctx.va_vertex_ptr != nullptr && ctx.va_vertex_size >= 3;
        const bool fast_col = !ctx.va_color_enabled ||
            (ctx.va_color_ptr != nullptr && ctx.va_color_type == GL_UNSIGNED_BYTE && ctx.va_color_size == 4);
        const bool fast_tc0 = !ctx.va_texcoord_enabled[0] ||
            (ctx.va_tc_ptr[0] != nullptr && ctx.va_tc_size[0] >= 2);
        const bool fast_tc1 = !ctx.va_texcoord_enabled[1] ||
            (ctx.va_tc_ptr[1] != nullptr && ctx.va_tc_size[1] >= 2);

        if (fast_pos && fast_col && fast_tc0 && fast_tc1)
        {
            const size_t pos_stride = (ctx.va_vertex_stride > 0)
                ? static_cast<size_t>(ctx.va_vertex_stride)
                : static_cast<size_t>(ctx.va_vertex_size) * sizeof(float);
            const auto* pos_ptr = static_cast<const uint8_t*>(ctx.va_vertex_ptr) + first_vert * pos_stride;

            size_t col_stride = 0;
            const auto* col_ptr = reinterpret_cast<const uint8_t*>(ctx.cur_color);
            if (ctx.va_color_enabled)
            {
                col_stride = (ctx.va_color_stride > 0) ? static_cast<size_t>(ctx.va_color_stride) : 4u;
                col_ptr = static_cast<const uint8_t*>(ctx.va_color_ptr) + first_vert * col_stride;
            }

            size_t uv0_stride = 0;
            const auto* uv0_ptr = reinterpret_cast<const uint8_t*>(ctx.cur_uv[0]);
            if (ctx.va_texcoord_enabled[0])
            {
                uv0_stride = (ctx.va_tc_stride[0] > 0)
                    ? static_cast<size_t>(ctx.va_tc_stride[0])
                    : static_cast<size_t>(ctx.va_tc_size[0]) * sizeof(float);
                uv0_ptr = static_cast<const uint8_t*>(ctx.va_tc_ptr[0]) + first_vert * uv0_stride;
            }

            size_t uv1_stride = 0;
            const auto* uv1_ptr = reinterpret_cast<const uint8_t*>(ctx.cur_uv[1]);
            if (ctx.va_texcoord_enabled[1])
            {
                uv1_stride = (ctx.va_tc_stride[1] > 0)
                    ? static_cast<size_t>(ctx.va_tc_stride[1])
                    : static_cast<size_t>(ctx.va_tc_size[1]) * sizeof(float);
                uv1_ptr = static_cast<const uint8_t*>(ctx.va_tc_ptr[1]) + first_vert * uv1_stride;
            }

            // stream 32-byte vertices into write-combined vbo memory without reads
            const __m128 const_u1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(uv1_ptr)));
            if (uv1_stride == 0)
            {
                for (uint32_t v = 0; v < vert_count; ++v)
                {
                    const __m128 xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(pos_ptr)));
                    const __m128 z_col = _mm_unpacklo_ps(
                        _mm_load_ss(reinterpret_cast<const float*>(pos_ptr + 8u)),
                        _mm_load_ss(reinterpret_cast<const float*>(col_ptr)));
                    const __m128 lo = _mm_movelh_ps(xy, z_col);

                    const __m128 u0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(uv0_ptr)));
                    const __m128 hi = _mm_movelh_ps(u0, const_u1);

                    _mm_storeu_ps(reinterpret_cast<float*>(&dst[v]) + 0, lo);
                    _mm_storeu_ps(reinterpret_cast<float*>(&dst[v]) + 4, hi);

                    pos_ptr += pos_stride;
                    col_ptr += col_stride;
                    uv0_ptr += uv0_stride;
                }
            }
            else
            {
                for (uint32_t v = 0; v < vert_count; ++v)
                {
                    const __m128 xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(pos_ptr)));
                    const __m128 z_col = _mm_unpacklo_ps(
                        _mm_load_ss(reinterpret_cast<const float*>(pos_ptr + 8u)),
                        _mm_load_ss(reinterpret_cast<const float*>(col_ptr)));
                    const __m128 lo = _mm_movelh_ps(xy, z_col);

                    const __m128 u0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(uv0_ptr)));
                    const __m128 u1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(uv1_ptr)));
                    const __m128 hi = _mm_movelh_ps(u0, u1);

                    _mm_storeu_ps(reinterpret_cast<float*>(&dst[v]) + 0, lo);
                    _mm_storeu_ps(reinterpret_cast<float*>(&dst[v]) + 4, hi);

                    pos_ptr += pos_stride;
                    col_ptr += col_stride;
                    uv0_ptr += uv0_stride;
                    uv1_ptr += uv1_stride;
                }
            }
            return;
        }

        for (uint32_t v = 0; v < vert_count; ++v)
        {
            dst[v] = fetch_client_vertex(first_vert + v);
        }
    }
}
