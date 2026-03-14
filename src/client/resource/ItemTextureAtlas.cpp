#include "ItemTextureAtlas.hpp"
#include "TextureAtlasBuilder.hpp"
#include "../../common/resource/IResourcePack.hpp"
#include "../../common/item/Item.hpp"
#include "../../common/item/ItemRegistry.hpp"
#include "../../common/item/BlockItem.hpp"
#include "../renderer/VulkanContext.hpp"
#include "../renderer/VulkanBuffer.hpp"
#include <spdlog/spdlog.h>

// stb_image - 只需要头文件，实现在TextureAtlasBuilder.cpp中
#include <stb_image.h>
#include <cstring>

namespace mc::client {

ItemTextureAtlas::ItemTextureAtlas() = default;

ItemTextureAtlas::~ItemTextureAtlas() {
    destroy();
}

ItemTextureAtlas::ItemTextureAtlas(ItemTextureAtlas&& other) noexcept
    : m_texture(std::move(other.m_texture))
    , m_sampler(other.m_sampler)
    , m_device(other.m_device)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_uploaded(other.m_uploaded)
    , m_regionsByItemId(std::move(other.m_regionsByItemId))
    , m_regionsByLocation(std::move(other.m_regionsByLocation))
    , m_pixels(std::move(other.m_pixels))
{
    other.m_sampler = VK_NULL_HANDLE;
    other.m_device = VK_NULL_HANDLE;
    other.m_width = 0;
    other.m_height = 0;
    other.m_uploaded = false;
}

ItemTextureAtlas& ItemTextureAtlas::operator=(ItemTextureAtlas&& other) noexcept {
    if (this != &other) {
        destroy();
        m_texture = std::move(other.m_texture);
        m_sampler = other.m_sampler;
        m_device = other.m_device;
        m_width = other.m_width;
        m_height = other.m_height;
        m_uploaded = other.m_uploaded;
        m_regionsByItemId = std::move(other.m_regionsByItemId);
        m_regionsByLocation = std::move(other.m_regionsByLocation);
        m_pixels = std::move(other.m_pixels);

        other.m_sampler = VK_NULL_HANDLE;
        other.m_device = VK_NULL_HANDLE;
        other.m_width = 0;
        other.m_height = 0;
        other.m_uploaded = false;
    }
    return *this;
}

Result<void> ItemTextureAtlas::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    u32 width,
    u32 height)
{
    if (device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Device is null");
    }

    m_device = device;
    m_width = width;
    m_height = height;

    // 创建纹理
    TextureConfig config;
    config.width = width;
    config.height = height;
    config.format = VK_FORMAT_R8G8B8A8_SRGB;  // RGBA格式
    config.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    config.mipLevels = 1;

    auto result = m_texture.create(device, physicalDevice, config);
    if (!result.success()) {
        return result.error();
    }

    // 创建图像视图
    result = m_texture.createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
    if (!result.success()) {
        return result.error();
    }

    // 创建采样器
    auto samplerResult = createSampler();
    if (!samplerResult.success()) {
        return samplerResult.error();
    }

    // 初始化像素缓冲区（透明）
    m_pixels.resize(static_cast<size_t>(width) * height * 4, 0);

    return {};
}

void ItemTextureAtlas::destroy() {
    if (m_sampler != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    m_texture.destroy();

    m_device = VK_NULL_HANDLE;
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

    // 遍历所有物品，加载非方块物品的纹理
    ItemRegistry::instance().forEachItem([&](Item& item) {
        // 跳过方块物品（它们使用方块纹理图集）
        const BlockItem* blockItem = dynamic_cast<const BlockItem*>(&item);
        if (blockItem != nullptr) {
            return;  // 继续下一个物品
        }

        // 构建纹理路径: textures/item/<item_name>.png
        const ResourceLocation& itemId = item.itemLocation();
        String texturePath = "textures/item/" + itemId.path() + ".png";
        ResourceLocation textureLocation(itemId.namespace_(), "item/" + itemId.path());

        // 尝试从资源包加载纹理
        for (const auto& pack : resourcePacks) {
            auto result = builder.addTexture(*pack, ResourceLocation(itemId.namespace_(), texturePath));
            if (result.success()) {
                spdlog::debug("ItemTextureAtlas: Loaded texture for item {}", itemId.toString());
                break;
            }
        }
    });

    // 构建图集
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

    // 更新图集尺寸
    m_width = atlas.width;
    m_height = atlas.height;
    m_pixels = atlas.pixels;

    // 存储纹理区域
    for (const auto& [location, region] : atlas.regions) {
        m_regionsByLocation[location] = region;
    }

    // 映射物品ID到纹理区域
    ItemRegistry::instance().forEachItem([&](Item& item) {
        const BlockItem* blockItem = dynamic_cast<const BlockItem*>(&item);
        if (blockItem != nullptr) {
            return;  // 跳过方块物品
        }

        const ResourceLocation& itemId = item.itemLocation();
        ResourceLocation textureLocation(itemId.namespace_(), "item/" + itemId.path());

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

Result<void> ItemTextureAtlas::upload(
    VkCommandBuffer commandBuffer,
    VulkanBuffer& stagingBuffer)
{
    if (m_pixels.empty()) {
        return {};
    }

    VkDeviceSize requiredSize = static_cast<VkDeviceSize>(m_pixels.size());

    // 检查暂存缓冲区大小
    if (stagingBuffer.size() < requiredSize) {
        return Error(ErrorCode::CapacityExceeded,
                     "Staging buffer too small for item texture atlas");
    }

    // 映射并复制数据
    auto mapResult = stagingBuffer.map();
    if (!mapResult.success()) {
        return mapResult.error();
    }

    void* mapped = mapResult.value();
    std::memcpy(mapped, m_pixels.data(), m_pixels.size());
    stagingBuffer.unmap();

    // 转换图像布局
    m_texture.transitionLayout(commandBuffer,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲到图像
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

    vkCmdCopyBufferToImage(commandBuffer,
                           stagingBuffer.buffer(),
                           m_texture.image(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &region);

    // 转换到着色器读取布局
    m_texture.transitionLayout(commandBuffer,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    m_uploaded = true;

    // 清理像素数据（已上传到GPU）
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

Result<void> ItemTextureAtlas::createSampler() {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;  // 物品纹理使用最近邻过滤（像素风格）
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
        return Error(ErrorCode::InitializationFailed, "Failed to create item texture sampler");
    }

    return {};
}

} // namespace mc::client
