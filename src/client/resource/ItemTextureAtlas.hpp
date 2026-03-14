#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "../../common/resource/ResourceLocation.hpp"
#include "../renderer/MeshTypes.hpp"
#include "../renderer/VulkanTexture.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

class IResourcePack;
class Item;

namespace client {

class VulkanContext;
class VulkanBuffer;

/**
 * @brief 物品纹理图集
 *
 * 管理非方块物品的纹理图集（如工具、食物、材料等）。
 * 方块物品使用方块纹理图集（VulkanTextureAtlas）。
 *
 * 纹理来源：
 * - 资源包中的 textures/item/*.png
 * - 方块纹理图集中的纹理（用于方块物品）
 */
class ItemTextureAtlas {
public:
    ItemTextureAtlas();
    ~ItemTextureAtlas();

    // 禁止拷贝
    ItemTextureAtlas(const ItemTextureAtlas&) = delete;
    ItemTextureAtlas& operator=(const ItemTextureAtlas&) = delete;

    // 允许移动
    ItemTextureAtlas(ItemTextureAtlas&&) noexcept;
    ItemTextureAtlas& operator=(ItemTextureAtlas&&) noexcept;

    /**
     * @brief 创建物品纹理图集
     *
     * @param device Vulkan设备
     * @param physicalDevice 物理设备
     * @param width 图集宽度
     * @param height 图集高度
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        u32 width = 256,
        u32 height = 256);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 从资源包加载物品纹理
     *
     * 加载 textures/item/*.png 文件
     *
     * @param resourcePacks 资源包列表
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadFromResourcePacks(
        const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks);

    /**
     * @brief 上传纹理数据到GPU
     *
     * @param commandBuffer 命令缓冲区
     * @param stagingBuffer 暂存缓冲区
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> upload(
        VkCommandBuffer commandBuffer,
        VulkanBuffer& stagingBuffer);

    /**
     * @brief 获取物品纹理区域
     *
     * @param itemId 物品ID
     * @return 纹理区域指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getItemTexture(u32 itemId) const;

    /**
     * @brief 获取物品纹理区域（通过资源位置）
     *
     * @param location 纹理资源位置
     * @return 纹理区域指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getItemTexture(const ResourceLocation& location) const;

    /**
     * @brief 检查是否有该物品的纹理
     */
    [[nodiscard]] bool hasItemTexture(u32 itemId) const;

    /**
     * @brief 添加纹理区域映射
     *
     * 用于将方块纹理映射到物品
     *
     * @param itemId 物品ID
     * @param region 纹理区域
     */
    void addTextureRegion(u32 itemId, const TextureRegion& region);

    /**
     * @brief 添加纹理区域映射（通过资源位置）
     *
     * @param location 纹理资源位置
     * @param region 纹理区域
     */
    void addTextureRegion(const ResourceLocation& location, const TextureRegion& region);

    /**
     * @brief 获取Vulkan纹理
     */
    VulkanTexture& texture() { return m_texture; }
    const VulkanTexture& texture() const { return m_texture; }

    /**
     * @brief 获取采样器
     */
    VkSampler sampler() const { return m_sampler; }

    /**
     * @brief 获取图像视图
     */
    VkImageView imageView() const { return m_texture.imageView(); }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return m_texture.isValid(); }

    /**
     * @brief 检查纹理数据是否已上传
     */
    [[nodiscard]] bool isUploaded() const { return m_uploaded; }

    /**
     * @brief 获取图集宽度
     */
    [[nodiscard]] u32 width() const { return m_width; }

    /**
     * @brief 获取图集高度
     */
    [[nodiscard]] u32 height() const { return m_height; }

    /**
     * @brief 获取已加载的纹理数量
     */
    [[nodiscard]] size_t textureCount() const { return m_regionsByItemId.size(); }

private:
    VulkanTexture m_texture;
    VkSampler m_sampler = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    u32 m_width = 0;
    u32 m_height = 0;
    bool m_uploaded = false;

    // 纹理区域映射（按物品ID）
    std::unordered_map<u32, TextureRegion> m_regionsByItemId;

    // 纹理区域映射（按资源位置）
    std::unordered_map<ResourceLocation, TextureRegion> m_regionsByLocation;

    // 待上传的像素数据
    std::vector<u8> m_pixels;

    /**
     * @brief 创建采样器
     */
    [[nodiscard]] Result<void> createSampler();
};

} // namespace client
} // namespace mc
