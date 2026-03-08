#include "VulkanTexture.hpp"
#include "VulkanBuffer.hpp"
#include "MeshTypes.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace mr::client {

// ============================================================================
// VulkanTexture 实现
// ============================================================================

VulkanTexture::VulkanTexture() = default;

VulkanTexture::~VulkanTexture() {
    destroy();
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
    : m_device(other.m_device)
    , m_image(other.m_image)
    , m_memory(other.m_memory)
    , m_imageView(other.m_imageView)
    , m_sampler(other.m_sampler)
    , m_format(other.m_format)
    , m_layout(other.m_layout)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_mipLevels(other.m_mipLevels)
    , m_arrayLayers(other.m_arrayLayers)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
}

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_image = other.m_image;
        m_memory = other.m_memory;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_format = other.m_format;
        m_layout = other.m_layout;
        m_width = other.m_width;
        m_height = other.m_height;
        m_mipLevels = other.m_mipLevels;
        m_arrayLayers = other.m_arrayLayers;

        other.m_device = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
    }
    return *this;
}

Result<void> VulkanTexture::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    const TextureConfig& config)
{
    if (m_image != VK_NULL_HANDLE) {
        destroy();
    }

    m_device = device;
    m_width = config.width;
    m_height = config.height;
    m_format = config.format;
    m_mipLevels = config.mipLevels;
    m_arrayLayers = config.arrayLayers;

    // 创建图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers = m_arrayLayers;
    imageInfo.format = m_format;
    imageInfo.tiling = config.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = config.usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = config.samples;
    imageInfo.flags = 0;

    if (vkCreateImage(device, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create image");
    }

    // 获取内存需求
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_image, &memRequirements);

    // 分配内存
    auto memoryTypeResult = findMemoryType(
        memRequirements.memoryTypeBits,
        config.properties,
        physicalDevice);

    if (!memoryTypeResult.success()) {
        vkDestroyImage(device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return memoryTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeResult.value();

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        vkDestroyImage(device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return Error(ErrorCode::InitializationFailed, "Failed to allocate image memory");
    }

    vkBindImageMemory(device, m_image, m_memory, 0);
    m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    return {};
}

void VulkanTexture::destroy() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_device, m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }

        if (m_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }

        if (m_image != VK_NULL_HANDLE) {
            vkDestroyImage(m_device, m_image, nullptr);
            m_image = VK_NULL_HANDLE;
        }

        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }

        m_device = VK_NULL_HANDLE;
    }

    m_width = 0;
    m_height = 0;
    m_mipLevels = 1;
    m_arrayLayers = 1;
    m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

Result<void> VulkanTexture::upload(
    VkCommandBuffer commandBuffer,
    VulkanBuffer& stagingBuffer,
    const void* data)
{
    // 复制数据到暂存缓冲区
    void* mapped = stagingBuffer.map().value();
    if (!mapped) {
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }

    VkDeviceSize imageSize = m_width * m_height * 4; // 假设 RGBA8
    std::memcpy(mapped, data, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    // 转换到传输布局
    transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲区到图像
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_width, m_height, 1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer.buffer(),
        m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    // 转换到着色器只读布局
    transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return {};
}

Result<void> VulkanTexture::generateMipmaps(
    VkCommandBuffer commandBuffer,
    VkPhysicalDevice physicalDevice)
{
    // 检查设备是否支持线性过滤
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, m_format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        return Error(ErrorCode::Unsupported, "Texture format does not support linear blitting");
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = m_image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    i32 mipWidth = static_cast<i32>(m_width);
    i32 mipHeight = static_cast<i32>(m_height);

    for (u32 i = 1; i < m_mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {
            mipWidth > 1 ? mipWidth / 2 : 1,
            mipHeight > 1 ? mipHeight / 2 : 1,
            1
        };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            commandBuffer,
            m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return {};
}

void VulkanTexture::transitionLayout(
    VkCommandBuffer commandBuffer,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_arrayLayers;

    // 设置访问掩码
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStage, dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    m_layout = newLayout;
}

Result<void> VulkanTexture::createSampler(
    VkFilter magFilter,
    VkFilter minFilter,
    VkSamplerAddressMode addressMode)
{
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<f32>(m_mipLevels);

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sampler");
    }

    return {};
}

Result<void> VulkanTexture::createImageView(VkImageAspectFlags aspectMask) {
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_arrayLayers;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create image view");
    }

    return {};
}

Result<u32> VulkanTexture::findMemoryType(
    u32 typeFilter,
    VkMemoryPropertyFlags properties,
    VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::NotFound, "Failed to find suitable memory type");
}

// ============================================================================
// VulkanTextureAtlas 实现
// ============================================================================

VulkanTextureAtlas::VulkanTextureAtlas() = default;

VulkanTextureAtlas::~VulkanTextureAtlas() = default;

Result<void> VulkanTextureAtlas::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    u32 width,
    u32 height)
{
    m_width = width;
    m_height = height;
    m_tileSize = 16;  // 默认瓦片大小
    m_tilesPerRow = width / m_tileSize;
    m_tileU = 1.0f / static_cast<f32>(m_tilesPerRow);
    m_tileV = static_cast<f32>(m_tileSize) / static_cast<f32>(height);

    TextureConfig config;
    config.width = width;
    config.height = height;
    // 使用 UNORM 而不是 SRGB，避免颜色空间转换问题
    config.format = VK_FORMAT_R8G8B8A8_UNORM;
    config.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    config.mipLevels = 1;

    auto result = m_texture.create(device, physicalDevice, config);
    if (!result.success()) {
        return result;
    }

    // 创建 image view
    result = m_texture.createImageView();
    if (!result.success()) {
        return result;
    }

    // 创建 sampler
    result = m_texture.createSampler(
        VK_FILTER_NEAREST, // 像素风格使用最近邻过滤
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_REPEAT);

    return result;
}

void VulkanTextureAtlas::destroy() {
    m_texture.destroy();
    m_width = 0;
    m_height = 0;
    m_tileSize = 0;
    m_tilesPerRow = 0;
    m_tileU = 0.0f;
    m_tileV = 0.0f;
}

Result<void> VulkanTextureAtlas::upload(
    VkCommandBuffer commandBuffer,
    VulkanBuffer& stagingBuffer)
{
    // 转换到传输布局
    m_texture.transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲区到图像
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_width, m_height, 1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer.buffer(),
        m_texture.image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    // 转换到着色器只读布局
    m_texture.transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    return Result<void>::ok();
}

mr::TextureRegion VulkanTextureAtlas::getRegion(u32 tileX, u32 tileY) const {
    f32 u0 = static_cast<f32>(tileX * m_tileSize) / static_cast<f32>(m_width);
    f32 v0 = static_cast<f32>(tileY * m_tileSize) / static_cast<f32>(m_height);
    f32 u1 = static_cast<f32>((tileX + 1) * m_tileSize) / static_cast<f32>(m_width);
    f32 v1 = static_cast<f32>((tileY + 1) * m_tileSize) / static_cast<f32>(m_height);
    return mr::TextureRegion(u0, v0, u1, v1);
}

mr::TextureRegion VulkanTextureAtlas::getRegion(u32 tileIndex) const {
    u32 tileX = tileIndex % m_tilesPerRow;
    u32 tileY = tileIndex / m_tilesPerRow;
    return getRegion(tileX, tileY);
}

} // namespace mr::client
