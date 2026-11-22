#include "vulkan/vk_internal.hpp"

#include <cstdlib>
#include <emmintrin.h>
#include <tmmintrin.h>

namespace qlqpc::vulkan
{
    vulkan_context ctx{};

    uint32_t find_memory_type(uint32_t type_bits, VkFlags req_props)
    {
        for (uint32_t i = 0; i < ctx.mem_props.memoryTypeCount; ++i)
        {
            if ((type_bits & (1u << i)) != 0 &&
                (ctx.mem_props.memoryTypes[i].propertyFlags & req_props) == req_props)
            {
                return i;
            }
        }
        return UINT32_MAX;
    }

    bool create_buffer(
        VkDeviceSize size,
        VkFlags usage,
        VkFlags mem_props,
        VkBuffer& out_buf,
        VkDeviceMemory& out_mem)
    {
        VkBufferCreateInfo bci{};
        bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size = size;
        bci.usage = usage;
        bci.sharingMode = 0;
        if (ctx.vk.vkCreateBuffer(ctx.device, &bci, nullptr, &out_buf) != VK_SUCCESS)
        {
            return false;
        }
        VkMemoryRequirements req{};
        ctx.vk.vkGetBufferMemoryRequirements(ctx.device, out_buf, &req);
        const uint32_t mem_type = find_memory_type(req.memoryTypeBits, mem_props);
        if (mem_type == UINT32_MAX)
        {
            ctx.vk.vkDestroyBuffer(ctx.device, out_buf, nullptr);
            out_buf = vk_null_handle;
            return false;
        }
        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = req.size;
        mai.memoryTypeIndex = mem_type;
        if (ctx.vk.vkAllocateMemory(ctx.device, &mai, nullptr, &out_mem) != VK_SUCCESS)
        {
            ctx.vk.vkDestroyBuffer(ctx.device, out_buf, nullptr);
            out_buf = vk_null_handle;
            return false;
        }
        if (ctx.vk.vkBindBufferMemory(ctx.device, out_buf, out_mem, 0) != VK_SUCCESS)
        {
            ctx.vk.vkFreeMemory(ctx.device, out_mem, nullptr);
            ctx.vk.vkDestroyBuffer(ctx.device, out_buf, nullptr);
            out_buf = vk_null_handle;
            out_mem = vk_null_handle;
            return false;
        }
        return true;
    }

    bool create_buffer(
        VkDeviceSize size,
        VkFlags usage,
        VkFlags mem_props,
        VkBuffer& out_buf,
        VkDeviceMemory& out_mem,
        void** out_mapped)
    {
        if (!create_buffer(size, usage, mem_props, out_buf, out_mem))
        {
            return false;
        }
        if (out_mapped != nullptr)
        {
            if (ctx.vk.vkMapMemory(ctx.device, out_mem, 0, size, 0, out_mapped) != VK_SUCCESS)
            {
                ctx.vk.vkFreeMemory(ctx.device, out_mem, nullptr);
                ctx.vk.vkDestroyBuffer(ctx.device, out_buf, nullptr);
                out_buf = vk_null_handle;
                out_mem = vk_null_handle;
                *out_mapped = nullptr;
                return false;
            }
        }
        return true;
    }

    bool create_image_2d(
        uint32_t width,
        uint32_t height,
        uint32_t mip_levels,
        VkFormat format,
        VkFlags usage,
        VkFlags aspect_mask,
        VkImage& out_img,
        VkDeviceMemory& out_mem,
        VkImageView& out_view)
    {
        VkImageCreateInfo ici{};
        ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ici.imageType = 1;
        ici.format = format;
        ici.extent = {std::max(1u, width), std::max(1u, height), 1u};
        ici.mipLevels = std::max(1u, mip_levels);
        ici.arrayLayers = 1u;
        ici.samples = 1;
        ici.tiling = 0;
        ici.usage = usage;
        ici.sharingMode = 0;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (ctx.vk.vkCreateImage(ctx.device, &ici, nullptr, &out_img) != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements req{};
        ctx.vk.vkGetImageMemoryRequirements(ctx.device, out_img, &req);
        const uint32_t mem_type = find_memory_type(req.memoryTypeBits, 0x01u);
        if (mem_type == UINT32_MAX)
        {
            ctx.vk.vkDestroyImage(ctx.device, out_img, nullptr);
            out_img = vk_null_handle;
            return false;
        }
        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = req.size;
        mai.memoryTypeIndex = mem_type;
        if (ctx.vk.vkAllocateMemory(ctx.device, &mai, nullptr, &out_mem) != VK_SUCCESS)
        {
            ctx.vk.vkDestroyImage(ctx.device, out_img, nullptr);
            out_img = vk_null_handle;
            return false;
        }
        if (ctx.vk.vkBindImageMemory(ctx.device, out_img, out_mem, 0) != VK_SUCCESS)
        {
            ctx.vk.vkFreeMemory(ctx.device, out_mem, nullptr);
            ctx.vk.vkDestroyImage(ctx.device, out_img, nullptr);
            out_img = vk_null_handle;
            out_mem = vk_null_handle;
            return false;
        }

        VkImageViewCreateInfo ivci{};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.image = out_img;
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = format;
        ivci.subresourceRange.aspectMask = aspect_mask;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = std::max(1u, mip_levels);
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;
        if (ctx.vk.vkCreateImageView(ctx.device, &ivci, nullptr, &out_view) != VK_SUCCESS)
        {
            ctx.vk.vkFreeMemory(ctx.device, out_mem, nullptr);
            ctx.vk.vkDestroyImage(ctx.device, out_img, nullptr);
            out_img = vk_null_handle;
            out_mem = vk_null_handle;
            return false;
        }
        return true;
    }

    void cmd_transition_image(
        VkCommandBuffer cmd,
        VkImage image,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        VkFlags aspect_mask,
        uint32_t mip_levels,
        uint32_t base_mip)
    {
        auto layout_stage_access = [](VkImageLayout layout, bool is_src, VkFlags& stage, VkFlags& access)
        {
            switch (layout)
            {
                case VK_IMAGE_LAYOUT_UNDEFINED:
                    stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    access = 0u;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    access = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    access = VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    access = VK_ACCESS_SHADER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                    stage = is_src ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                    access = 0u;
                    break;
                case VK_IMAGE_LAYOUT_GENERAL:
                default:
                    stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_TRANSFER_BIT;
                    access = VK_ACCESS_SHADER_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_TRANSFER_READ_BIT |
                             VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
            }
        };

        VkFlags src_stage = 0u;
        VkFlags src_access = 0u;
        VkFlags dst_stage = 0u;
        VkFlags dst_access = 0u;
        layout_stage_access(old_layout, true, src_stage, src_access);
        layout_stage_access(new_layout, false, dst_stage, dst_access);

        VkImageMemoryBarrier b{};
        b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout = old_layout;
        b.newLayout = new_layout;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = image;
        b.subresourceRange = {aspect_mask, base_mip, mip_levels, 0u, 1u};
        b.srcAccessMask = src_access;
        b.dstAccessMask = dst_access;
        ctx.vk.vkCmdPipelineBarrier(
            cmd, src_stage, dst_stage, 0u, 0u, nullptr, 0u, nullptr, 1u, &b);
    }

    void clear_color_image(
        VkCommandBuffer cmd,
        VkImage image,
        uint32_t mip_levels,
        VkImageLayout final_layout)
    {
        cmd_transition_image(
            cmd, image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1u, mip_levels, 0u);
        VkClearColorValue ccv{};
        ccv.float32[0] = 0.0f;
        ccv.float32[1] = 0.0f;
        ccv.float32[2] = 0.0f;
        ccv.float32[3] = 1.0f;
        VkImageSubresourceRange range{1u, 0u, mip_levels, 0u, 1u};
        ctx.vk.vkCmdClearColorImage(
            cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ccv, 1u, &range);
        cmd_transition_image(
            cmd, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, final_layout,
            1u, mip_levels, 0u);
    }

    void clear_depth_stencil_image(VkCommandBuffer cmd, VkImage image)
    {
        cmd_transition_image(
            cmd, image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0x06u, 1u, 0u);
        VkClearDepthStencilValue cdsv{1.0f, 0u};
        VkImageSubresourceRange range{0x06u, 0u, 1u, 0u, 1u};
        ctx.vk.vkCmdClearDepthStencilImage(
            cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cdsv, 1u, &range);
        // keep depth/stencil in attachment-optimal layout to avoid full-surface decompression
        cmd_transition_image(
            cmd, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            0x06u, 1u, 0u);
    }

    void rebuild_texture_sampler(vk_texture_object& tex)
    {
        ctx.tex_bindings_dirty = true;
        const bool use_mips = !(tex.min_filter == GL_NEAREST || tex.min_filter == GL_LINEAR);
        const uint32_t max_lod_level = use_mips
            ? std::min(tex.max_uploaded_level, std::max(1u, tex.mip_levels) - 1u)
            : 0u;
        const vulkan_context::sampler_key skey
        {
            tex.min_filter,
            tex.mag_filter,
            tex.wrap_s,
            tex.wrap_t,
            max_lod_level
        };
        if (auto it = ctx.sampler_cache.find(skey); it != ctx.sampler_cache.end())
        {
            tex.sampler = it->second;
            return;
        }

        VkSamplerCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sci.magFilter = (tex.mag_filter == GL_NEAREST) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
        if (tex.min_filter == GL_NEAREST ||
            tex.min_filter == GL_NEAREST_MIPMAP_NEAREST ||
            tex.min_filter == GL_NEAREST_MIPMAP_LINEAR)
        {
            sci.minFilter = VK_FILTER_NEAREST;
        }
        else
        {
            sci.minFilter = VK_FILTER_LINEAR;
        }
        if (tex.min_filter == GL_NEAREST_MIPMAP_LINEAR ||
            tex.min_filter == GL_LINEAR_MIPMAP_LINEAR)
        {
            sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
        else
        {
            sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }
        auto map_wrap = [](uint32_t w)
        {
            return (w == GL_CLAMP || w == GL_CLAMP_TO_EDGE)
                ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                : VK_SAMPLER_ADDRESS_MODE_REPEAT;
        };
        sci.addressModeU = map_wrap(tex.wrap_s);
        sci.addressModeV = map_wrap(tex.wrap_t);
        sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.minLod = 0.0f;
        sci.maxLod = use_mips ? static_cast<float>(max_lod_level) : 0.0f;

        VkSampler samp = vk_null_handle;
        if (ctx.vk.vkCreateSampler(ctx.device, &sci, nullptr, &samp) == VK_SUCCESS)
        {
            ctx.sampler_cache.emplace(skey, samp);
            tex.sampler = samp;
        }
    }

    void upload_texture_level(
        vk_texture_object& tex,
        uint32_t level,
        int32_t xoffset,
        int32_t yoffset,
        uint32_t width,
        uint32_t height,
        uint32_t format,
        uint32_t type,
        const void* pixels)
    {
        if (width == 0 || height == 0 || ctx.staging_mapped[0] == nullptr || tex.image == vk_null_handle)
        {
            return;
        }
        if (pixels != nullptr && type != GL_UNSIGNED_BYTE)
        {
            return;
        }
        const uint32_t clamped_level = std::min(level, std::max(1u, tex.mip_levels) - 1u);
        const uint32_t mip_w = std::max(1u, tex.width >> clamped_level);
        const uint32_t mip_h = std::max(1u, tex.height >> clamped_level);
        if (xoffset < 0 || yoffset < 0 ||
            static_cast<uint32_t>(xoffset) + width > mip_w ||
            static_cast<uint32_t>(yoffset) + height > mip_h)
        {
            return;
        }
        const size_t dst_bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
        const VkDeviceSize aligned_bytes = (static_cast<VkDeviceSize>(dst_bytes) + 15ull) & ~15ull;
        if (aligned_bytes > vulkan_context::staging_size)
        {
            return;
        }

        VkDeviceSize buf_off = 0;
        size_t fi = 0;
        if (ctx.context_created)
        {
            if (ctx.staging_offset + aligned_bytes > vulkan_context::staging_size)
            {
                flush_pending_commands_and_wait();
                ctx.staging_offset = 0;
            }
            end_active_render_pass();
            begin_frame_if_needed();
            fi = ctx.cur_frame;
            buf_off = ctx.staging_offset;
            ctx.staging_offset += aligned_bytes;
        }

        uint8_t* const stage_dst = ctx.staging_mapped[fi] + buf_off;
        if (pixels == nullptr)
        {
            std::memset(stage_dst, 0, dst_bytes);
        }
        else if (type == GL_UNSIGNED_BYTE)
        {
            const auto* src = static_cast<const uint8_t*>(pixels);
            const size_t bpp = (format == GL_ALPHA)
                ? 1u
                : ((format == GL_RGB || format == GL_BGR_EXT) ? 3u : 4u);
            const size_t row_elems = (ctx.unpack_row_length > 0)
                ? static_cast<size_t>(ctx.unpack_row_length)
                : static_cast<size_t>(width);
            const size_t raw_stride = row_elems * bpp;
            const size_t align = static_cast<size_t>(std::max(1, ctx.unpack_alignment));
            const size_t row_stride = (raw_stride + align - 1u) & ~(align - 1u);
            const size_t skip_x = static_cast<size_t>(std::max(0, ctx.unpack_skip_pixels));
            const size_t skip_y = static_cast<size_t>(std::max(0, ctx.unpack_skip_rows));

            if (format == GL_RGBA && skip_x == 0 && skip_y == 0 && row_stride == width * 4u)
            {
                std::memcpy(stage_dst, src, dst_bytes);
            }
            else
            {
                const __m128i bgra_mask = _mm_setr_epi8(
                    2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);
                for (uint32_t y = 0; y < height; ++y)
                {
                    const uint8_t* src_row = src + (skip_y + y) * row_stride + skip_x * bpp;
                    uint8_t* dst_row = stage_dst + static_cast<size_t>(y) * width * 4u;
                    uint32_t x = 0;
                    if (format == GL_BGRA_EXT)
                    {
                        for (; x + 4u <= width; x += 4u)
                        {
                            const __m128i px = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_row + x * 4u));
                            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst_row + x * 4u), _mm_shuffle_epi8(px, bgra_mask));
                        }
                        for (; x < width; ++x)
                        {
                            dst_row[x * 4 + 0] = src_row[x * 4 + 2];
                            dst_row[x * 4 + 1] = src_row[x * 4 + 1];
                            dst_row[x * 4 + 2] = src_row[x * 4 + 0];
                            dst_row[x * 4 + 3] = src_row[x * 4 + 3];
                        }
                    }
                    else if (format == GL_ALPHA)
                    {
                        for (; x < width; ++x)
                        {
                            const uint32_t rgba = 0x00FFFFFFu | (static_cast<uint32_t>(src_row[x]) << 24u);
                            std::memcpy(dst_row + x * 4u, &rgba, 4u);
                        }
                    }
                    else if (format == GL_RGB || format == GL_BGR_EXT)
                    {
                        const bool swap_rb = (format == GL_BGR_EXT);
                        for (; x < width; ++x)
                        {
                            dst_row[x * 4 + 0] = src_row[x * 3 + (swap_rb ? 2u : 0u)];
                            dst_row[x * 4 + 1] = src_row[x * 3 + 1u];
                            dst_row[x * 4 + 2] = src_row[x * 3 + (swap_rb ? 0u : 2u)];
                            dst_row[x * 4 + 3] = 255u;
                        }
                    }
                    else
                    {
                        std::memcpy(dst_row, src_row, static_cast<size_t>(width) * 4u);
                    }
                }
            }
        }

        auto record_copy = [&](VkCommandBuffer cmd)
        {
            if (tex.current_layout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                cmd_transition_image(
                    cmd, tex.image, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL, 1u, tex.mip_levels, 0u);
                tex.current_layout = VK_IMAGE_LAYOUT_GENERAL;
            }
            cmd_transition_image(
                cmd, tex.image, VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, 1u, clamped_level);
            VkBufferImageCopy bic{};
            bic.bufferOffset = buf_off;
            bic.imageSubresource = {1u, clamped_level, 0u, 1u};
            bic.imageOffset = {xoffset, yoffset, 0};
            bic.imageExtent = {width, height, 1u};
            ctx.vk.vkCmdCopyBufferToImage(
                cmd, ctx.staging_bufs[fi], tex.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &bic);
            cmd_transition_image(
                cmd, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_GENERAL, 1u, 1u, clamped_level);
            tex.current_layout = VK_IMAGE_LAYOUT_GENERAL;
        };

        if (ctx.context_created && ctx.active_cmd != nullptr)
        {
            record_copy(ctx.active_cmd);
        }
        else
        {
            execute_immediate_cmd(record_copy);
        }

        if (clamped_level > tex.max_uploaded_level)
        {
            tex.max_uploaded_level = clamped_level;
            rebuild_texture_sampler(tex);
        }
    }

    namespace
    {
        DWORD WINAPI present_worker_thread_proc(LPVOID)
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

            LARGE_INTEGER freq{};
            if (QueryPerformanceFrequency(&freq) == FALSE || freq.QuadPart <= 0)
            {
                std::abort();
            }
            const int64_t spin_ticks = std::max<int64_t>(1, freq.QuadPart / 4000);

            for (;;)
            {
                const uint32_t st = ctx.present_worker_state.load(std::memory_order_acquire);
                if (st == 2u)
                {
                    return 0;
                }
                if (st == 1u)
                {
                    const vulkan_context::present_job job = ctx.pending_present_job;

                    VkSubmitInfo si{};
                    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    si.waitSemaphoreCount = 1u;
                    si.pWaitSemaphores = &job.wait_sem;
                    si.pWaitDstStageMask = &job.wait_stage;
                    si.commandBufferCount = 1u;
                    si.pCommandBuffers = &job.cmd;
                    si.signalSemaphoreCount = 1u;
                    si.pSignalSemaphores = &job.signal_sem;
                    ctx.vk.vkResetFences(ctx.device, 1u, &job.fence);
                    const VkResult sub_res = ctx.vk.vkQueueSubmit(ctx.queue, 1u, &si, job.fence);
                    ctx.frame_fence_pending[job.frame_idx] = (sub_res == VK_SUCCESS);

                    VkResult pres_res = sub_res;
                    if (sub_res == VK_SUCCESS && ctx.swapchain != vk_null_handle)
                    {
                        VkPresentInfoKHR pi{};
                        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                        pi.waitSemaphoreCount = 1u;
                        pi.pWaitSemaphores = &job.signal_sem;
                        pi.swapchainCount = 1u;
                        pi.pSwapchains = &ctx.swapchain;
                        pi.pImageIndices = &job.sc_idx;
                        pres_res = ctx.vk.vkQueuePresentKHR(ctx.queue, &pi);
                    }

                    ctx.last_submit_result = sub_res;
                    ctx.last_present_result = pres_res;
                    ctx.last_present_acq_result = job.acq_result;
                    ctx.has_present_result = true;

                    const bool pres_ok = (pres_res == VK_SUCCESS || pres_res == VK_SUBOPTIMAL_KHR);
                    const bool acq_ok = (job.acq_result == VK_SUCCESS || job.acq_result == VK_SUBOPTIMAL_KHR);
                    if (ctx.window_focused_atomic.load(std::memory_order_relaxed) &&
                        sub_res == VK_SUCCESS &&
                        pres_ok &&
                        acq_ok &&
                        ctx.swapchain != vk_null_handle &&
                        job.next_acquire_sem != vk_null_handle)
                    {
                        uint32_t next_sc = 0;
                        const VkResult next_acq = ctx.vk.vkAcquireNextImageKHR(
                            ctx.device, ctx.swapchain, ~0ull,
                            job.next_acquire_sem, vk_null_handle, &next_sc);
                        ctx.preacquired_result = next_acq;
                        ctx.preacquired_sc_idx = next_sc;
                        ctx.preacquired_sem_idx = job.next_sem_idx;
                        ctx.has_preacquired_image = true;
                    }
                    else
                    {
                        ctx.has_preacquired_image = false;
                    }

                    ctx.present_worker_state.store(0u, std::memory_order_seq_cst);
                    if (ctx.main_waiting_on_present_worker.load(std::memory_order_seq_cst) &&
                        ctx.present_worker_done_event != nullptr)
                    {
                        SetEvent(ctx.present_worker_done_event);
                    }
                    continue;
                }

                if (ctx.window_focused_atomic.load(std::memory_order_relaxed))
                {
                    LARGE_INTEGER spin_start{};
                    QueryPerformanceCounter(&spin_start);
                    uint32_t waited_state = 0u;
                    for (;;)
                    {
                        waited_state = ctx.present_worker_state.load(std::memory_order_acquire);
                        if (waited_state != 0u ||
                            !ctx.window_focused_atomic.load(std::memory_order_relaxed))
                        {
                            break;
                        }
                        _mm_pause();
                        LARGE_INTEGER now{};
                        QueryPerformanceCounter(&now);
                        if ((now.QuadPart - spin_start.QuadPart) >= spin_ticks)
                        {
                            break;
                        }
                    }
                    if (waited_state != 0u)
                    {
                        continue;
                    }
                }

                ctx.present_worker_sleeping.store(true, std::memory_order_seq_cst);
                if (ctx.present_worker_state.load(std::memory_order_seq_cst) == 0u &&
                    ctx.present_worker_wake_event != nullptr)
                {
                    WaitForSingleObject(ctx.present_worker_wake_event, INFINITE);
                }
                ctx.present_worker_sleeping.store(false, std::memory_order_seq_cst);
            }
        }

        void rollback_failed_device_creation()
        {
            stop_present_worker();
            ctx.context_created = false;
            ctx.frames_in_flight_busy = false;
            ctx.frame_fence_pending[0] = false;
            ctx.frame_fence_pending[1] = false;
            ctx.has_preacquired_image = false;
            ctx.has_present_result = false;
            ctx.present_render_pass_format = VK_FORMAT_B8G8R8A8_UNORM;
            ctx.swap_interval = 0;
            ctx.gamma_identity = true;
            ctx.pipeline_cache_dirty = false;
            ctx.cached_pipeline = vk_null_handle;
            ctx.cached_pso_topo = 0xFFu;
            ctx.pso_key_dirty = true;
            ctx.swapchain_needs_recreate = false;
            ctx.swapchain_out_of_date = false;
            invalidate_cmd_draw_state();
            ctx.hdc = nullptr;
            ctx.hwnd = nullptr;
            ctx.cmd_recording = false;
            ctx.active_cmd = nullptr;
            ctx.render_pass_active = false;
            ctx.vbo_offset = 0;
            ctx.ibo_offset = 0;
            ctx.staging_offset = 0;
            ctx.cur_frame = 0;
            ctx.next_sem_idx = 0;

            if (ctx.device != nullptr)
            {
                if (ctx.vk.vkDeviceWaitIdle != nullptr)
                {
                    ctx.vk.vkDeviceWaitIdle(ctx.device);
                }
                for (auto& [id, fbo] : ctx.fbos)
                {
                    if (fbo.fb != vk_null_handle && ctx.vk.vkDestroyFramebuffer != nullptr)
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
                ctx.next_tex_id = 1u;

                destroy_swapchain();
                destroy_fbo0();

                for (auto& [k, samp] : ctx.sampler_cache)
                {
                    if (samp != vk_null_handle && ctx.vk.vkDestroySampler != nullptr)
                    {
                        ctx.vk.vkDestroySampler(ctx.device, samp, nullptr);
                    }
                }
                ctx.sampler_cache.clear();
                destroy_vk_image_resources(ctx.white_tex.view, ctx.white_tex.image, ctx.white_tex.memory);
                ctx.white_tex = vk_texture_object{};
                destroy_vk_image_resources(ctx.gamma_lut_tex.view, ctx.gamma_lut_tex.image, ctx.gamma_lut_tex.memory);
                ctx.gamma_lut_tex = vk_texture_object{};

                for (auto& [k, mod] : ctx.frag_shader_mods)
                {
                    if (mod != vk_null_handle && ctx.vk.vkDestroyShaderModule != nullptr)
                    {
                        ctx.vk.vkDestroyShaderModule(ctx.device, mod, nullptr);
                    }
                }
                ctx.frag_shader_mods.clear();

                for (auto& [k, pipe] : ctx.pipelines)
                {
                    if (pipe != vk_null_handle && ctx.vk.vkDestroyPipeline != nullptr)
                    {
                        ctx.vk.vkDestroyPipeline(ctx.device, pipe, nullptr);
                    }
                }
                ctx.pipelines.clear();

                auto destroy_opt = [](auto fn, uint64_t& handle)
                {
                    if (handle != vk_null_handle && fn != nullptr)
                    {
                        fn(ctx.device, handle, nullptr);
                        handle = vk_null_handle;
                    }
                };
                destroy_opt(ctx.vk.vkDestroyShaderModule, ctx.vert_shader_mod);
                destroy_opt(ctx.vk.vkDestroyPipelineCache, ctx.pipeline_cache);
                destroy_opt(ctx.vk.vkDestroyPipelineLayout, ctx.pipeline_layout);
                destroy_opt(ctx.vk.vkDestroyDescriptorSetLayout, ctx.desc_set_layout);
                destroy_opt(ctx.vk.vkDestroyRenderPass, ctx.scene_render_pass);
                destroy_opt(ctx.vk.vkDestroyRenderPass, ctx.scene_render_pass_clear_c);
                destroy_opt(ctx.vk.vkDestroyRenderPass, ctx.scene_render_pass_clear_d);
                destroy_opt(ctx.vk.vkDestroyRenderPass, ctx.scene_render_pass_clear_cd);
                destroy_opt(ctx.vk.vkDestroyRenderPass, ctx.present_render_pass);

                auto destroy_buf = [](VkBuffer& buf, VkDeviceMemory& mem, uint8_t*& mapped)
                {
                    if (buf != vk_null_handle && ctx.vk.vkDestroyBuffer != nullptr)
                    {
                        ctx.vk.vkDestroyBuffer(ctx.device, buf, nullptr);
                        buf = vk_null_handle;
                    }
                    if (mem != vk_null_handle && ctx.vk.vkFreeMemory != nullptr)
                    {
                        ctx.vk.vkFreeMemory(ctx.device, mem, nullptr);
                        mem = vk_null_handle;
                    }
                    mapped = nullptr;
                };
                for (size_t i = 0; i < vulkan_context::frames_in_flight; ++i)
                {
                    ctx.desc_caches[i].clear();
                    destroy_opt(ctx.vk.vkDestroyDescriptorPool, ctx.desc_pools[i]);
                    destroy_buf(ctx.vbo_bufs[i], ctx.vbo_mems[i], ctx.vbo_mapped[i]);
                    destroy_buf(ctx.ibo_bufs[i], ctx.ibo_mems[i], ctx.ibo_mapped[i]);
                    destroy_buf(ctx.staging_bufs[i], ctx.staging_mems[i], ctx.staging_mapped[i]);
                    destroy_opt(ctx.vk.vkDestroyFence, ctx.frame_fences[i]);
                    destroy_opt(ctx.vk.vkDestroyCommandPool, ctx.frame_cmd_pools[i]);
                    ctx.frame_cmds[i] = nullptr;
                }
                for (uint32_t i = 0; i < vulkan_context::sem_ring_size; ++i)
                {
                    destroy_opt(ctx.vk.vkDestroySemaphore, ctx.img_avail_sems[i]);
                    destroy_opt(ctx.vk.vkDestroySemaphore, ctx.render_done_sems[i]);
                }
                destroy_opt(ctx.vk.vkDestroyFence, ctx.upload_fence);
                destroy_opt(ctx.vk.vkDestroyCommandPool, ctx.upload_cmd_pool);
                ctx.upload_cmd = nullptr;
                if (ctx.vk.vkDestroyDevice != nullptr)
                {
                    ctx.vk.vkDestroyDevice(ctx.device, nullptr);
                }
                else if (ctx.vk.vkGetDeviceProcAddr != nullptr)
                {
                    const auto fn_destroy_dev = reinterpret_cast<PFN_vkDestroyDevice>(
                        ctx.vk.vkGetDeviceProcAddr(ctx.device, "vkDestroyDevice"));
                    if (fn_destroy_dev != nullptr)
                    {
                        fn_destroy_dev(ctx.device, nullptr);
                    }
                }
                ctx.device = nullptr;
                ctx.queue = nullptr;
            }

            if (ctx.instance != nullptr)
            {
                if (ctx.surface != vk_null_handle && ctx.vk.vkDestroySurfaceKHR != nullptr)
                {
                    ctx.vk.vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
                    ctx.surface = vk_null_handle;
                }
                if (ctx.vk.vkDestroyInstance != nullptr)
                {
                    ctx.vk.vkDestroyInstance(ctx.instance, nullptr);
                }
                ctx.instance = nullptr;
            }
            ctx.phys_dev = nullptr;
        }
    }

    void wait_for_present_worker_idle()
    {
        for (uint32_t spin = 0; spin < 256u; ++spin)
        {
            if (ctx.present_worker_state.load(std::memory_order_acquire) != 1u)
            {
                return;
            }
            _mm_pause();
        }
        while (ctx.present_worker_state.load(std::memory_order_acquire) == 1u)
        {
            if (ctx.present_worker_done_event == nullptr)
            {
                _mm_pause();
                continue;
            }
            ctx.main_waiting_on_present_worker.store(true, std::memory_order_seq_cst);
            if (ctx.present_worker_state.load(std::memory_order_seq_cst) == 1u)
            {
                WaitForSingleObject(ctx.present_worker_done_event, 10u);
            }
            ctx.main_waiting_on_present_worker.store(false, std::memory_order_seq_cst);
        }
    }

    void start_present_worker_if_needed()
    {
        if (ctx.present_worker_thread != nullptr)
        {
            return;
        }
        if (ctx.present_worker_wake_event == nullptr)
        {
            ctx.present_worker_wake_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        }
        if (ctx.present_worker_done_event == nullptr)
        {
            ctx.present_worker_done_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        }
        ctx.present_worker_state.store(0u, std::memory_order_release);
        ctx.present_worker_sleeping.store(false, std::memory_order_release);
        ctx.main_waiting_on_present_worker.store(false, std::memory_order_release);
        ctx.window_focused_atomic.store(ctx.last_window_focused, std::memory_order_release);
        ctx.last_main_thread_cpu = ~0u;
        ctx.present_worker_thread = CreateThread(
            nullptr, 0, &present_worker_thread_proc, nullptr, 0u, nullptr);
    }

    void stop_present_worker()
    {
        wait_for_present_worker_idle();
        if (ctx.present_worker_thread != nullptr)
        {
            ctx.present_worker_state.store(2u, std::memory_order_seq_cst);
            if (ctx.present_worker_wake_event != nullptr)
            {
                SetEvent(ctx.present_worker_wake_event);
            }
            WaitForSingleObject(ctx.present_worker_thread, INFINITE);
            CloseHandle(ctx.present_worker_thread);
            ctx.present_worker_thread = nullptr;
        }
        if (ctx.present_worker_wake_event != nullptr)
        {
            CloseHandle(ctx.present_worker_wake_event);
            ctx.present_worker_wake_event = nullptr;
        }
        if (ctx.present_worker_done_event != nullptr)
        {
            CloseHandle(ctx.present_worker_done_event);
            ctx.present_worker_done_event = nullptr;
        }
        ctx.present_worker_state.store(0u, std::memory_order_release);
        ctx.present_worker_sleeping.store(false, std::memory_order_release);
        ctx.main_waiting_on_present_worker.store(false, std::memory_order_release);
        ctx.window_focused_atomic.store(true, std::memory_order_release);
        ctx.last_main_thread_cpu = ~0u;
    }

    void reset_swapchain_semaphores()
    {
        wait_for_present_worker_idle();
        ctx.has_preacquired_image = false;
        ctx.has_present_result = false;
        ctx.next_sem_idx = 0u;
        if (ctx.device == nullptr)
        {
            return;
        }
        ctx.vk.vkDeviceWaitIdle(ctx.device);
        VkSemaphoreCreateInfo smci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0u};
        for (uint32_t i = 0; i < vulkan_context::sem_ring_size; ++i)
        {
            if (ctx.img_avail_sems[i] != vk_null_handle)
            {
                ctx.vk.vkDestroySemaphore(ctx.device, ctx.img_avail_sems[i], nullptr);
                ctx.img_avail_sems[i] = vk_null_handle;
            }
            ctx.vk.vkCreateSemaphore(ctx.device, &smci, nullptr, &ctx.img_avail_sems[i]);
            if (ctx.render_done_sems[i] != vk_null_handle)
            {
                ctx.vk.vkDestroySemaphore(ctx.device, ctx.render_done_sems[i], nullptr);
                ctx.render_done_sems[i] = vk_null_handle;
            }
            ctx.vk.vkCreateSemaphore(ctx.device, &smci, nullptr, &ctx.render_done_sems[i]);
        }
    }

    namespace
    {
        void make_scene_subpass_dependencies(VkSubpassDependency (&deps)[2])
        {
            constexpr VkFlags scene_stages =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_TRANSFER_BIT;
            constexpr VkFlags scene_access =
                VK_ACCESS_SHADER_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_TRANSFER_READ_BIT |
                VK_ACCESS_TRANSFER_WRITE_BIT;

            deps[0] = {};
            deps[0].srcSubpass = ~0u;
            deps[0].dstSubpass = 0u;
            deps[0].srcStageMask = scene_stages;
            deps[0].dstStageMask = scene_stages;
            deps[0].srcAccessMask = scene_access;
            deps[0].dstAccessMask = scene_access;

            deps[1] = {};
            deps[1].srcSubpass = 0u;
            deps[1].dstSubpass = ~0u;
            deps[1].srcStageMask = scene_stages;
            deps[1].dstStageMask = scene_stages;
            deps[1].srcAccessMask = scene_access;
            deps[1].dstAccessMask = scene_access;
        }

        bool create_present_render_pass()
        {
            VkAttachmentDescription pres_att{};
            pres_att.format = ctx.swapchain_format;
            pres_att.samples = 1;
            pres_att.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            pres_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            pres_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            pres_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            pres_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            pres_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference pres_ref{0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            VkSubpassDescription pres_sub{};
            pres_sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            pres_sub.colorAttachmentCount = 1u;
            pres_sub.pColorAttachments = &pres_ref;

            VkSubpassDependency deps[2]{};
            make_scene_subpass_dependencies(deps);

            VkRenderPassCreateInfo prpci{};
            prpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            prpci.attachmentCount = 1u;
            prpci.pAttachments = &pres_att;
            prpci.subpassCount = 1u;
            prpci.pSubpasses = &pres_sub;
            prpci.dependencyCount = 2u;
            prpci.pDependencies = deps;

            if (ctx.vk.vkCreateRenderPass(ctx.device, &prpci, nullptr, &ctx.present_render_pass) != VK_SUCCESS)
            {
                ctx.present_render_pass = vk_null_handle;
                return false;
            }
            ctx.present_render_pass_format = ctx.swapchain_format;
            return true;
        }
    }

    bool create_render_passes()
    {
        // keep color and depth/stencil in attachment-optimal layouts across passes so
        // compression metadata does not decompress between draws
        VkAttachmentDescription scene_atts[2]{};
        scene_atts[0].format = VK_FORMAT_R8G8B8A8_UNORM;
        scene_atts[0].samples = 1;
        scene_atts[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        scene_atts[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        scene_atts[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        scene_atts[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        scene_atts[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        scene_atts[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        scene_atts[1].format = ctx.depth_stencil_format;
        scene_atts[1].samples = 1;
        scene_atts[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        scene_atts[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        scene_atts[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        scene_atts[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        scene_atts[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        scene_atts[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_ref{0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference ds_ref{1u, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1u;
        subpass.pColorAttachments = &color_ref;
        subpass.pDepthStencilAttachment = &ds_ref;

        VkSubpassDependency deps[2]{};
        make_scene_subpass_dependencies(deps);

        VkRenderPassCreateInfo rpci{};
        rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpci.attachmentCount = 2u;
        rpci.pAttachments = scene_atts;
        rpci.subpassCount = 1u;
        rpci.pSubpasses = &subpass;
        rpci.dependencyCount = 2u;
        rpci.pDependencies = deps;

        if (ctx.vk.vkCreateRenderPass(ctx.device, &rpci, nullptr, &ctx.scene_render_pass) != VK_SUCCESS)
        {
            return false;
        }

        auto destroy_created_render_passes = [&]()
        {
            auto destroy_rp = [&](VkRenderPass& rp)
            {
                if (rp != vk_null_handle)
                {
                    ctx.vk.vkDestroyRenderPass(ctx.device, rp, nullptr);
                    rp = vk_null_handle;
                }
            };
            destroy_rp(ctx.present_render_pass);
            destroy_rp(ctx.scene_render_pass_clear_cd);
            destroy_rp(ctx.scene_render_pass_clear_d);
            destroy_rp(ctx.scene_render_pass_clear_c);
            destroy_rp(ctx.scene_render_pass);
        };

        scene_atts[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        scene_atts[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        scene_atts[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        if (ctx.vk.vkCreateRenderPass(ctx.device, &rpci, nullptr, &ctx.scene_render_pass_clear_c) != VK_SUCCESS)
        {
            destroy_created_render_passes();
            return false;
        }

        scene_atts[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        scene_atts[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        scene_atts[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        if (ctx.vk.vkCreateRenderPass(ctx.device, &rpci, nullptr, &ctx.scene_render_pass_clear_d) != VK_SUCCESS)
        {
            destroy_created_render_passes();
            return false;
        }

        scene_atts[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        scene_atts[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        scene_atts[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        if (ctx.vk.vkCreateRenderPass(ctx.device, &rpci, nullptr, &ctx.scene_render_pass_clear_cd) != VK_SUCCESS)
        {
            destroy_created_render_passes();
            return false;
        }

        if (!create_present_render_pass())
        {
            destroy_created_render_passes();
            return false;
        }
        return true;
    }

    void destroy_fbo0()
    {
        if (ctx.device == nullptr)
        {
            return;
        }
        if (ctx.fbo0_fb != vk_null_handle)
        {
            ctx.vk.vkDestroyFramebuffer(ctx.device, ctx.fbo0_fb, nullptr);
            ctx.fbo0_fb = vk_null_handle;
        }
        if (ctx.fbo0_color_view != vk_null_handle)
        {
            reset_all_descriptor_caches();
        }
        destroy_vk_image_resources(ctx.fbo0_color_view, ctx.fbo0_color_img, ctx.fbo0_color_mem);
        destroy_vk_image_resources(ctx.fbo0_ds_view, ctx.fbo0_ds_img, ctx.fbo0_ds_mem);
        ctx.fbo0_width = 0;
        ctx.fbo0_height = 0;
        if (ctx.bound_fbo_id == 0)
        {
            ctx.cur_vk_fb = vk_null_handle;
            ctx.cur_color_img = vk_null_handle;
        }
    }

    bool create_fbo0(uint32_t w, uint32_t h)
    {
        destroy_fbo0();
        w = std::max(1u, w);
        h = std::max(1u, h);

        if (!create_image_2d(
                w, h, 1u, VK_FORMAT_R8G8B8A8_UNORM,
                0x01u | 0x02u | 0x04u | 0x10u, 1u,
                ctx.fbo0_color_img, ctx.fbo0_color_mem, ctx.fbo0_color_view))
        {
            destroy_fbo0();
            return false;
        }

        if (!create_image_2d(
                w, h, 1u, ctx.depth_stencil_format,
                0x02u | 0x20u, 0x02u | 0x04u,
                ctx.fbo0_ds_img, ctx.fbo0_ds_mem, ctx.fbo0_ds_view))
        {
            destroy_fbo0();
            return false;
        }

        VkImageView views[2] = {ctx.fbo0_color_view, ctx.fbo0_ds_view};
        VkFramebufferCreateInfo fbci{};
        fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass = ctx.scene_render_pass;
        fbci.attachmentCount = 2u;
        fbci.pAttachments = views;
        fbci.width = w;
        fbci.height = h;
        fbci.layers = 1u;
        if (ctx.vk.vkCreateFramebuffer(ctx.device, &fbci, nullptr, &ctx.fbo0_fb) != VK_SUCCESS)
        {
            destroy_fbo0();
            return false;
        }

        execute_immediate_cmd([&](VkCommandBuffer cmd)
        {
            clear_color_image(cmd, ctx.fbo0_color_img, 1u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            clear_depth_stencil_image(cmd, ctx.fbo0_ds_img);
        });

        ctx.fbo0_width = w;
        ctx.fbo0_height = h;
        if (ctx.bound_fbo_id == 0)
        {
            ctx.cur_fb_w = w;
            ctx.cur_fb_h = h;
            ctx.cur_vk_fb = ctx.fbo0_fb;
            ctx.cur_color_img = ctx.fbo0_color_img;
        }
        return true;
    }

    void destroy_swapchain()
    {
        wait_for_present_worker_idle();
        if (ctx.device == nullptr)
        {
            return;
        }
        for (auto fb : ctx.swapchain_fbs)
        {
            if (fb != vk_null_handle)
            {
                ctx.vk.vkDestroyFramebuffer(ctx.device, fb, nullptr);
            }
        }
        ctx.swapchain_fbs.clear();
        for (auto iv : ctx.swapchain_views)
        {
            if (iv != vk_null_handle)
            {
                ctx.vk.vkDestroyImageView(ctx.device, iv, nullptr);
            }
        }
        ctx.swapchain_views.clear();
        ctx.swapchain_images.clear();
        if (ctx.swapchain != vk_null_handle)
        {
            ctx.vk.vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
            ctx.swapchain = vk_null_handle;
        }
        reset_swapchain_semaphores();
    }

    bool create_or_recreate_swapchain()
    {
        if (ctx.device == nullptr || ctx.surface == vk_null_handle)
        {
            return false;
        }
        if (ctx.hwnd != nullptr && is_window_minimized_or_zero(ctx.hwnd))
        {
            ctx.swapchain_needs_recreate = true;
            ctx.swapchain_out_of_date = true;
            return false;
        }
        wait_for_present_worker_idle();
        ctx.vk.vkDeviceWaitIdle(ctx.device);

        VkBool32 present_supported = VK_FALSE;
        if (ctx.vk.vkGetPhysicalDeviceSurfaceSupportKHR(
                ctx.phys_dev, ctx.graphics_queue_family, ctx.surface, &present_supported) != VK_SUCCESS ||
            present_supported != VK_TRUE)
        {
            return false;
        }

        VkSurfaceCapabilitiesKHR caps{};
        if (ctx.vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.phys_dev, ctx.surface, &caps) != VK_SUCCESS)
        {
            return false;
        }

        uint32_t fmt_count = 0;
        ctx.vk.vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.phys_dev, ctx.surface, &fmt_count, nullptr);
        if (fmt_count == 0)
        {
            return false;
        }
        std::vector<VkSurfaceFormatKHR> fmts(fmt_count);
        ctx.vk.vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.phys_dev, ctx.surface, &fmt_count, fmts.data());
        bool found_surface_format = false;
        for (const auto& f : fmts)
        {
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM)
            {
                ctx.swapchain_format = f.format;
                ctx.swapchain_color_space = f.colorSpace;
                found_surface_format = true;
                break;
            }
        }
        if (!found_surface_format)
        {
            return false;
        }

        uint32_t pm_count = 0;
        ctx.vk.vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.phys_dev, ctx.surface, &pm_count, nullptr);
        std::vector<VkPresentModeKHR> pmodes(std::max(1u, pm_count), VK_PRESENT_MODE_FIFO_KHR);
        if (pm_count > 0)
        {
            ctx.vk.vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.phys_dev, ctx.surface, &pm_count, pmodes.data());
        }

        VkPresentModeKHR chosen_pm = VK_PRESENT_MODE_FIFO_KHR;
        if (ctx.swap_interval <= 0)
        {
            for (auto pm : pmodes)
            {
                if (pm == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    chosen_pm = VK_PRESENT_MODE_IMMEDIATE_KHR;
                    break;
                }
                if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    chosen_pm = VK_PRESENT_MODE_MAILBOX_KHR;
                }
            }
        }

        VkExtent2D ext = caps.currentExtent;
        if (ext.width == 0 || ext.height == 0 ||
            caps.maxImageExtent.width == 0 || caps.maxImageExtent.height == 0)
        {
            ctx.swapchain_needs_recreate = true;
            ctx.swapchain_out_of_date = true;
            return false;
        }
        if (ext.width == ~0u)
        {
            ext.width = std::clamp(ctx.vid_width, caps.minImageExtent.width, caps.maxImageExtent.width);
            ext.height = std::clamp(ctx.vid_height, caps.minImageExtent.height, caps.maxImageExtent.height);
        }
        if (ext.width == 0 || ext.height == 0)
        {
            ctx.swapchain_needs_recreate = true;
            ctx.swapchain_out_of_date = true;
            return false;
        }
        ctx.swapchain_extent = ext;

        destroy_swapchain();

        if (ctx.scene_render_pass == vk_null_handle)
        {
            if (!create_render_passes())
            {
                return false;
            }
        }
        else if (ctx.present_render_pass == vk_null_handle ||
                 ctx.present_render_pass_format != ctx.swapchain_format)
        {
            for (auto it = ctx.pipelines.begin(); it != ctx.pipelines.end();)
            {
                if (it->first.is_present_pass != 0)
                {
                    if (ctx.bound_pipeline == it->second)
                    {
                        ctx.bound_pipeline = vk_null_handle;
                    }
                    if (ctx.cached_pipeline == it->second)
                    {
                        ctx.cached_pipeline = vk_null_handle;
                        ctx.pso_key_dirty = true;
                    }
                    if (it->second != vk_null_handle)
                    {
                        ctx.vk.vkDestroyPipeline(ctx.device, it->second, nullptr);
                    }
                    it = ctx.pipelines.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            if (ctx.present_render_pass != vk_null_handle)
            {
                ctx.vk.vkDestroyRenderPass(ctx.device, ctx.present_render_pass, nullptr);
                ctx.present_render_pass = vk_null_handle;
            }
            if (!create_present_render_pass())
            {
                return false;
            }
        }

        uint32_t img_count = std::max(2u, caps.minImageCount + 1u);
        if (caps.maxImageCount > 0 && img_count > caps.maxImageCount)
        {
            img_count = caps.maxImageCount;
        }

        VkSwapchainCreateInfoKHR sci{};
        sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        sci.surface = ctx.surface;
        sci.minImageCount = img_count;
        sci.imageFormat = ctx.swapchain_format;
        sci.imageColorSpace = ctx.swapchain_color_space;
        sci.imageExtent = ext;
        sci.imageArrayLayers = 1u;
        sci.imageUsage = 0x10u | 0x02u;
        sci.imageSharingMode = 0;
        sci.preTransform = caps.currentTransform ? caps.currentTransform : 1u;
        sci.compositeAlpha = 1u;
        sci.presentMode = chosen_pm;
        sci.clipped = VK_TRUE;

        if (ctx.vk.vkCreateSwapchainKHR(ctx.device, &sci, nullptr, &ctx.swapchain) != VK_SUCCESS)
        {
            return false;
        }

        uint32_t sc_count = 0;
        if (ctx.vk.vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &sc_count, nullptr) != VK_SUCCESS ||
            sc_count == 0u)
        {
            destroy_swapchain();
            return false;
        }
        ctx.swapchain_images.resize(sc_count);
        if (ctx.vk.vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &sc_count, ctx.swapchain_images.data()) != VK_SUCCESS)
        {
            destroy_swapchain();
            return false;
        }

        ctx.swapchain_views.resize(sc_count, vk_null_handle);
        ctx.swapchain_fbs.resize(sc_count, vk_null_handle);
        for (uint32_t i = 0; i < sc_count; ++i)
        {
            VkImageViewCreateInfo ivci{};
            ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ivci.image = ctx.swapchain_images[i];
            ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ivci.format = ctx.swapchain_format;
            ivci.subresourceRange = {1u, 0u, 1u, 0u, 1u};
            if (ctx.vk.vkCreateImageView(ctx.device, &ivci, nullptr, &ctx.swapchain_views[i]) != VK_SUCCESS)
            {
                destroy_swapchain();
                return false;
            }

            VkFramebufferCreateInfo fbci{};
            fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbci.renderPass = ctx.present_render_pass;
            fbci.attachmentCount = 1u;
            fbci.pAttachments = &ctx.swapchain_views[i];
            fbci.width = ext.width;
            fbci.height = ext.height;
            fbci.layers = 1u;
            if (ctx.vk.vkCreateFramebuffer(ctx.device, &fbci, nullptr, &ctx.swapchain_fbs[i]) != VK_SUCCESS)
            {
                destroy_swapchain();
                return false;
            }
        }

        uint32_t target_fbo_w = ext.width;
        uint32_t target_fbo_h = ext.height;
        if (ctx.active)
        {
            const ptrdiff_t slide = static_cast<ptrdiff_t>(ctx.engine_base) -
                                    static_cast<ptrdiff_t>(preferred_base);
            const int eng_w = *reinterpret_cast<const int*>(static_cast<uintptr_t>(va_glconfig_width + slide));
            const int eng_h = *reinterpret_cast<const int*>(static_cast<uintptr_t>(va_glconfig_height + slide));
            if (eng_w > 0 && eng_h > 0)
            {
                target_fbo_w = static_cast<uint32_t>(eng_w);
                target_fbo_h = static_cast<uint32_t>(eng_h);
            }
        }

        if (ctx.fbo0_fb == vk_null_handle ||
            ctx.fbo0_width != target_fbo_w ||
            ctx.fbo0_height != target_fbo_h)
        {
            ctx.vid_width = target_fbo_w;
            ctx.vid_height = target_fbo_h;
            if (!create_fbo0(target_fbo_w, target_fbo_h))
            {
                destroy_swapchain();
                return false;
            }
        }
        ctx.swapchain_needs_recreate = false;
        ctx.swapchain_out_of_date = false;
        start_present_worker_if_needed();
        return true;
    }

    bool ensure_vulkan_device_created(HDC hdc)
    {
        if (ctx.context_created)
        {
            return true;
        }
        const HWND hwnd = (hdc != nullptr) ? WindowFromDC(hdc) : nullptr;
        if (hwnd == nullptr || !IsWindow(hwnd))
        {
            return false;
        }
        if (!ctx.vk.load())
        {
            return false;
        }

        auto fail_init = [&]() -> bool
        {
            rollback_failed_device_creation();
            return false;
        };

        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "QuakeLive_QLQPC_Vulkan";
        app_info.applicationVersion = 1u;
        app_info.pEngineName = "QLQPC_Vulkan";
        app_info.engineVersion = 1u;
        app_info.apiVersion = VK_API_VERSION_1_0;

        const char* inst_exts[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
        VkInstanceCreateInfo ici{};
        ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ici.pApplicationInfo = &app_info;
        ici.enabledExtensionCount = 2u;
        ici.ppEnabledExtensionNames = inst_exts;

        if (ctx.vk.vkCreateInstance(&ici, nullptr, &ctx.instance) != VK_SUCCESS)
        {
            ctx.instance = nullptr;
            return fail_init();
        }
        if (!ctx.vk.load_instance(ctx.instance))
        {
            return fail_init();
        }

        ctx.hdc = hdc;
        ctx.hwnd = hwnd;
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
            ctx.surface = vk_null_handle;
            return fail_init();
        }

        uint32_t gpu_count = 0;
        if (ctx.vk.vkEnumeratePhysicalDevices(ctx.instance, &gpu_count, nullptr) != VK_SUCCESS || gpu_count == 0)
        {
            return fail_init();
        }
        std::vector<VkPhysicalDevice> gpus(gpu_count);
        if (ctx.vk.vkEnumeratePhysicalDevices(ctx.instance, &gpu_count, gpus.data()) != VK_SUCCESS || gpu_count == 0)
        {
            return fail_init();
        }

        auto find_graphics_queue_family = [](VkPhysicalDevice pd, uint32_t& out_qf) -> bool
        {
            uint32_t qf_count = 0;
            ctx.vk.vkGetPhysicalDeviceQueueFamilyProperties(pd, &qf_count, nullptr);
            if (qf_count == 0)
            {
                return false;
            }
            std::vector<VkQueueFamilyProperties> qfs(qf_count);
            ctx.vk.vkGetPhysicalDeviceQueueFamilyProperties(pd, &qf_count, qfs.data());
            for (uint32_t i = 0; i < qf_count; ++i)
            {
                if ((qfs[i].queueFlags & 0x01u) != 0)
                {
                    VkBool32 present_supported = VK_FALSE;
                    if (ctx.vk.vkGetPhysicalDeviceSurfaceSupportKHR(
                            pd, i, ctx.surface, &present_supported) == VK_SUCCESS &&
                        present_supported == VK_TRUE)
                    {
                        out_qf = i;
                        return true;
                    }
                }
            }
            return false;
        };

        VkPhysicalDevice chosen_pd = nullptr;
        uint32_t chosen_qf = 0;
        for (auto pd : gpus)
        {
            VkPhysicalDeviceProperties props{};
            ctx.vk.vkGetPhysicalDeviceProperties(pd, &props);
            uint32_t qf = 0;
            if (props.deviceType == 2u && find_graphics_queue_family(pd, qf))
            {
                chosen_pd = pd;
                chosen_qf = qf;
                break;
            }
        }
        if (chosen_pd == nullptr)
        {
            for (auto pd : gpus)
            {
                uint32_t qf = 0;
                if (find_graphics_queue_family(pd, qf))
                {
                    chosen_pd = pd;
                    chosen_qf = qf;
                    break;
                }
            }
        }
        if (chosen_pd == nullptr)
        {
            return fail_init();
        }
        ctx.phys_dev = chosen_pd;
        ctx.graphics_queue_family = chosen_qf;
        ctx.vk.vkGetPhysicalDeviceProperties(ctx.phys_dev, &ctx.phys_props);
        ctx.vk.vkGetPhysicalDeviceMemoryProperties(ctx.phys_dev, &ctx.mem_props);
        ctx.gpu_name_str = ctx.phys_props.deviceName;
        ctx.gl_renderer_str = ctx.gpu_name_str;
        switch (ctx.phys_props.vendorID)
        {
            case 0x1002u:
                ctx.gl_vendor_str = "AMD";
                break;
            case 0x10DEu:
                ctx.gl_vendor_str = "NVIDIA";
                break;
            case 0x8086u:
                ctx.gl_vendor_str = "Intel";
                break;
            default:
                ctx.gl_vendor_str = "Vulkan";
                break;
        }

        VkFormatProperties fp{};
        ctx.vk.vkGetPhysicalDeviceFormatProperties(ctx.phys_dev, VK_FORMAT_D32_SFLOAT_S8_UINT, &fp);
        if ((fp.optimalTilingFeatures & 0x200u) != 0)
        {
            ctx.depth_stencil_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
        else
        {
            ctx.vk.vkGetPhysicalDeviceFormatProperties(ctx.phys_dev, VK_FORMAT_D24_UNORM_S8_UINT, &fp);
            if ((fp.optimalTilingFeatures & 0x200u) != 0)
            {
                ctx.depth_stencil_format = VK_FORMAT_D24_UNORM_S8_UINT;
            }
            else
            {
                return fail_init();
            }
        }

        const float q_prio = 1.0f;
        VkDeviceQueueCreateInfo dqci{};
        dqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        dqci.queueFamilyIndex = ctx.graphics_queue_family;
        dqci.queueCount = 1u;
        dqci.pQueuePriorities = &q_prio;

        VkPhysicalDeviceFeatures sup_feats{};
        ctx.vk.vkGetPhysicalDeviceFeatures(ctx.phys_dev, &sup_feats);
        VkPhysicalDeviceFeatures req_feats{};
        req_feats.fillModeNonSolid = sup_feats.fillModeNonSolid;
        req_feats.samplerAnisotropy = sup_feats.samplerAnisotropy;

        const char* dev_exts[] = {"VK_KHR_swapchain"};
        VkDeviceCreateInfo dci{};
        dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dci.queueCreateInfoCount = 1u;
        dci.pQueueCreateInfos = &dqci;
        dci.enabledExtensionCount = 1u;
        dci.ppEnabledExtensionNames = dev_exts;
        dci.pEnabledFeatures = &req_feats;

        if (ctx.vk.vkCreateDevice(ctx.phys_dev, &dci, nullptr, &ctx.device) != VK_SUCCESS)
        {
            ctx.device = nullptr;
            return fail_init();
        }
        if (!ctx.vk.load_device(ctx.device))
        {
            return fail_init();
        }
        ctx.vk.vkGetDeviceQueue(ctx.device, ctx.graphics_queue_family, 0u, &ctx.queue);

        VkCommandPoolCreateInfo cpci{};
        cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cpci.flags = 0x02u;
        cpci.queueFamilyIndex = ctx.graphics_queue_family;

        if (ctx.vk.vkCreateCommandPool(ctx.device, &cpci, nullptr, &ctx.upload_cmd_pool) != VK_SUCCESS)
        {
            return fail_init();
        }
        VkCommandBufferAllocateInfo cbai{};
        cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool = ctx.upload_cmd_pool;
        cbai.level = 0;
        cbai.commandBufferCount = 1u;
        if (ctx.vk.vkAllocateCommandBuffers(ctx.device, &cbai, &ctx.upload_cmd) != VK_SUCCESS)
        {
            return fail_init();
        }

        for (size_t i = 0; i < vulkan_context::frames_in_flight; ++i)
        {
            if (ctx.vk.vkCreateCommandPool(ctx.device, &cpci, nullptr, &ctx.frame_cmd_pools[i]) != VK_SUCCESS)
            {
                return fail_init();
            }
            cbai.commandPool = ctx.frame_cmd_pools[i];
            cbai.commandBufferCount = 1u;
            if (ctx.vk.vkAllocateCommandBuffers(ctx.device, &cbai, &ctx.frame_cmds[i]) != VK_SUCCESS)
            {
                return fail_init();
            }
        }

        VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 1u};
        VkSemaphoreCreateInfo smci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0u};
        if (ctx.vk.vkCreateFence(ctx.device, &fci, nullptr, &ctx.upload_fence) != VK_SUCCESS)
        {
            return fail_init();
        }

        for (uint32_t i = 0; i < vulkan_context::sem_ring_size; ++i)
        {
            if (ctx.vk.vkCreateSemaphore(ctx.device, &smci, nullptr, &ctx.img_avail_sems[i]) != VK_SUCCESS ||
                ctx.vk.vkCreateSemaphore(ctx.device, &smci, nullptr, &ctx.render_done_sems[i]) != VK_SUCCESS)
            {
                return fail_init();
            }
        }

        for (size_t i = 0; i < vulkan_context::frames_in_flight; ++i)
        {
            if (ctx.vk.vkCreateFence(ctx.device, &fci, nullptr, &ctx.frame_fences[i]) != VK_SUCCESS)
            {
                return fail_init();
            }

            VkDescriptorPoolSize dps{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16384u};
            VkDescriptorPoolCreateInfo dpci{};
            dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            dpci.maxSets = 8192u;
            dpci.poolSizeCount = 1u;
            dpci.pPoolSizes = &dps;
            if (ctx.vk.vkCreateDescriptorPool(ctx.device, &dpci, nullptr, &ctx.desc_pools[i]) != VK_SUCCESS)
            {
                return fail_init();
            }
            ctx.desc_caches[i].reserve(4096u);

            if (!create_buffer(
                    vulkan_context::vbo_size, 0x80u,
                    0x02u | 0x04u,
                    ctx.vbo_bufs[i], ctx.vbo_mems[i],
                    reinterpret_cast<void**>(&ctx.vbo_mapped[i])))
            {
                return fail_init();
            }
            if (!create_buffer(
                    vulkan_context::ibo_size, 0x40u,
                    0x02u | 0x04u,
                    ctx.ibo_bufs[i], ctx.ibo_mems[i],
                    reinterpret_cast<void**>(&ctx.ibo_mapped[i])))
            {
                return fail_init();
            }
            if (!create_buffer(
                    vulkan_context::staging_size, 0x01u | 0x02u,
                    0x02u | 0x04u,
                    ctx.staging_bufs[i], ctx.staging_mems[i],
                    reinterpret_cast<void**>(&ctx.staging_mapped[i])))
            {
                return fail_init();
            }
        }

        if (ctx.textures.size() < 4096u)
        {
            ctx.textures.resize(4096u);
        }

        VkDescriptorSetLayoutBinding bindings[2]{};
        bindings[0] = {0u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, 0x10u, nullptr};
        bindings[1] = {1u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, 0x10u, nullptr};
        VkDescriptorSetLayoutCreateInfo dslci{};
        dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dslci.bindingCount = 2u;
        dslci.pBindings = bindings;
        if (ctx.vk.vkCreateDescriptorSetLayout(ctx.device, &dslci, nullptr, &ctx.desc_set_layout) != VK_SUCCESS)
        {
            return fail_init();
        }

        VkPushConstantRange pcr{};
        pcr.stageFlags = 0x01u | 0x10u;
        pcr.offset = 0u;
        pcr.size = sizeof(shader_push_constants);
        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.setLayoutCount = 1u;
        plci.pSetLayouts = &ctx.desc_set_layout;
        plci.pushConstantRangeCount = 1u;
        plci.pPushConstantRanges = &pcr;
        if (ctx.vk.vkCreatePipelineLayout(ctx.device, &plci, nullptr, &ctx.pipeline_layout) != VK_SUCCESS)
        {
            return fail_init();
        }
        init_pipeline_cache();

        const auto vert_spv = build_vertex_shader_spirv();
        VkShaderModuleCreateInfo vsmci{};
        vsmci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vsmci.codeSize = vert_spv.size() * sizeof(uint32_t);
        vsmci.pCode = vert_spv.data();
        if (ctx.vk.vkCreateShaderModule(ctx.device, &vsmci, nullptr, &ctx.vert_shader_mod) != VK_SUCCESS)
        {
            return fail_init();
        }

        ctx.white_tex.width = 1u;
        ctx.white_tex.height = 1u;
        ctx.white_tex.mip_levels = 1u;
        ctx.white_tex.wrap_s = GL_CLAMP_TO_EDGE;
        ctx.white_tex.wrap_t = GL_CLAMP_TO_EDGE;
        if (!create_image_2d(
                1u, 1u, 1u, VK_FORMAT_R8G8B8A8_UNORM, 0x02u | 0x04u, 1u,
                ctx.white_tex.image, ctx.white_tex.memory, ctx.white_tex.view))
        {
            return fail_init();
        }
        rebuild_texture_sampler(ctx.white_tex);
        const uint8_t white_px[4] = {255u, 255u, 255u, 255u};
        upload_texture_level(ctx.white_tex, 0u, 0, 0, 1u, 1u, GL_RGBA, GL_UNSIGNED_BYTE, white_px);

        ctx.gamma_lut_tex.width = 256u;
        ctx.gamma_lut_tex.height = 1u;
        ctx.gamma_lut_tex.mip_levels = 1u;
        ctx.gamma_lut_tex.wrap_s = GL_CLAMP_TO_EDGE;
        ctx.gamma_lut_tex.wrap_t = GL_CLAMP_TO_EDGE;
        if (!create_image_2d(
                256u, 1u, 1u, VK_FORMAT_R8G8B8A8_UNORM, 0x02u | 0x04u, 1u,
                ctx.gamma_lut_tex.image, ctx.gamma_lut_tex.memory, ctx.gamma_lut_tex.view))
        {
            return fail_init();
        }
        rebuild_texture_sampler(ctx.gamma_lut_tex);
        uint8_t id_lut[256 * 4]{};
        for (int i = 0; i < 256; ++i)
        {
            const auto v = static_cast<uint16_t>((i << 8) | i);
            ctx.gamma_ramp[0][i] = v;
            ctx.gamma_ramp[1][i] = v;
            ctx.gamma_ramp[2][i] = v;
            id_lut[i * 4 + 0] = static_cast<uint8_t>(i);
            id_lut[i * 4 + 1] = static_cast<uint8_t>(i);
            id_lut[i * 4 + 2] = static_cast<uint8_t>(i);
            id_lut[i * 4 + 3] = 255u;
        }
        ctx.gamma_identity = true;
        upload_texture_level(ctx.gamma_lut_tex, 0u, 0, 0, 256u, 1u, GL_RGBA, GL_UNSIGNED_BYTE, id_lut);

        if (!create_or_recreate_swapchain())
        {
            return fail_init();
        }

        ctx.viewport[0] = 0;
        ctx.viewport[1] = 0;
        ctx.viewport[2] = static_cast<int>(ctx.cur_fb_w);
        ctx.viewport[3] = static_cast<int>(ctx.cur_fb_h);
        ctx.scissor[0] = 0;
        ctx.scissor[1] = 0;
        ctx.scissor[2] = static_cast<int>(ctx.cur_fb_w);
        ctx.scissor[3] = static_cast<int>(ctx.cur_fb_h);

        ctx.context_created = true;
        return true;
    }

    void destroy_vulkan_device()
    {
        if (!ctx.context_created && ctx.device == nullptr && ctx.instance == nullptr)
        {
            return;
        }
        if (ctx.context_created)
        {
            teardown_context_and_surface();
        }
        rollback_failed_device_creation();
    }

    void end_active_render_pass()
    {
        if (ctx.render_pass_active && ctx.active_cmd != nullptr)
        {
            ctx.vk.vkCmdEndRenderPass(ctx.active_cmd);
            ctx.render_pass_active = false;
            if (ctx.bound_fbo_id != 0 && ctx.cur_color_img != vk_null_handle)
            {
                cmd_transition_image(
                    ctx.active_cmd, ctx.cur_color_img,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 1u, 1u, 0u);
            }
            invalidate_cmd_draw_state();
        }
    }

    bool is_window_minimized_or_zero(HWND hwnd)
    {
        if (hwnd == nullptr || !IsWindow(hwnd))
        {
            return false;
        }
        if (IsIconic(hwnd) != FALSE)
        {
            return true;
        }
        RECT rc{};
        if (!GetClientRect(hwnd, &rc) || rc.right <= rc.left || rc.bottom <= rc.top)
        {
            return true;
        }
        return false;
    }

    bool is_window_focused(HWND hwnd)
    {
        if (hwnd == nullptr)
        {
            return true;
        }
        if (!IsWindow(hwnd) || IsIconic(hwnd) != FALSE)
        {
            return false;
        }
        const HWND fg = GetForegroundWindow();
        if (fg == nullptr)
        {
            return false;
        }
        if (fg == hwnd || IsChild(hwnd, fg) != FALSE)
        {
            return true;
        }
        const HWND root_hw = GetAncestor(hwnd, GA_ROOT);
        const HWND root_fg = GetAncestor(fg, GA_ROOT);
        if (root_hw != nullptr && (root_fg == root_hw || GetWindow(fg, GW_OWNER) == root_hw))
        {
            return true;
        }
        return false;
    }

    swapchain_frame_action evaluate_pre_present_action(const swapchain_focus_policy_input& in)
    {
        if (in.window_minimized)
        {
            return swapchain_frame_action::skip_frame;
        }
        if (!in.window_focused &&
            (in.swapchain_out_of_date || !in.swapchain_valid))
        {
            return swapchain_frame_action::skip_frame;
        }
        if (!in.swapchain_valid)
        {
            return swapchain_frame_action::recreate_and_proceed;
        }
        if (in.window_focused &&
            (in.swapchain_out_of_date || (in.focus_restored && in.swapchain_needs_recreate)))
        {
            return swapchain_frame_action::recreate_and_proceed;
        }
        return swapchain_frame_action::proceed;
    }

    bool should_recreate_on_suboptimal(
        bool window_minimized,
        bool window_focused,
        bool suboptimal_recreate_attempted)
    {
        if (window_minimized || !window_focused)
        {
            return false;
        }
        return !suboptimal_recreate_attempted;
    }

    void poll_hwnd_state_throttled(HWND hwnd)
    {
        const auto publish_focus_atomic = []()
        {
            ctx.window_focused_atomic.store(
                !ctx.cached_window_minimized && ctx.cached_window_focused,
                std::memory_order_relaxed);
        };
        if (hwnd == nullptr)
        {
            ctx.last_polled_hwnd = nullptr;
            ctx.cached_window_valid = false;
            ctx.cached_window_minimized = false;
            ctx.cached_window_focused = true;
            publish_focus_atomic();
            return;
        }
        if (ctx.window_poll_interval_qpc <= 0)
        {
            LARGE_INTEGER freq{};
            if (QueryPerformanceFrequency(&freq) == FALSE || freq.QuadPart <= 0)
            {
                std::abort();
            }
            ctx.window_poll_interval_qpc = std::max<int64_t>(1, freq.QuadPart / 125);
        }
        LARGE_INTEGER now_qpc{};
        QueryPerformanceCounter(&now_qpc);
        if (hwnd == ctx.last_polled_hwnd &&
            (now_qpc.QuadPart - ctx.last_window_poll_qpc) < ctx.window_poll_interval_qpc)
        {
            return;
        }
        ctx.last_polled_hwnd = hwnd;
        ctx.last_window_poll_qpc = now_qpc.QuadPart;

        if (!IsWindow(hwnd))
        {
            ctx.cached_window_valid = false;
            ctx.cached_window_minimized = false;
            ctx.cached_window_focused = false;
            publish_focus_atomic();
            return;
        }
        ctx.cached_window_valid = true;
        RECT rc{};
        if (IsIconic(hwnd) != FALSE ||
            !GetClientRect(hwnd, &rc) || rc.right <= rc.left || rc.bottom <= rc.top)
        {
            ctx.cached_window_minimized = true;
            ctx.cached_window_focused = false;
            publish_focus_atomic();
            return;
        }
        ctx.cached_window_minimized = false;
        ctx.cached_window_focused = is_window_focused(hwnd);
        ctx.cached_client_w = static_cast<uint32_t>(rc.right - rc.left);
        ctx.cached_client_h = static_cast<uint32_t>(rc.bottom - rc.top);
        publish_focus_atomic();
    }

    void sync_engine_and_window_dimensions()
    {
        if (ctx.cmd_recording || !ctx.context_created)
        {
            return;
        }
        if (ctx.hwnd != nullptr)
        {
            poll_hwnd_state_throttled(ctx.hwnd);
            ctx.window_state_cached_this_frame = true;
            if (ctx.cached_window_valid)
            {
                if (ctx.cached_window_minimized)
                {
                    return;
                }
                const uint32_t win_w = ctx.cached_client_w;
                const uint32_t win_h = ctx.cached_client_h;
                if (win_w > 0 && win_h > 0 && (win_w != ctx.last_client_w || win_h != ctx.last_client_h))
                {
                    ctx.last_client_w = win_w;
                    ctx.last_client_h = win_h;
                    ctx.suboptimal_recreate_attempted = false;
                    if (ctx.swapchain != vk_null_handle &&
                        (win_w != ctx.swapchain_extent.width || win_h != ctx.swapchain_extent.height))
                    {
                        if (!ctx.cached_window_focused)
                        {
                            ctx.swapchain_needs_recreate = true;
                        }
                        else
                        {
                            create_or_recreate_swapchain();
                        }
                    }
                }
            }
        }
        if (ctx.active)
        {
            const ptrdiff_t slide = static_cast<ptrdiff_t>(ctx.engine_base) -
                                    static_cast<ptrdiff_t>(preferred_base);
            const int eng_w = *reinterpret_cast<const int*>(static_cast<uintptr_t>(va_glconfig_width + slide));
            const int eng_h = *reinterpret_cast<const int*>(static_cast<uintptr_t>(va_glconfig_height + slide));
            if (eng_w > 0 && eng_h > 0)
            {
                const auto uw = static_cast<uint32_t>(eng_w);
                const auto uh = static_cast<uint32_t>(eng_h);
                if (ctx.fbo0_fb == vk_null_handle || ctx.fbo0_width != uw || ctx.fbo0_height != uh)
                {
                    wait_for_present_worker_idle();
                    ctx.vk.vkDeviceWaitIdle(ctx.device);
                    ctx.frames_in_flight_busy = false;
                    ctx.frame_fence_pending[0] = false;
                    ctx.frame_fence_pending[1] = false;
                    ctx.vid_width = uw;
                    ctx.vid_height = uh;
                    create_fbo0(uw, uh);
                }
            }
        }
    }

    void begin_frame_if_needed()
    {
        if (ctx.cmd_recording || !ctx.context_created)
        {
            return;
        }
        sync_engine_and_window_dimensions();

        const size_t fi = ctx.cur_frame;
        if (ctx.frame_fence_pending[fi])
        {
            ctx.vk.vkWaitForFences(ctx.device, 1u, &ctx.frame_fences[fi], VK_TRUE, ~0ull);
            ctx.frame_fence_pending[fi] = false;
        }

        if (ctx.desc_caches[fi].size() >= 7168u)
        {
            ctx.vk.vkResetDescriptorPool(ctx.device, ctx.desc_pools[fi], 0u);
            ctx.desc_caches[fi].clear();
        }

        ctx.vbo_offset = 0;
        ctx.ibo_offset = 0;
        ctx.staging_offset = 0;
        // preserve cva_locked across command-buffer flushes inside glLockArraysEXT blocks
        ctx.cva_ibo_valid = false;
        invalidate_cmd_draw_state();

        ctx.active_cmd = ctx.frame_cmds[fi];
        ctx.vk.vkResetCommandBuffer(ctx.active_cmd, 0u);
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = 1u;
        ctx.vk.vkBeginCommandBuffer(ctx.active_cmd, &bi);
        ctx.cmd_recording = true;
    }

    void ensure_render_pass_active()
    {
        begin_frame_if_needed();
        if (ctx.render_pass_active || ctx.active_cmd == nullptr || ctx.cur_vk_fb == vk_null_handle)
        {
            return;
        }
        if (ctx.bound_fbo_id != 0 && ctx.cur_color_img != vk_null_handle)
        {
            cmd_transition_image(
                ctx.active_cmd, ctx.cur_color_img,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1u, 1u, 0u);
        }
        VkRenderPassBeginInfo rpbi{};
        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = ctx.scene_render_pass;
        rpbi.framebuffer = ctx.cur_vk_fb;
        rpbi.renderArea = {{0, 0}, {ctx.cur_fb_w, ctx.cur_fb_h}};
        rpbi.clearValueCount = 0u;
        ctx.vk.vkCmdBeginRenderPass(ctx.active_cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        ctx.render_pass_active = true;
        invalidate_cmd_draw_state();
    }

    bool try_begin_render_pass_with_clear(uint32_t mask)
    {
        if (ctx.render_pass_active)
        {
            return false;
        }
        begin_frame_if_needed();
        if (ctx.render_pass_active || ctx.active_cmd == nullptr || ctx.cur_vk_fb == vk_null_handle)
        {
            return false;
        }
        if (ctx.scissor_enabled)
        {
            const VkRect2D sc = compute_vk_scissor_rect();
            if (sc.offset.x != 0 || sc.offset.y != 0 ||
                sc.extent.width != ctx.cur_fb_w || sc.extent.height != ctx.cur_fb_h)
            {
                return false;
            }
        }

        const bool want_color = (mask & GL_COLOR_BUFFER_BIT) != 0;
        const bool want_depth = (mask & GL_DEPTH_BUFFER_BIT) != 0;
        const bool want_stencil = (mask & GL_STENCIL_BUFFER_BIT) != 0;
        const bool full_ds = want_depth && (!ctx.stencil_test_enabled || want_stencil);

        VkRenderPass rp = vk_null_handle;
        if (want_color && full_ds)
        {
            rp = ctx.scene_render_pass_clear_cd;
        }
        else if (want_color && !want_depth && !want_stencil)
        {
            rp = ctx.scene_render_pass_clear_c;
        }
        else if (!want_color && full_ds)
        {
            rp = ctx.scene_render_pass_clear_d;
        }
        if (rp == vk_null_handle)
        {
            return false;
        }

        if (ctx.bound_fbo_id != 0 && ctx.cur_color_img != vk_null_handle)
        {
            cmd_transition_image(
                ctx.active_cmd, ctx.cur_color_img,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1u, 1u, 0u);
        }

        VkClearValue clear_vals[2]{};
        clear_vals[0].color.float32[0] = ctx.clear_color[0];
        clear_vals[0].color.float32[1] = ctx.clear_color[1];
        clear_vals[0].color.float32[2] = ctx.clear_color[2];
        clear_vals[0].color.float32[3] = ctx.clear_color[3];
        clear_vals[1].depthStencil.depth = ctx.clear_depth;
        clear_vals[1].depthStencil.stencil = ctx.clear_stencil;

        VkRenderPassBeginInfo rpbi{};
        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = rp;
        rpbi.framebuffer = ctx.cur_vk_fb;
        rpbi.renderArea = {{0, 0}, {ctx.cur_fb_w, ctx.cur_fb_h}};
        rpbi.clearValueCount = 2u;
        rpbi.pClearValues = clear_vals;
        ctx.vk.vkCmdBeginRenderPass(ctx.active_cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        ctx.render_pass_active = true;
        invalidate_cmd_draw_state();
        return true;
    }

    void flush_pending_commands_and_wait()
    {
        if (!ctx.context_created || ctx.device == nullptr)
        {
            return;
        }
        wait_for_present_worker_idle();
        if (ctx.cmd_recording)
        {
            end_active_render_pass();
            ctx.vk.vkEndCommandBuffer(ctx.active_cmd);
            ctx.cmd_recording = false;

            const size_t fi = ctx.cur_frame;
            VkSubmitInfo si{};
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.commandBufferCount = 1u;
            si.pCommandBuffers = &ctx.active_cmd;
            ctx.vk.vkResetFences(ctx.device, 1u, &ctx.frame_fences[fi]);
            const VkResult sub = ctx.vk.vkQueueSubmit(ctx.queue, 1u, &si, ctx.frame_fences[fi]);
            ctx.active_cmd = nullptr;
            if (sub == VK_SUCCESS)
            {
                ctx.frame_fence_pending[fi] = true;
                ctx.frames_in_flight_busy = true;
            }
            else
            {
                ctx.frame_fence_pending[fi] = false;
            }
        }
        if (ctx.frames_in_flight_busy)
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
        ctx.vbo_offset = 0;
        ctx.ibo_offset = 0;
        ctx.staging_offset = 0;
        ctx.cva_ibo_valid = false;
        invalidate_cmd_draw_state();
    }
}
