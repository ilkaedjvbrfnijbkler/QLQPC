#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>

namespace qlqpc::vulkan
{
    #define QLVK_PTR __stdcall

    using VkFlags = uint32_t;
    using VkBool32 = uint32_t;
    using VkDeviceSize = uint64_t;
    using VkSampleMask = uint32_t;

    // dispatchable handles
    using VkInstance = struct VkInstance_T*;
    using VkPhysicalDevice = struct VkPhysicalDevice_T*;
    using VkDevice = struct VkDevice_T*;
    using VkQueue = struct VkQueue_T*;
    using VkCommandBuffer = struct VkCommandBuffer_T*;

    // non-dispatchable handles are always 64-bit even on 32-bit x86
    using VkSurfaceKHR = uint64_t;
    using VkSwapchainKHR = uint64_t;
    using VkBuffer = uint64_t;
    using VkImage = uint64_t;
    using VkDeviceMemory = uint64_t;
    using VkImageView = uint64_t;
    using VkShaderModule = uint64_t;
    using VkPipelineCache = uint64_t;
    using VkPipelineLayout = uint64_t;
    using VkRenderPass = uint64_t;
    using VkPipeline = uint64_t;
    using VkDescriptorSetLayout = uint64_t;
    using VkSampler = uint64_t;
    using VkDescriptorPool = uint64_t;
    using VkDescriptorSet = uint64_t;
    using VkFramebuffer = uint64_t;
    using VkCommandPool = uint64_t;
    using VkFence = uint64_t;
    using VkSemaphore = uint64_t;

    inline constexpr uint64_t vk_null_handle = 0ull;
    inline constexpr VkBool32 VK_TRUE = 1u;
    inline constexpr VkBool32 VK_FALSE = 0u;
    inline constexpr uint32_t VK_API_VERSION_1_0 = (1u << 22);
    inline constexpr uint32_t VK_QUEUE_FAMILY_IGNORED = ~0u;

    enum VkResult : int32_t
    {
        VK_SUCCESS = 0,
        VK_SUBOPTIMAL_KHR = 1000001003,
        VK_ERROR_OUT_OF_DATE_KHR = -1000001004,
        VK_ERROR_SURFACE_LOST_KHR = -1000000000,
    };

    enum VkStructureType : int32_t
    {
        VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2,
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
        VK_STRUCTURE_TYPE_SUBMIT_INFO = 4,
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5,
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8,
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO = 9,
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12,
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO = 14,
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO = 15,
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO = 16,
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO = 17,
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO = 18,
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO = 19,
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO = 20,
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO = 22,
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO = 23,
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO = 24,
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO = 25,
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO = 26,
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO = 27,
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO = 28,
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO = 30,
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO = 31,
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO = 32,
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO = 33,
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO = 34,
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET = 35,
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO = 37,
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO = 38,
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 39,
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 40,
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 42,
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO = 43,
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER = 45,
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR = 1000009000,
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR = 1000001000,
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR = 1000001001,
    };

    enum VkFormat : int32_t
    {
        VK_FORMAT_R8G8B8A8_UNORM = 37,
        VK_FORMAT_B8G8R8A8_UNORM = 44,
        VK_FORMAT_R32G32_SFLOAT = 103,
        VK_FORMAT_R32G32B32_SFLOAT = 106,
        VK_FORMAT_D24_UNORM_S8_UINT = 129,
        VK_FORMAT_D32_SFLOAT_S8_UINT = 130,
    };

    enum VkImageLayout : int32_t
    {
        VK_IMAGE_LAYOUT_UNDEFINED = 0,
        VK_IMAGE_LAYOUT_GENERAL = 1,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002,
    };

    enum VkPresentModeKHR : int32_t
    {
        VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
        VK_PRESENT_MODE_MAILBOX_KHR = 1,
        VK_PRESENT_MODE_FIFO_KHR = 2,
    };

    enum VkPrimitiveTopology : int32_t
    {
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    };

    enum VkPolygonMode : int32_t
    {
        VK_POLYGON_MODE_FILL = 0,
        VK_POLYGON_MODE_LINE = 1,
    };

    enum VkFrontFace : int32_t
    {
        VK_FRONT_FACE_COUNTER_CLOCKWISE = 0,
        VK_FRONT_FACE_CLOCKWISE = 1,
    };

    enum VkCompareOp : int32_t
    {
        VK_COMPARE_OP_NEVER = 0,
        VK_COMPARE_OP_LESS = 1,
        VK_COMPARE_OP_EQUAL = 2,
        VK_COMPARE_OP_LESS_OR_EQUAL = 3,
        VK_COMPARE_OP_GREATER = 4,
        VK_COMPARE_OP_NOT_EQUAL = 5,
        VK_COMPARE_OP_GREATER_OR_EQUAL = 6,
        VK_COMPARE_OP_ALWAYS = 7,
    };

    enum VkStencilOp : int32_t
    {
        VK_STENCIL_OP_KEEP = 0,
        VK_STENCIL_OP_ZERO = 1,
        VK_STENCIL_OP_REPLACE = 2,
        VK_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
        VK_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
        VK_STENCIL_OP_INVERT = 5,
        VK_STENCIL_OP_INCREMENT_AND_WRAP = 6,
        VK_STENCIL_OP_DECREMENT_AND_WRAP = 7,
    };

    enum VkBlendFactor : int32_t
    {
        VK_BLEND_FACTOR_ZERO = 0,
        VK_BLEND_FACTOR_ONE = 1,
        VK_BLEND_FACTOR_SRC_COLOR = 2,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
        VK_BLEND_FACTOR_DST_COLOR = 4,
        VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
        VK_BLEND_FACTOR_SRC_ALPHA = 6,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
        VK_BLEND_FACTOR_DST_ALPHA = 8,
        VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
    };

    enum VkBlendOp : int32_t
    {
        VK_BLEND_OP_ADD = 0,
    };

    enum VkDynamicState : int32_t
    {
        VK_DYNAMIC_STATE_VIEWPORT = 0,
        VK_DYNAMIC_STATE_SCISSOR = 1,
        VK_DYNAMIC_STATE_DEPTH_BIAS = 3,
        VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK = 6,
        VK_DYNAMIC_STATE_STENCIL_WRITE_MASK = 7,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE = 8,
    };

    enum VkFilter : int32_t
    {
        VK_FILTER_NEAREST = 0,
        VK_FILTER_LINEAR = 1,
    };

    enum VkSamplerMipmapMode : int32_t
    {
        VK_SAMPLER_MIPMAP_MODE_NEAREST = 0,
        VK_SAMPLER_MIPMAP_MODE_LINEAR = 1,
    };

    enum VkSamplerAddressMode : int32_t
    {
        VK_SAMPLER_ADDRESS_MODE_REPEAT = 0,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    };

    inline constexpr int32_t VK_ATTACHMENT_LOAD_OP_LOAD = 0;
    inline constexpr int32_t VK_ATTACHMENT_LOAD_OP_CLEAR = 1;
    inline constexpr int32_t VK_ATTACHMENT_LOAD_OP_DONT_CARE = 2;
    inline constexpr int32_t VK_ATTACHMENT_STORE_OP_STORE = 0;
    inline constexpr int32_t VK_ATTACHMENT_STORE_OP_DONT_CARE = 1;
    inline constexpr int32_t VK_PIPELINE_BIND_POINT_GRAPHICS = 0;
    inline constexpr int32_t VK_SUBPASS_CONTENTS_INLINE = 0;
    inline constexpr int32_t VK_IMAGE_VIEW_TYPE_2D = 1;
    inline constexpr int32_t VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1;
    inline constexpr int32_t VK_INDEX_TYPE_UINT32 = 1;
    inline constexpr uint8_t VK_CULL_MODE_NONE = 0;
    inline constexpr uint8_t VK_CULL_MODE_FRONT_BIT = 1;
    inline constexpr uint8_t VK_CULL_MODE_BACK_BIT = 2;
    inline constexpr uint8_t VK_CULL_MODE_FRONT_AND_BACK = 3;

    inline constexpr VkFlags VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001u;
    inline constexpr VkFlags VK_PIPELINE_STAGE_VERTEX_SHADER_BIT = 0x00000008u;
    inline constexpr VkFlags VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT = 0x00000080u;
    inline constexpr VkFlags VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = 0x00000100u;
    inline constexpr VkFlags VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = 0x00000200u;
    inline constexpr VkFlags VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400u;
    inline constexpr VkFlags VK_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000u;
    inline constexpr VkFlags VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000u;

    inline constexpr VkFlags VK_ACCESS_SHADER_READ_BIT = 0x00000020u;
    inline constexpr VkFlags VK_ACCESS_COLOR_ATTACHMENT_READ_BIT = 0x00000080u;
    inline constexpr VkFlags VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x00000100u;
    inline constexpr VkFlags VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200u;
    inline constexpr VkFlags VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400u;
    inline constexpr VkFlags VK_ACCESS_TRANSFER_READ_BIT = 0x00000800u;
    inline constexpr VkFlags VK_ACCESS_TRANSFER_WRITE_BIT = 0x00001000u;

    struct VkExtent2D
    {
        uint32_t width;
        uint32_t height;
    };

    struct VkExtent3D
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };

    struct VkOffset2D
    {
        int32_t x;
        int32_t y;
    };

    struct VkOffset3D
    {
        int32_t x;
        int32_t y;
        int32_t z;
    };

    struct VkRect2D
    {
        VkOffset2D offset;
        VkExtent2D extent;
    };

    struct VkViewport
    {
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;
    };

    struct VkApplicationInfo
    {
        VkStructureType sType;
        const void* pNext;
        const char* pApplicationName;
        uint32_t applicationVersion;
        const char* pEngineName;
        uint32_t engineVersion;
        uint32_t apiVersion;
    };

    struct VkInstanceCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        const VkApplicationInfo* pApplicationInfo;
        uint32_t enabledLayerCount;
        const char* const* ppEnabledLayerNames;
        uint32_t enabledExtensionCount;
        const char* const* ppEnabledExtensionNames;
    };

    struct VkPhysicalDeviceLimits
    {
        uint8_t raw[504];
    };

    struct VkPhysicalDeviceSparseProperties
    {
        VkBool32 residencyStandard2DBlockShape;
        VkBool32 residencyStandard2DMSBlockShape;
        VkBool32 residencyStandard3DBlockShape;
        VkBool32 residencyAlignedMipSize;
        VkBool32 residencyNonResidentStrict;
    };

    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4324)
    #endif
    struct alignas(8) VkPhysicalDeviceProperties
    {
        uint32_t apiVersion;
        uint32_t driverVersion;
        uint32_t vendorID;
        uint32_t deviceID;
        uint32_t deviceType;
        char deviceName[256];
        uint8_t pipelineCacheUUID[16];
        alignas(8) VkPhysicalDeviceLimits limits;
        VkPhysicalDeviceSparseProperties sparseProperties;
    };
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    struct VkPhysicalDeviceFeatures
    {
        VkBool32 robustBufferAccess;
        VkBool32 fullDrawIndexUint32;
        VkBool32 imageCubeArray;
        VkBool32 independentBlend;
        VkBool32 geometryShader;
        VkBool32 tessellationShader;
        VkBool32 sampleRateShading;
        VkBool32 dualSrcBlend;
        VkBool32 logicOp;
        VkBool32 multiDrawIndirect;
        VkBool32 drawIndirectFirstInstance;
        VkBool32 depthClamp;
        VkBool32 depthBiasClamp;
        VkBool32 fillModeNonSolid;
        VkBool32 depthBounds;
        VkBool32 wideLines;
        VkBool32 largePoints;
        VkBool32 alphaToOne;
        VkBool32 multiViewport;
        VkBool32 samplerAnisotropy;
        VkBool32 textureCompressionETC2;
        VkBool32 textureCompressionASTC_LDR;
        VkBool32 textureCompressionBC;
        VkBool32 occlusionQueryPrecise;
        VkBool32 pipelineStatisticsQuery;
        VkBool32 vertexPipelineStoresAndAtomics;
        VkBool32 fragmentStoresAndAtomics;
        VkBool32 shaderTessellationAndGeometryPointSize;
        VkBool32 shaderImageGatherExtended;
        VkBool32 shaderStorageImageExtendedFormats;
        VkBool32 shaderStorageImageMultisample;
        VkBool32 shaderStorageImageReadWithoutFormat;
        VkBool32 shaderStorageImageWriteWithoutFormat;
        VkBool32 shaderUniformBufferArrayDynamicIndexing;
        VkBool32 shaderSampledImageArrayDynamicIndexing;
        VkBool32 shaderStorageBufferArrayDynamicIndexing;
        VkBool32 shaderStorageImageArrayDynamicIndexing;
        VkBool32 shaderClipDistance;
        VkBool32 shaderCullDistance;
        VkBool32 shaderFloat64;
        VkBool32 shaderInt64;
        VkBool32 shaderInt16;
        VkBool32 shaderResourceResidency;
        VkBool32 shaderResourceMinLod;
        VkBool32 sparseBinding;
        VkBool32 sparseResidencyBuffer;
        VkBool32 sparseResidencyImage2D;
        VkBool32 sparseResidencyImage3D;
        VkBool32 sparseResidency2Samples;
        VkBool32 sparseResidency4Samples;
        VkBool32 sparseResidency8Samples;
        VkBool32 sparseResidency16Samples;
        VkBool32 sparseResidencyAliased;
        VkBool32 variableMultisampleRate;
        VkBool32 inheritedQueries;
    };

    struct VkQueueFamilyProperties
    {
        VkFlags queueFlags;
        uint32_t queueCount;
        uint32_t timestampValidBits;
        VkExtent3D minImageTransferGranularity;
    };

    struct VkMemoryType
    {
        VkFlags propertyFlags;
        uint32_t heapIndex;
    };

    struct VkMemoryHeap
    {
        VkDeviceSize size;
        VkFlags flags;
    };

    struct VkPhysicalDeviceMemoryProperties
    {
        uint32_t memoryTypeCount;
        VkMemoryType memoryTypes[32];
        uint32_t memoryHeapCount;
        VkMemoryHeap memoryHeaps[16];
    };

    struct VkFormatProperties
    {
        VkFlags linearTilingFeatures;
        VkFlags optimalTilingFeatures;
        VkFlags bufferFeatures;
    };

    struct VkDeviceQueueCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t queueFamilyIndex;
        uint32_t queueCount;
        const float* pQueuePriorities;
    };

    struct VkDeviceCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t queueCreateInfoCount;
        const VkDeviceQueueCreateInfo* pQueueCreateInfos;
        uint32_t enabledLayerCount;
        const char* const* ppEnabledLayerNames;
        uint32_t enabledExtensionCount;
        const char* const* ppEnabledExtensionNames;
        const VkPhysicalDeviceFeatures* pEnabledFeatures;
    };

    struct VkWin32SurfaceCreateInfoKHR
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        HINSTANCE hinstance;
        HWND hwnd;
    };

    struct VkSurfaceCapabilitiesKHR
    {
        uint32_t minImageCount;
        uint32_t maxImageCount;
        VkExtent2D currentExtent;
        VkExtent2D minImageExtent;
        VkExtent2D maxImageExtent;
        uint32_t maxImageArrayLayers;
        VkFlags supportedTransforms;
        VkFlags currentTransform;
        VkFlags supportedCompositeAlpha;
        VkFlags supportedUsageFlags;
    };

    struct VkSurfaceFormatKHR
    {
        VkFormat format;
        int32_t colorSpace;
    };

    struct VkSwapchainCreateInfoKHR
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkSurfaceKHR surface;
        uint32_t minImageCount;
        VkFormat imageFormat;
        int32_t imageColorSpace;
        VkExtent2D imageExtent;
        uint32_t imageArrayLayers;
        VkFlags imageUsage;
        int32_t imageSharingMode;
        uint32_t queueFamilyIndexCount;
        const uint32_t* pQueueFamilyIndices;
        VkFlags preTransform;
        VkFlags compositeAlpha;
        VkPresentModeKHR presentMode;
        VkBool32 clipped;
        VkSwapchainKHR oldSwapchain;
    };

    struct VkPresentInfoKHR
    {
        VkStructureType sType;
        const void* pNext;
        uint32_t waitSemaphoreCount;
        const VkSemaphore* pWaitSemaphores;
        uint32_t swapchainCount;
        const VkSwapchainKHR* pSwapchains;
        const uint32_t* pImageIndices;
        VkResult* pResults;
    };

    struct VkMemoryRequirements
    {
        VkDeviceSize size;
        VkDeviceSize alignment;
        uint32_t memoryTypeBits;
    };

    struct VkMemoryAllocateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkDeviceSize allocationSize;
        uint32_t memoryTypeIndex;
    };

    struct VkBufferCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkDeviceSize size;
        VkFlags usage;
        int32_t sharingMode;
        uint32_t queueFamilyIndexCount;
        const uint32_t* pQueueFamilyIndices;
    };

    struct VkImageCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        int32_t imageType;
        VkFormat format;
        VkExtent3D extent;
        uint32_t mipLevels;
        uint32_t arrayLayers;
        int32_t samples;
        int32_t tiling;
        VkFlags usage;
        int32_t sharingMode;
        uint32_t queueFamilyIndexCount;
        const uint32_t* pQueueFamilyIndices;
        VkImageLayout initialLayout;
    };

    struct VkImageSubresourceRange
    {
        VkFlags aspectMask;
        uint32_t baseMipLevel;
        uint32_t levelCount;
        uint32_t baseArrayLayer;
        uint32_t layerCount;
    };

    struct VkImageSubresourceLayers
    {
        VkFlags aspectMask;
        uint32_t mipLevel;
        uint32_t baseArrayLayer;
        uint32_t layerCount;
    };

    struct VkComponentMapping
    {
        int32_t r;
        int32_t g;
        int32_t b;
        int32_t a;
    };

    struct VkImageViewCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkImage image;
        int32_t viewType;
        VkFormat format;
        VkComponentMapping components;
        VkImageSubresourceRange subresourceRange;
    };

    struct VkSamplerCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkFilter magFilter;
        VkFilter minFilter;
        VkSamplerMipmapMode mipmapMode;
        VkSamplerAddressMode addressModeU;
        VkSamplerAddressMode addressModeV;
        VkSamplerAddressMode addressModeW;
        float mipLodBias;
        VkBool32 anisotropyEnable;
        float maxAnisotropy;
        VkBool32 compareEnable;
        VkCompareOp compareOp;
        float minLod;
        float maxLod;
        int32_t borderColor;
        VkBool32 unnormalizedCoordinates;
    };

    struct VkAttachmentDescription
    {
        VkFlags flags;
        VkFormat format;
        int32_t samples;
        int32_t loadOp;
        int32_t storeOp;
        int32_t stencilLoadOp;
        int32_t stencilStoreOp;
        VkImageLayout initialLayout;
        VkImageLayout finalLayout;
    };

    struct VkAttachmentReference
    {
        uint32_t attachment;
        VkImageLayout layout;
    };

    struct VkSubpassDescription
    {
        VkFlags flags;
        int32_t pipelineBindPoint;
        uint32_t inputAttachmentCount;
        const VkAttachmentReference* pInputAttachments;
        uint32_t colorAttachmentCount;
        const VkAttachmentReference* pColorAttachments;
        const VkAttachmentReference* pResolveAttachments;
        const VkAttachmentReference* pDepthStencilAttachment;
        uint32_t preserveAttachmentCount;
        const uint32_t* pPreserveAttachments;
    };

    struct VkSubpassDependency
    {
        uint32_t srcSubpass;
        uint32_t dstSubpass;
        VkFlags srcStageMask;
        VkFlags dstStageMask;
        VkFlags srcAccessMask;
        VkFlags dstAccessMask;
        VkFlags dependencyFlags;
    };

    struct VkRenderPassCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t attachmentCount;
        const VkAttachmentDescription* pAttachments;
        uint32_t subpassCount;
        const VkSubpassDescription* pSubpasses;
        uint32_t dependencyCount;
        const VkSubpassDependency* pDependencies;
    };

    struct VkFramebufferCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkRenderPass renderPass;
        uint32_t attachmentCount;
        const VkImageView* pAttachments;
        uint32_t width;
        uint32_t height;
        uint32_t layers;
    };

    struct VkShaderModuleCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        size_t codeSize;
        const uint32_t* pCode;
    };

    struct VkPipelineCacheCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        size_t initialDataSize;
        const void* pInitialData;
    };

    struct VkDescriptorSetLayoutBinding
    {
        uint32_t binding;
        int32_t descriptorType;
        uint32_t descriptorCount;
        VkFlags stageFlags;
        const VkSampler* pImmutableSamplers;
    };

    struct VkDescriptorSetLayoutCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t bindingCount;
        const VkDescriptorSetLayoutBinding* pBindings;
    };

    struct VkPushConstantRange
    {
        VkFlags stageFlags;
        uint32_t offset;
        uint32_t size;
    };

    struct VkPipelineLayoutCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t setLayoutCount;
        const VkDescriptorSetLayout* pSetLayouts;
        uint32_t pushConstantRangeCount;
        const VkPushConstantRange* pPushConstantRanges;
    };

    struct VkDescriptorPoolSize
    {
        int32_t type;
        uint32_t descriptorCount;
    };

    struct VkDescriptorPoolCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t maxSets;
        uint32_t poolSizeCount;
        const VkDescriptorPoolSize* pPoolSizes;
    };

    struct VkDescriptorSetAllocateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkDescriptorPool descriptorPool;
        uint32_t descriptorSetCount;
        const VkDescriptorSetLayout* pSetLayouts;
    };

    struct VkDescriptorImageInfo
    {
        VkSampler sampler;
        VkImageView imageView;
        VkImageLayout imageLayout;
    };

    struct VkWriteDescriptorSet
    {
        VkStructureType sType;
        const void* pNext;
        VkDescriptorSet dstSet;
        uint32_t dstBinding;
        uint32_t dstArrayElement;
        uint32_t descriptorCount;
        int32_t descriptorType;
        const VkDescriptorImageInfo* pImageInfo;
        const void* pBufferInfo;
        const void* pTexelBufferView;
    };

    struct VkPipelineShaderStageCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkFlags stage;
        VkShaderModule module;
        const char* pName;
        const void* pSpecializationInfo;
    };

    struct VkVertexInputBindingDescription
    {
        uint32_t binding;
        uint32_t stride;
        int32_t inputRate;
    };

    struct VkVertexInputAttributeDescription
    {
        uint32_t location;
        uint32_t binding;
        VkFormat format;
        uint32_t offset;
    };

    struct VkPipelineVertexInputStateCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t vertexBindingDescriptionCount;
        const VkVertexInputBindingDescription* pVertexBindingDescriptions;
        uint32_t vertexAttributeDescriptionCount;
        const VkVertexInputAttributeDescription* pVertexAttributeDescriptions;
    };

    struct VkPipelineInputAssemblyStateCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkPrimitiveTopology topology;
        VkBool32 primitiveRestartEnable;
    };

    struct VkPipelineViewportStateCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t viewportCount;
        const VkViewport* pViewports;
        uint32_t scissorCount;
        const VkRect2D* pScissors;
    };

    struct VkPipelineRasterizationStateCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkBool32 depthClampEnable;
        VkBool32 rasterizerDiscardEnable;
        VkPolygonMode polygonMode;
        VkFlags cullMode;
        VkFrontFace frontFace;
        VkBool32 depthBiasEnable;
        float depthBiasConstantFactor;
        float depthBiasClamp;
        float depthBiasSlopeFactor;
        float lineWidth;
    };

    struct VkPipelineMultisampleStateCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        int32_t rasterizationSamples;
        VkBool32 sampleShadingEnable;
        float minSampleShading;
        const VkSampleMask* pSampleMask;
        VkBool32 alphaToCoverageEnable;
        VkBool32 alphaToOneEnable;
    };

    struct VkStencilOpState
    {
        VkStencilOp failOp;
        VkStencilOp passOp;
        VkStencilOp depthFailOp;
        VkCompareOp compareOp;
        uint32_t compareMask;
        uint32_t writeMask;
        uint32_t reference;
    };

    struct VkPipelineDepthStencilStateCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkBool32 depthTestEnable;
        VkBool32 depthWriteEnable;
        VkCompareOp depthCompareOp;
        VkBool32 depthBoundsTestEnable;
        VkBool32 stencilTestEnable;
        VkStencilOpState front;
        VkStencilOpState back;
        float minDepthBounds;
        float maxDepthBounds;
    };

    struct VkPipelineColorBlendAttachmentState
    {
        VkBool32 blendEnable;
        VkBlendFactor srcColorBlendFactor;
        VkBlendFactor dstColorBlendFactor;
        VkBlendOp colorBlendOp;
        VkBlendFactor srcAlphaBlendFactor;
        VkBlendFactor dstAlphaBlendFactor;
        VkBlendOp alphaBlendOp;
        VkFlags colorWriteMask;
    };

    struct VkPipelineColorBlendStateCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        VkBool32 logicOpEnable;
        int32_t logicOp;
        uint32_t attachmentCount;
        const VkPipelineColorBlendAttachmentState* pAttachments;
        float blendConstants[4];
    };

    struct VkPipelineDynamicStateCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t dynamicStateCount;
        const VkDynamicState* pDynamicStates;
    };

    struct VkGraphicsPipelineCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t stageCount;
        const VkPipelineShaderStageCreateInfo* pStages;
        const VkPipelineVertexInputStateCreateInfo* pVertexInputState;
        const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState;
        const void* pTessellationState;
        const VkPipelineViewportStateCreateInfo* pViewportState;
        const VkPipelineRasterizationStateCreateInfo* pRasterizationState;
        const VkPipelineMultisampleStateCreateInfo* pMultisampleState;
        const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState;
        const VkPipelineColorBlendStateCreateInfo* pColorBlendState;
        const VkPipelineDynamicStateCreateInfo* pDynamicState;
        VkPipelineLayout layout;
        VkRenderPass renderPass;
        uint32_t subpass;
        VkPipeline basePipelineHandle;
        int32_t basePipelineIndex;
    };

    struct VkCommandPoolCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        uint32_t queueFamilyIndex;
    };

    struct VkCommandBufferAllocateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkCommandPool commandPool;
        int32_t level;
        uint32_t commandBufferCount;
    };

    struct VkCommandBufferBeginInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
        const void* pInheritanceInfo;
    };

    union VkClearColorValue
    {
        float float32[4];
        int32_t int32[4];
        uint32_t uint32[4];
    };

    struct VkClearDepthStencilValue
    {
        float depth;
        uint32_t stencil;
    };

    union VkClearValue
    {
        VkClearColorValue color;
        VkClearDepthStencilValue depthStencil;
    };

    struct VkRenderPassBeginInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkRenderPass renderPass;
        VkFramebuffer framebuffer;
        VkRect2D renderArea;
        uint32_t clearValueCount;
        const VkClearValue* pClearValues;
    };

    struct VkClearAttachment
    {
        VkFlags aspectMask;
        uint32_t colorAttachment;
        VkClearValue clearValue;
    };

    struct VkClearRect
    {
        VkRect2D rect;
        uint32_t baseArrayLayer;
        uint32_t layerCount;
    };

    struct VkImageMemoryBarrier
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags srcAccessMask;
        VkFlags dstAccessMask;
        VkImageLayout oldLayout;
        VkImageLayout newLayout;
        uint32_t srcQueueFamilyIndex;
        uint32_t dstQueueFamilyIndex;
        VkImage image;
        VkImageSubresourceRange subresourceRange;
    };

    struct VkBufferImageCopy
    {
        VkDeviceSize bufferOffset;
        uint32_t bufferRowLength;
        uint32_t bufferImageHeight;
        VkImageSubresourceLayers imageSubresource;
        VkOffset3D imageOffset;
        VkExtent3D imageExtent;
    };

    struct VkImageBlit
    {
        VkImageSubresourceLayers srcSubresource;
        VkOffset3D srcOffsets[2];
        VkImageSubresourceLayers dstSubresource;
        VkOffset3D dstOffsets[2];
    };

    struct VkFenceCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
    };

    struct VkSemaphoreCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        VkFlags flags;
    };

    struct VkSubmitInfo
    {
        VkStructureType sType;
        const void* pNext;
        uint32_t waitSemaphoreCount;
        const VkSemaphore* pWaitSemaphores;
        const VkFlags* pWaitDstStageMask;
        uint32_t commandBufferCount;
        const VkCommandBuffer* pCommandBuffers;
        uint32_t signalSemaphoreCount;
        const VkSemaphore* pSignalSemaphores;
    };

    using PFN_vkVoidFunction = void(QLVK_PTR*)();
    using PFN_vkGetInstanceProcAddr = PFN_vkVoidFunction(QLVK_PTR*)(VkInstance, const char*);
    using PFN_vkGetDeviceProcAddr = PFN_vkVoidFunction(QLVK_PTR*)(VkDevice, const char*);

    using PFN_vkCreateInstance = VkResult(QLVK_PTR*)(const VkInstanceCreateInfo*, const void*, VkInstance*);
    using PFN_vkDestroyInstance = void(QLVK_PTR*)(VkInstance, const void*);
    using PFN_vkEnumeratePhysicalDevices = VkResult(QLVK_PTR*)(VkInstance, uint32_t*, VkPhysicalDevice*);
    using PFN_vkGetPhysicalDeviceProperties = void(QLVK_PTR*)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
    using PFN_vkGetPhysicalDeviceFeatures = void(QLVK_PTR*)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
    using PFN_vkGetPhysicalDeviceQueueFamilyProperties = void(QLVK_PTR*)(VkPhysicalDevice, uint32_t*, VkQueueFamilyProperties*);
    using PFN_vkGetPhysicalDeviceMemoryProperties = void(QLVK_PTR*)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*);
    using PFN_vkGetPhysicalDeviceFormatProperties = void(QLVK_PTR*)(VkPhysicalDevice, VkFormat, VkFormatProperties*);
    using PFN_vkCreateDevice = VkResult(QLVK_PTR*)(VkPhysicalDevice, const VkDeviceCreateInfo*, const void*, VkDevice*);
    using PFN_vkDestroyDevice = void(QLVK_PTR*)(VkDevice, const void*);

    using PFN_vkCreateWin32SurfaceKHR = VkResult(QLVK_PTR*)(VkInstance, const VkWin32SurfaceCreateInfoKHR*, const void*, VkSurfaceKHR*);
    using PFN_vkDestroySurfaceKHR = void(QLVK_PTR*)(VkInstance, VkSurfaceKHR, const void*);
    using PFN_vkGetPhysicalDeviceSurfaceSupportKHR = VkResult(QLVK_PTR*)(VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32*);
    using PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = VkResult(QLVK_PTR*)(VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR*);
    using PFN_vkGetPhysicalDeviceSurfaceFormatsKHR = VkResult(QLVK_PTR*)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkSurfaceFormatKHR*);
    using PFN_vkGetPhysicalDeviceSurfacePresentModesKHR = VkResult(QLVK_PTR*)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkPresentModeKHR*);

    using PFN_vkGetDeviceQueue = void(QLVK_PTR*)(VkDevice, uint32_t, uint32_t, VkQueue*);
    using PFN_vkDeviceWaitIdle = VkResult(QLVK_PTR*)(VkDevice);
    using PFN_vkQueueSubmit = VkResult(QLVK_PTR*)(VkQueue, uint32_t, const VkSubmitInfo*, VkFence);

    using PFN_vkCreateSwapchainKHR = VkResult(QLVK_PTR*)(VkDevice, const VkSwapchainCreateInfoKHR*, const void*, VkSwapchainKHR*);
    using PFN_vkDestroySwapchainKHR = void(QLVK_PTR*)(VkDevice, VkSwapchainKHR, const void*);
    using PFN_vkGetSwapchainImagesKHR = VkResult(QLVK_PTR*)(VkDevice, VkSwapchainKHR, uint32_t*, VkImage*);
    using PFN_vkAcquireNextImageKHR = VkResult(QLVK_PTR*)(VkDevice, VkSwapchainKHR, uint64_t, VkSemaphore, VkFence, uint32_t*);
    using PFN_vkQueuePresentKHR = VkResult(QLVK_PTR*)(VkQueue, const VkPresentInfoKHR*);

    using PFN_vkAllocateMemory = VkResult(QLVK_PTR*)(VkDevice, const VkMemoryAllocateInfo*, const void*, VkDeviceMemory*);
    using PFN_vkFreeMemory = void(QLVK_PTR*)(VkDevice, VkDeviceMemory, const void*);
    using PFN_vkMapMemory = VkResult(QLVK_PTR*)(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkFlags, void**);

    using PFN_vkCreateBuffer = VkResult(QLVK_PTR*)(VkDevice, const VkBufferCreateInfo*, const void*, VkBuffer*);
    using PFN_vkDestroyBuffer = void(QLVK_PTR*)(VkDevice, VkBuffer, const void*);
    using PFN_vkGetBufferMemoryRequirements = void(QLVK_PTR*)(VkDevice, VkBuffer, VkMemoryRequirements*);
    using PFN_vkBindBufferMemory = VkResult(QLVK_PTR*)(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize);

    using PFN_vkCreateImage = VkResult(QLVK_PTR*)(VkDevice, const VkImageCreateInfo*, const void*, VkImage*);
    using PFN_vkDestroyImage = void(QLVK_PTR*)(VkDevice, VkImage, const void*);
    using PFN_vkGetImageMemoryRequirements = void(QLVK_PTR*)(VkDevice, VkImage, VkMemoryRequirements*);
    using PFN_vkBindImageMemory = VkResult(QLVK_PTR*)(VkDevice, VkImage, VkDeviceMemory, VkDeviceSize);

    using PFN_vkCreateImageView = VkResult(QLVK_PTR*)(VkDevice, const VkImageViewCreateInfo*, const void*, VkImageView*);
    using PFN_vkDestroyImageView = void(QLVK_PTR*)(VkDevice, VkImageView, const void*);

    using PFN_vkCreateSampler = VkResult(QLVK_PTR*)(VkDevice, const VkSamplerCreateInfo*, const void*, VkSampler*);
    using PFN_vkDestroySampler = void(QLVK_PTR*)(VkDevice, VkSampler, const void*);

    using PFN_vkCreateShaderModule = VkResult(QLVK_PTR*)(VkDevice, const VkShaderModuleCreateInfo*, const void*, VkShaderModule*);
    using PFN_vkDestroyShaderModule = void(QLVK_PTR*)(VkDevice, VkShaderModule, const void*);

    using PFN_vkCreateRenderPass = VkResult(QLVK_PTR*)(VkDevice, const VkRenderPassCreateInfo*, const void*, VkRenderPass*);
    using PFN_vkDestroyRenderPass = void(QLVK_PTR*)(VkDevice, VkRenderPass, const void*);

    using PFN_vkCreateFramebuffer = VkResult(QLVK_PTR*)(VkDevice, const VkFramebufferCreateInfo*, const void*, VkFramebuffer*);
    using PFN_vkDestroyFramebuffer = void(QLVK_PTR*)(VkDevice, VkFramebuffer, const void*);

    using PFN_vkCreateDescriptorSetLayout = VkResult(QLVK_PTR*)(VkDevice, const VkDescriptorSetLayoutCreateInfo*, const void*, VkDescriptorSetLayout*);
    using PFN_vkDestroyDescriptorSetLayout = void(QLVK_PTR*)(VkDevice, VkDescriptorSetLayout, const void*);

    using PFN_vkCreatePipelineLayout = VkResult(QLVK_PTR*)(VkDevice, const VkPipelineLayoutCreateInfo*, const void*, VkPipelineLayout*);
    using PFN_vkDestroyPipelineLayout = void(QLVK_PTR*)(VkDevice, VkPipelineLayout, const void*);

    using PFN_vkCreatePipelineCache = VkResult(QLVK_PTR*)(VkDevice, const VkPipelineCacheCreateInfo*, const void*, VkPipelineCache*);
    using PFN_vkDestroyPipelineCache = void(QLVK_PTR*)(VkDevice, VkPipelineCache, const void*);
    using PFN_vkGetPipelineCacheData = VkResult(QLVK_PTR*)(VkDevice, VkPipelineCache, size_t*, void*);

    using PFN_vkCreateGraphicsPipelines = VkResult(QLVK_PTR*)(VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo*, const void*, VkPipeline*);
    using PFN_vkDestroyPipeline = void(QLVK_PTR*)(VkDevice, VkPipeline, const void*);

    using PFN_vkCreateDescriptorPool = VkResult(QLVK_PTR*)(VkDevice, const VkDescriptorPoolCreateInfo*, const void*, VkDescriptorPool*);
    using PFN_vkDestroyDescriptorPool = void(QLVK_PTR*)(VkDevice, VkDescriptorPool, const void*);
    using PFN_vkResetDescriptorPool = VkResult(QLVK_PTR*)(VkDevice, VkDescriptorPool, VkFlags);
    using PFN_vkAllocateDescriptorSets = VkResult(QLVK_PTR*)(VkDevice, const VkDescriptorSetAllocateInfo*, VkDescriptorSet*);
    using PFN_vkUpdateDescriptorSets = void(QLVK_PTR*)(VkDevice, uint32_t, const VkWriteDescriptorSet*, uint32_t, const void*);

    using PFN_vkCreateCommandPool = VkResult(QLVK_PTR*)(VkDevice, const VkCommandPoolCreateInfo*, const void*, VkCommandPool*);
    using PFN_vkDestroyCommandPool = void(QLVK_PTR*)(VkDevice, VkCommandPool, const void*);
    using PFN_vkAllocateCommandBuffers = VkResult(QLVK_PTR*)(VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer*);
    using PFN_vkBeginCommandBuffer = VkResult(QLVK_PTR*)(VkCommandBuffer, const VkCommandBufferBeginInfo*);
    using PFN_vkEndCommandBuffer = VkResult(QLVK_PTR*)(VkCommandBuffer);
    using PFN_vkResetCommandBuffer = VkResult(QLVK_PTR*)(VkCommandBuffer, VkFlags);

    using PFN_vkCreateFence = VkResult(QLVK_PTR*)(VkDevice, const VkFenceCreateInfo*, const void*, VkFence*);
    using PFN_vkDestroyFence = void(QLVK_PTR*)(VkDevice, VkFence, const void*);
    using PFN_vkWaitForFences = VkResult(QLVK_PTR*)(VkDevice, uint32_t, const VkFence*, VkBool32, uint64_t);
    using PFN_vkResetFences = VkResult(QLVK_PTR*)(VkDevice, uint32_t, const VkFence*);

    using PFN_vkCreateSemaphore = VkResult(QLVK_PTR*)(VkDevice, const VkSemaphoreCreateInfo*, const void*, VkSemaphore*);
    using PFN_vkDestroySemaphore = void(QLVK_PTR*)(VkDevice, VkSemaphore, const void*);

    using PFN_vkCmdBeginRenderPass = void(QLVK_PTR*)(VkCommandBuffer, const VkRenderPassBeginInfo*, int32_t);
    using PFN_vkCmdEndRenderPass = void(QLVK_PTR*)(VkCommandBuffer);
    using PFN_vkCmdBindPipeline = void(QLVK_PTR*)(VkCommandBuffer, int32_t, VkPipeline);
    using PFN_vkCmdSetViewport = void(QLVK_PTR*)(VkCommandBuffer, uint32_t, uint32_t, const VkViewport*);
    using PFN_vkCmdSetScissor = void(QLVK_PTR*)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D*);
    using PFN_vkCmdSetDepthBias = void(QLVK_PTR*)(VkCommandBuffer, float, float, float);
    using PFN_vkCmdSetStencilCompareMask = void(QLVK_PTR*)(VkCommandBuffer, VkFlags, uint32_t);
    using PFN_vkCmdSetStencilWriteMask = void(QLVK_PTR*)(VkCommandBuffer, VkFlags, uint32_t);
    using PFN_vkCmdSetStencilReference = void(QLVK_PTR*)(VkCommandBuffer, VkFlags, uint32_t);
    using PFN_vkCmdBindDescriptorSets = void(QLVK_PTR*)(VkCommandBuffer, int32_t, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet*, uint32_t, const uint32_t*);
    using PFN_vkCmdBindVertexBuffers = void(QLVK_PTR*)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer*, const VkDeviceSize*);
    using PFN_vkCmdBindIndexBuffer = void(QLVK_PTR*)(VkCommandBuffer, VkBuffer, VkDeviceSize, int32_t);
    using PFN_vkCmdPushConstants = void(QLVK_PTR*)(VkCommandBuffer, VkPipelineLayout, VkFlags, uint32_t, uint32_t, const void*);
    using PFN_vkCmdDrawIndexed = void(QLVK_PTR*)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, int32_t, uint32_t);
    using PFN_vkCmdClearAttachments = void(QLVK_PTR*)(VkCommandBuffer, uint32_t, const VkClearAttachment*, uint32_t, const VkClearRect*);
    using PFN_vkCmdClearColorImage = void(QLVK_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue*, uint32_t, const VkImageSubresourceRange*);
    using PFN_vkCmdClearDepthStencilImage = void(QLVK_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearDepthStencilValue*, uint32_t, const VkImageSubresourceRange*);
    using PFN_vkCmdPipelineBarrier = void(QLVK_PTR*)(VkCommandBuffer, VkFlags, VkFlags, VkFlags, uint32_t, const void*, uint32_t, const void*, uint32_t, const VkImageMemoryBarrier*);
    using PFN_vkCmdCopyBufferToImage = void(QLVK_PTR*)(VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy*);
    using PFN_vkCmdCopyImageToBuffer = void(QLVK_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy*);
    using PFN_vkCmdBlitImage = void(QLVK_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit*, VkFilter);

    #undef QLVK_PTR

    struct vulkan_dispatch
    {
        HMODULE h_module{nullptr};
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{nullptr};
        PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr{nullptr};

        // global and instance functions
        PFN_vkCreateInstance vkCreateInstance{nullptr};
        PFN_vkDestroyInstance vkDestroyInstance{nullptr};
        PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices{nullptr};
        PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties{nullptr};
        PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures{nullptr};
        PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties{nullptr};
        PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties{nullptr};
        PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties{nullptr};
        PFN_vkCreateDevice vkCreateDevice{nullptr};
        PFN_vkDestroyDevice vkDestroyDevice{nullptr};
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR{nullptr};
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR{nullptr};
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR{nullptr};
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR{nullptr};
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR{nullptr};
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR{nullptr};

        // device functions
        PFN_vkGetDeviceQueue vkGetDeviceQueue{nullptr};
        PFN_vkDeviceWaitIdle vkDeviceWaitIdle{nullptr};
        PFN_vkQueueSubmit vkQueueSubmit{nullptr};
        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR{nullptr};
        PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR{nullptr};
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR{nullptr};
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR{nullptr};
        PFN_vkQueuePresentKHR vkQueuePresentKHR{nullptr};
        PFN_vkAllocateMemory vkAllocateMemory{nullptr};
        PFN_vkFreeMemory vkFreeMemory{nullptr};
        PFN_vkMapMemory vkMapMemory{nullptr};
        PFN_vkCreateBuffer vkCreateBuffer{nullptr};
        PFN_vkDestroyBuffer vkDestroyBuffer{nullptr};
        PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements{nullptr};
        PFN_vkBindBufferMemory vkBindBufferMemory{nullptr};
        PFN_vkCreateImage vkCreateImage{nullptr};
        PFN_vkDestroyImage vkDestroyImage{nullptr};
        PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements{nullptr};
        PFN_vkBindImageMemory vkBindImageMemory{nullptr};
        PFN_vkCreateImageView vkCreateImageView{nullptr};
        PFN_vkDestroyImageView vkDestroyImageView{nullptr};
        PFN_vkCreateSampler vkCreateSampler{nullptr};
        PFN_vkDestroySampler vkDestroySampler{nullptr};
        PFN_vkCreateShaderModule vkCreateShaderModule{nullptr};
        PFN_vkDestroyShaderModule vkDestroyShaderModule{nullptr};
        PFN_vkCreateRenderPass vkCreateRenderPass{nullptr};
        PFN_vkDestroyRenderPass vkDestroyRenderPass{nullptr};
        PFN_vkCreateFramebuffer vkCreateFramebuffer{nullptr};
        PFN_vkDestroyFramebuffer vkDestroyFramebuffer{nullptr};
        PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout{nullptr};
        PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout{nullptr};
        PFN_vkCreatePipelineLayout vkCreatePipelineLayout{nullptr};
        PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout{nullptr};
        PFN_vkCreatePipelineCache vkCreatePipelineCache{nullptr};
        PFN_vkDestroyPipelineCache vkDestroyPipelineCache{nullptr};
        PFN_vkGetPipelineCacheData vkGetPipelineCacheData{nullptr};
        PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines{nullptr};
        PFN_vkDestroyPipeline vkDestroyPipeline{nullptr};
        PFN_vkCreateDescriptorPool vkCreateDescriptorPool{nullptr};
        PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool{nullptr};
        PFN_vkResetDescriptorPool vkResetDescriptorPool{nullptr};
        PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets{nullptr};
        PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets{nullptr};
        PFN_vkCreateCommandPool vkCreateCommandPool{nullptr};
        PFN_vkDestroyCommandPool vkDestroyCommandPool{nullptr};
        PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers{nullptr};
        PFN_vkBeginCommandBuffer vkBeginCommandBuffer{nullptr};
        PFN_vkEndCommandBuffer vkEndCommandBuffer{nullptr};
        PFN_vkResetCommandBuffer vkResetCommandBuffer{nullptr};
        PFN_vkCreateFence vkCreateFence{nullptr};
        PFN_vkDestroyFence vkDestroyFence{nullptr};
        PFN_vkWaitForFences vkWaitForFences{nullptr};
        PFN_vkResetFences vkResetFences{nullptr};
        PFN_vkCreateSemaphore vkCreateSemaphore{nullptr};
        PFN_vkDestroySemaphore vkDestroySemaphore{nullptr};
        PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass{nullptr};
        PFN_vkCmdEndRenderPass vkCmdEndRenderPass{nullptr};
        PFN_vkCmdBindPipeline vkCmdBindPipeline{nullptr};
        PFN_vkCmdSetViewport vkCmdSetViewport{nullptr};
        PFN_vkCmdSetScissor vkCmdSetScissor{nullptr};
        PFN_vkCmdSetDepthBias vkCmdSetDepthBias{nullptr};
        PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask{nullptr};
        PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask{nullptr};
        PFN_vkCmdSetStencilReference vkCmdSetStencilReference{nullptr};
        PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets{nullptr};
        PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers{nullptr};
        PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer{nullptr};
        PFN_vkCmdPushConstants vkCmdPushConstants{nullptr};
        PFN_vkCmdDrawIndexed vkCmdDrawIndexed{nullptr};
        PFN_vkCmdClearAttachments vkCmdClearAttachments{nullptr};
        PFN_vkCmdClearColorImage vkCmdClearColorImage{nullptr};
        PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage{nullptr};
        PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier{nullptr};
        PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage{nullptr};
        PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer{nullptr};
        PFN_vkCmdBlitImage vkCmdBlitImage{nullptr};

        bool load()
        {
            if (h_module != nullptr)
            {
                return true;
            }
            h_module = LoadLibraryW(L"vulkan-1.dll");
            if (h_module == nullptr)
            {
                return false;
            }
            vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                GetProcAddress(h_module, "vkGetInstanceProcAddr"));
            if (vkGetInstanceProcAddr == nullptr)
            {
                FreeLibrary(h_module);
                h_module = nullptr;
                return false;
            }
            vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
                vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
            return vkCreateInstance != nullptr;
        }

        bool load_instance(VkInstance inst)
        {
            #define QLVK_INST(fn)                                                    \
                fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(inst, #fn));   \
                if (fn == nullptr)                                                   \
                {                                                                    \
                    return false;                                                    \
                }
            QLVK_INST(vkDestroyInstance);
            QLVK_INST(vkEnumeratePhysicalDevices);
            QLVK_INST(vkGetPhysicalDeviceProperties);
            QLVK_INST(vkGetPhysicalDeviceFeatures);
            QLVK_INST(vkGetPhysicalDeviceQueueFamilyProperties);
            QLVK_INST(vkGetPhysicalDeviceMemoryProperties);
            QLVK_INST(vkGetPhysicalDeviceFormatProperties);
            QLVK_INST(vkCreateDevice);
            QLVK_INST(vkGetDeviceProcAddr);
            QLVK_INST(vkCreateWin32SurfaceKHR);
            QLVK_INST(vkDestroySurfaceKHR);
            QLVK_INST(vkGetPhysicalDeviceSurfaceSupportKHR);
            QLVK_INST(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
            QLVK_INST(vkGetPhysicalDeviceSurfaceFormatsKHR);
            QLVK_INST(vkGetPhysicalDeviceSurfacePresentModesKHR);
            #undef QLVK_INST
            return true;
        }

        bool load_device(VkDevice dev)
        {
            #define QLVK_DEV(fn)                                                   \
                fn = reinterpret_cast<PFN_##fn>(vkGetDeviceProcAddr(dev, #fn));    \
                if (fn == nullptr)                                                 \
                {                                                                  \
                    return false;                                                  \
                }
            QLVK_DEV(vkGetDeviceQueue);
            QLVK_DEV(vkDestroyDevice);
            QLVK_DEV(vkDeviceWaitIdle);
            QLVK_DEV(vkQueueSubmit);
            QLVK_DEV(vkCreateSwapchainKHR);
            QLVK_DEV(vkDestroySwapchainKHR);
            QLVK_DEV(vkGetSwapchainImagesKHR);
            QLVK_DEV(vkAcquireNextImageKHR);
            QLVK_DEV(vkQueuePresentKHR);
            QLVK_DEV(vkAllocateMemory);
            QLVK_DEV(vkFreeMemory);
            QLVK_DEV(vkMapMemory);
            QLVK_DEV(vkCreateBuffer);
            QLVK_DEV(vkDestroyBuffer);
            QLVK_DEV(vkGetBufferMemoryRequirements);
            QLVK_DEV(vkBindBufferMemory);
            QLVK_DEV(vkCreateImage);
            QLVK_DEV(vkDestroyImage);
            QLVK_DEV(vkGetImageMemoryRequirements);
            QLVK_DEV(vkBindImageMemory);
            QLVK_DEV(vkCreateImageView);
            QLVK_DEV(vkDestroyImageView);
            QLVK_DEV(vkCreateSampler);
            QLVK_DEV(vkDestroySampler);
            QLVK_DEV(vkCreateShaderModule);
            QLVK_DEV(vkDestroyShaderModule);
            QLVK_DEV(vkCreateRenderPass);
            QLVK_DEV(vkDestroyRenderPass);
            QLVK_DEV(vkCreateFramebuffer);
            QLVK_DEV(vkDestroyFramebuffer);
            QLVK_DEV(vkCreateDescriptorSetLayout);
            QLVK_DEV(vkDestroyDescriptorSetLayout);
            QLVK_DEV(vkCreatePipelineLayout);
            QLVK_DEV(vkDestroyPipelineLayout);
            QLVK_DEV(vkCreateGraphicsPipelines);
            QLVK_DEV(vkDestroyPipeline);
            QLVK_DEV(vkCreateDescriptorPool);
            QLVK_DEV(vkDestroyDescriptorPool);
            QLVK_DEV(vkResetDescriptorPool);
            QLVK_DEV(vkAllocateDescriptorSets);
            QLVK_DEV(vkUpdateDescriptorSets);
            QLVK_DEV(vkCreateCommandPool);
            QLVK_DEV(vkDestroyCommandPool);
            QLVK_DEV(vkAllocateCommandBuffers);
            QLVK_DEV(vkBeginCommandBuffer);
            QLVK_DEV(vkEndCommandBuffer);
            QLVK_DEV(vkResetCommandBuffer);
            QLVK_DEV(vkCreateFence);
            QLVK_DEV(vkDestroyFence);
            QLVK_DEV(vkWaitForFences);
            QLVK_DEV(vkResetFences);
            QLVK_DEV(vkCreateSemaphore);
            QLVK_DEV(vkDestroySemaphore);
            QLVK_DEV(vkCmdBeginRenderPass);
            QLVK_DEV(vkCmdEndRenderPass);
            QLVK_DEV(vkCmdBindPipeline);
            QLVK_DEV(vkCmdSetViewport);
            QLVK_DEV(vkCmdSetScissor);
            QLVK_DEV(vkCmdSetDepthBias);
            QLVK_DEV(vkCmdSetStencilCompareMask);
            QLVK_DEV(vkCmdSetStencilWriteMask);
            QLVK_DEV(vkCmdSetStencilReference);
            QLVK_DEV(vkCmdBindDescriptorSets);
            QLVK_DEV(vkCmdBindVertexBuffers);
            QLVK_DEV(vkCmdBindIndexBuffer);
            QLVK_DEV(vkCmdPushConstants);
            QLVK_DEV(vkCmdDrawIndexed);
            QLVK_DEV(vkCmdClearAttachments);
            QLVK_DEV(vkCmdClearColorImage);
            QLVK_DEV(vkCmdClearDepthStencilImage);
            QLVK_DEV(vkCmdPipelineBarrier);
            QLVK_DEV(vkCmdCopyBufferToImage);
            QLVK_DEV(vkCmdCopyImageToBuffer);
            QLVK_DEV(vkCmdBlitImage);
            QLVK_DEV(vkCreatePipelineCache);
            QLVK_DEV(vkDestroyPipelineCache);
            QLVK_DEV(vkGetPipelineCacheData);
            #undef QLVK_DEV
            return true;
        }
    };
}
