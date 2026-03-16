#include "ItemTextureAtlas.hpp"
#include "TextureAtlasBuilder.hpp"
#include "../renderer/trident/util/VulkanUtils.hpp"
#include "../../common/resource/IResourcePack.hpp"
#include "../../common/item/Item.hpp"
#include "../../common/item/ItemRegistry.hpp"
#include "../../common/item/BlockItem.hpp"
#include <spdlog/spdlog.h>

// stb_image - only header, implementation in TextureAtlasBuilder.cpp
#include <stb_image.h>
#include <cstring>

namespace mc::client {

ItemTextureAtlas::ItemTextureAtlas() = default;

ItemTextureAtlas::~ItemTextureAtlas() {
    destroy();
}

ItemTextureAtlas::ItemTextureAtlas(ItemTextureAtlas&& other) noexcept
    : m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_commandPool(other.m_commandPool)
    , m_graphicsQueue(other.m_graphicsQueue)
    , m_image(other.m_image)
    , m_imageMemory(other.m_imageMemory)
    , m_imageView(other.m_imageView)
    , m_sampler(other.m_sampler)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_uploaded(other.m_uploaded)
    , m_regionsByItemId(std::move(other.m_regionsByItemId))
    , m_regionsByLocation(std::move(other.m_regionsByLocation))
    , m_pixels(std::move(other.m_pixels))
{
    other.m_device = VK_NULL_HANDLE;
    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_commandPool = VK_NULL_HANDLE;
    other.m_graphicsQueue = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_imageMemory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
    other.m_width = 0;
    other.m_height = 0;
    other.m_uploaded = false;
}

ItemTextureAtlas& ItemTextureAtlas::operator=(ItemTextureAtlas&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_commandPool = other.m_commandPool;
        m_graphicsQueue = other.m_graphicsQueue;
        m_image = other.m_image;
        m_imageMemory = other.m_imageMemory;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_width = other.m_width;
        m_height = other.m_height;
        m_uploaded = other.m_uploaded;
        m_regionsByItemId = std::move(other.m_regionsByItemId);
        m_regionsByLocation = std::move(other.m_regionsByLocation);
        m_pixels = std::move(other.m_pixels);

        other.m_device = VK_NULL_HANDLE;
        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_commandPool = VK_NULL_HANDLE;
        other.m_graphicsQueue = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_imageMemory = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_width = 0;
        other.m_height = 0;
        other.m_uploaded = false;
    }
    return *this;
}

Result<void> ItemTextureAtlas::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 width,
    u32 height)
{
    if (device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Device is null");
    }
    if (commandPool == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Command pool is null");
    }
    if (graphicsQueue == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Graphics queue is null");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_width = width;
    m_height = height;

    // Create image
    auto imageResult = createImage();
    if (!imageResult.success()) {
        return imageResult.error();
    }

    // Create image view
    auto viewResult = createImageView();
    if (!viewResult.success()) {
        vkDestroyImage(m_device, m_image, nullptr);
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_image = VK_NULL_HANDLE;
        m_imageMemory = VK_NULL_HANDLE;
        return viewResult.error();
    }

    // Create sampler
    auto samplerResult = createSampler();
    if (!samplerResult.success()) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        vkDestroyImage(m_device, m_image, nullptr);
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageView = VK_NULL_HANDLE;
        m_image = VK_NULL_HANDLE;
        m_imageMemory = VK_NULL_HANDLE;
        return samplerResult.error();
    }

    // Initialize pixel buffer (transparent)
    m_pixels.resize(static_cast<size_t>(width) * height * 4, 0);

    return {};
}

void ItemTextureAtlas::destroy() {
    if (m_sampler != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    if (m_imageView != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }

    if (m_image != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }

    if (m_imageMemory != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageMemory = VK_NULL_HANDLE;
    }

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_width = 0;
    m_height = 0;
    m_uploaded = false;
    m_pixels.clear();
    m_regionsByItemId.clear();
    m_regionsByLocation.clear();
}

Result<void> ItemTextureAtlas::loadFromResourcePacks(
    const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks)
{
    if (resourcePacks.empty()) {
        spdlog::warn("ItemTextureAtlas: No resource packs provided");
        return {};
    }

    TextureAtlasBuilder builder;
    builder.setMaxSize(m_width, m_height);

    // Iterate all items and load non-block item textures
    ItemRegistry::instance().forEachItem([this, &builder, &resourcePacks](Item& item) {
        // Skip block items (they use block texture atlas)
        const BlockItem* blockItem = dynamic_cast<const BlockItem*>(&item);
        if (blockItem != nullptr) {
            return;
        }

        // Build texture path: textures/item/<item_name>.png
        const ResourceLocation& itemId = item.itemLocation();
        String texturePath = "textures/item/" + itemId.path() + ".png";

        // Try to load texture from resource packs
        for (const auto& pack : resourcePacks) {
            auto result = builder.addTexture(*pack, ResourceLocation(itemId.namespace_(), texturePath));
            if (result.success()) {
                spdlog::debug("ItemTextureAtlas: Loaded texture for item {}", itemId.toString());
                break;
            }
        }
    });

    // Build atlas
    auto atlasResult = builder.build();
    if (!atlasResult.success()) {
        spdlog::warn("ItemTextureAtlas: Failed to build atlas: {}", atlasResult.error().message());
        return atlasResult.error();
    }

    const auto& atlas = atlasResult.value();
    if (atlas.pixels.empty()) {
        spdlog::info("ItemTextureAtlas: No item textures loaded");
        return {};
    }

    // Update atlas size
    m_width = atlas.width;
    m_height = atlas.height;
    m_pixels = atlas.pixels;

    // Store texture regions
    for (const auto& pair : atlas.regions) {
        m_regionsByLocation[pair.first] = pair.second;
    }

    // Map item ID to texture region
    ItemRegistry::instance().forEachItem([this, &atlas](Item& item) {
        const BlockItem* blockItem = dynamic_cast<const BlockItem*>(&item);
        if (blockItem != nullptr) {
            return;
        }

        const ResourceLocation& itemId = item.itemLocation();
        auto it = atlas.regions.find(ResourceLocation(itemId.namespace_(),
                                     "textures/item/" + itemId.path() + ".png"));
        if (it != atlas.regions.end()) {
            m_regionsByItemId[item.itemId()] = it->second;
        }
    });

    spdlog::info("ItemTextureAtlas: Loaded {} item textures ({}x{})",
                 atlas.regions.size(), m_width, m_height);

    return {};
}

Result<void> ItemTextureAtlas::upload() {
    if (m_pixels.empty()) {
        return {};
    }

    if (m_image == VK_NULL_HANDLE) {
        // Need to recreate image (size may have changed)
        auto imageResult = createImage();
        if (!imageResult.success()) {
            return imageResult.error();
        }

        auto viewResult = createImageView();
        if (!viewResult.success()) {
            vkDestroyImage(m_device, m_image, nullptr);
            vkFreeMemory(m_device, m_imageMemory, nullptr);
            m_image = VK_NULL_HANDLE;
            m_imageMemory = VK_NULL_HANDLE;
            return viewResult.error();
        }
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_pixels.size());

    // Create staging buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create staging buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = findMemoryType(memRequirements.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!memTypeResult.success()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to allocate staging memory");
    }

    vkBindBufferMemory(m_device, stagingBuffer, stagingMemory, 0);

    // Map and copy data
    void* mappedData = nullptr;
    if (vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mappedData) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::OperationFailed, "Failed to map staging memory");
    }
    std::memcpy(mappedData, m_pixels.data(), m_pixels.size());
    vkUnmapMemory(m_device, stagingMemory);

    // Use single-time command to upload texture
    VkCommandBuffer cmd = beginSingleTimeCommands();

    // Transition image layout to transfer destination
    transitionImageLayout(cmd,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT);

    // Copy buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_width, m_height, 1};

    vkCmdCopyBufferToImage(cmd,
                           stagingBuffer,
                           m_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &region);

    // Transition to shader read-only layout
    transitionImageLayout(cmd,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    endSingleTimeCommands(cmd);

    // Cleanup staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    m_uploaded = true;

    // Clear pixel data (uploaded to GPU)
    m_pixels.clear();
    m_pixels.shrink_to_fit();

    return {};
}

const TextureRegion* ItemTextureAtlas::getItemTexture(u32 itemId) const {
    auto it = m_regionsByItemId.find(itemId);
    return it != m_regionsByItemId.end() ? &it->second : nullptr;
}

const TextureRegion* ItemTextureAtlas::getItemTexture(const ResourceLocation& location) const {
    auto it = m_regionsByLocation.find(location);
    return it != m_regionsByLocation.end() ? &it->second : nullptr;
}

bool ItemTextureAtlas::hasItemTexture(u32 itemId) const {
    return m_regionsByItemId.find(itemId) != m_regionsByItemId.end();
}

void ItemTextureAtlas::addTextureRegion(u32 itemId, const TextureRegion& region) {
    m_regionsByItemId[itemId] = region;
}

void ItemTextureAtlas::addTextureRegion(const ResourceLocation& location, const TextureRegion& region) {
    m_regionsByLocation[location] = region;
}

Result<void> ItemTextureAtlas::createImage() {
    // Create image
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create item texture atlas image");
    }

    // Allocate memory
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = findMemoryType(memRequirements.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!memTypeResult.success()) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate item texture atlas memory");
    }

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);

    return {};
}

Result<void> ItemTextureAtlas::createSampler() {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create item texture atlas sampler");
    }

    return {};
}

Result<void> ItemTextureAtlas::createImageView() {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create item texture atlas image view");
    }

    return {};
}

Result<u32> ItemTextureAtlas::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
    return renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

VkCommandBuffer ItemTextureAtlas::beginSingleTimeCommands() {
    return renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void ItemTextureAtlas::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, commandBuffer);
}

void ItemTextureAtlas::transitionImageLayout(VkCommandBuffer cmd,
                                              VkImageLayout oldLayout,
                                              VkImageLayout newLayout,
                                              VkPipelineStageFlags srcStage,
                                              VkPipelineStageFlags dstStage) {
    renderer::VulkanUtils::transitionImageLayout(cmd, m_image, oldLayout, newLayout, srcStage, dstStage);
}

} // namespace mc::client
