#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "MeshTypes.hpp"
#include <vulkan/vulkan.h>
#include <memory>

namespace mc::client {

// 前向声明
class VulkanBuffer;

// 纹理配置
struct TextureConfig {
    u32 width = 0;
    u32 height = 0;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    u32 mipLevels = 1;
    u32 arrayLayers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
};

// 纹理图像
class VulkanTexture {
public:
    VulkanTexture();
    ~VulkanTexture();

    // 禁止拷贝
    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    // 允许移动
    VulkanTexture(VulkanTexture&& other) noexcept;
    VulkanTexture& operator=(VulkanTexture&& other) noexcept;

    // 创建纹理
    [[nodiscard]] Result<void> create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        const TextureConfig& config);

    void destroy();

    // 上传数据
    [[nodiscard]] Result<void> upload(
        VkCommandBuffer commandBuffer,
        VulkanBuffer& stagingBuffer,
        const void* data);

    // 生成 mipmaps
    [[nodiscard]] Result<void> generateMipmaps(
        VkCommandBuffer commandBuffer,
        VkPhysicalDevice physicalDevice);

    // 转换布局
    void transitionLayout(
        VkCommandBuffer commandBuffer,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage);

    // 获取器
    VkImage image() const { return m_image; }
    VkImageView imageView() const { return m_imageView; }
    VkSampler sampler() const { return m_sampler; }
    VkFormat format() const { return m_format; }
    VkImageLayout layout() const { return m_layout; }
    u32 width() const { return m_width; }
    u32 height() const { return m_height; }
    u32 mipLevels() const { return m_mipLevels; }
    bool isValid() const { return m_image != VK_NULL_HANDLE; }

    // 创建 sampler (可选)
    [[nodiscard]] Result<void> createSampler(
        VkFilter magFilter = VK_FILTER_LINEAR,
        VkFilter minFilter = VK_FILTER_LINEAR,
        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    // 创建 image view (可选)
    [[nodiscard]] Result<void> createImageView(
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    u32 m_width = 0;
    u32 m_height = 0;
    u32 m_mipLevels = 1;
    u32 m_arrayLayers = 1;

    [[nodiscard]] Result<u32> findMemoryType(
        u32 typeFilter,
        VkMemoryPropertyFlags properties,
        VkPhysicalDevice physicalDevice);
};

// 纹理图集 (用于方块纹理，Vulkan版本)
class VulkanTextureAtlas {
public:
    VulkanTextureAtlas();
    ~VulkanTextureAtlas();

    [[nodiscard]] Result<void> create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        u32 width,
        u32 height);

    void destroy();

    // 上传纹理数据 (数据必须已经复制到 stagingBuffer 中)
    [[nodiscard]] Result<void> upload(
        VkCommandBuffer commandBuffer,
        VulkanBuffer& stagingBuffer);

    // 获取UV坐标
    [[nodiscard]] mc::TextureRegion getRegion(u32 tileX, u32 tileY) const;
    [[nodiscard]] mc::TextureRegion getRegion(u32 tileIndex) const;

    // 获取器
    VulkanTexture& texture() { return m_texture; }
    const VulkanTexture& texture() const { return m_texture; }
    u32 width() const { return m_width; }
    u32 height() const { return m_height; }
    u32 tileSize() const { return m_tileSize; }
    u32 tilesPerRow() const { return m_tilesPerRow; }
    bool isValid() const { return m_texture.isValid(); }

private:
    VulkanTexture m_texture;
    u32 m_width = 0;
    u32 m_height = 0;
    u32 m_tileSize = 16;  // 默认瓦片大小
    u32 m_tilesPerRow = 0;
    f32 m_tileU = 0.0f;
    f32 m_tileV = 0.0f;
};

} // namespace mc::client
