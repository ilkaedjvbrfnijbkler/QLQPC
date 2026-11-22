#pragma once

#include <cstdint>

namespace qlqpc::vulkan
{
    bool init_qgl_vulkan(uintptr_t engine_base);
    void shutdown_qgl_vulkan(uintptr_t engine_base);
    void destroy_vulkan_device();
    bool is_vulkan_backend_active();
}
