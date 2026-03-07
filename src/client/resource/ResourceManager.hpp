#pragma once

#include "../common/core/Types.hpp"
#include "../common/core/Result.hpp"
#include "../common/resource/ResourceLocation.hpp"
#include "../common/resource/IResourcePack.hpp"
#include "../renderer/MeshTypes.hpp"
#include "BlockModelLoader.hpp"
#include "BlockStateLoader.hpp"
#include "TextureAtlasBuilder.hpp"
#include <memory>
#include <map>

namespace mr {

// 前向声明
class ResourcePackList;

/**
 * @brief 方块外观信息
 *
 * 包含渲染方块所需的所有数据
 */
struct BlockAppearance {
    std::vector<ModelElement> elements;
    std::map<String, TextureRegion> faceTextures; // 方向 -> 纹理区域
    i32 xRotation = 0;  // X轴旋转
    i32 yRotation = 0;  // Y轴旋转
    bool uvLock = false;
};

/**
 * @brief 资源管理器
 *
 * 统一管理资源包、模型、方块状态和纹理
 */
class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    // ========================================================================
    // 资源包管理
    // ========================================================================

    /**
     * @brief 添加资源包
     * @param resourcePack 资源包指针
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> addResourcePack(ResourcePackPtr resourcePack);

    /**
     * @brief 清除所有资源包
     */
    void clearResourcePacks();

    /**
     * @brief 获取资源包数量
     */
    [[nodiscard]] size_t resourcePackCount() const { return m_resourcePacks.size(); }

    // ========================================================================
    // 资源加载
    // ========================================================================

    /**
     * @brief 加载所有资源
     *
     * 加载方块状态、模型、纹理等。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadAllResources();

    /**
     * @brief 重新加载所有资源
     *
     * 清除缓存并重新加载所有资源。
     * 在资源包变更后调用。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> reload();

    /**
     * @brief 构建纹理图集
     * @return 图集构建结果
     */
    [[nodiscard]] Result<AtlasBuildResult> buildTextureAtlas();

    // ========================================================================
    // 资源查询
    // ========================================================================

    /**
     * @brief 获取方块外观
     * @param blockId 方块资源位置
     * @param properties 属性映射
     * @return 方块外观指针，找不到返回 nullptr
     */
    [[nodiscard]] const BlockAppearance* getBlockAppearance(
        const ResourceLocation& blockId,
        const std::map<String, String>& properties = {}) const;

    /**
     * @brief 获取纹理区域
     * @param textureLocation 纹理资源位置
     * @return 纹理区域指针，找不到返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getTextureRegion(
        const ResourceLocation& textureLocation) const;

    /**
     * @brief 获取已烘焙的模型
     * @param modelLocation 模型资源位置
     * @return 烘焙模型指针，找不到返回 nullptr
     */
    [[nodiscard]] const BakedBlockModel* getBakedModel(
        const ResourceLocation& modelLocation);

    /**
     * @brief 获取方块状态定义
     * @param blockId 方块资源位置
     * @return 方块状态定义指针，找不到返回 nullptr
     */
    [[nodiscard]] const BlockStateDefinition* getBlockState(
        const ResourceLocation& blockId) const;

    // ========================================================================
    // 访问器
    // ========================================================================

    /**
     * @brief 获取模型加载器
     */
    [[nodiscard]] BlockModelLoader& modelLoader() { return m_modelLoader; }
    [[nodiscard]] const BlockModelLoader& modelLoader() const { return m_modelLoader; }

    /**
     * @brief 获取方块状态加载器
     */
    [[nodiscard]] BlockStateLoader& blockStateLoader() { return m_blockStateLoader; }
    [[nodiscard]] const BlockStateLoader& blockStateLoader() const { return m_blockStateLoader; }

    /**
     * @brief 获取纹理图集构建结果
     */
    [[nodiscard]] const AtlasBuildResult& atlasResult() const { return m_atlasResult; }

    /**
     * @brief 检查图集是否已构建
     */
    [[nodiscard]] bool isAtlasBuilt() const { return m_atlasBuilt; }

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置是否在烘焙模型时自动构建图集
     */
    void setAutoBuildAtlas(bool enable) { m_autoBuildAtlas = enable; }

    /**
     * @brief 清除所有缓存
     */
    void clear();

private:
    std::vector<ResourcePackPtr> m_resourcePacks;
    BlockModelLoader m_modelLoader;
    BlockStateLoader m_blockStateLoader;

    // 已烘焙模型缓存
    std::map<ResourceLocation, BakedBlockModel> m_bakedModels;

    // 方块外观缓存
    std::map<String, BlockAppearance> m_blockAppearances;

    // 纹理图集区域映射
    std::map<ResourceLocation, TextureRegion> m_textureRegions;

    // 已构建的图集
    AtlasBuildResult m_atlasResult;

    bool m_autoBuildAtlas = true;
    bool m_atlasBuilt = false;

    // 烘焙所有模型
    [[nodiscard]] Result<void> bakeAllModels();

    // 计算方块外观
    void computeBlockAppearances();

    // 收集所有需要的纹理
    [[nodiscard]] std::set<ResourceLocation> collectRequiredTextures() const;

    // 将纹理路径转换为资源位置
    [[nodiscard]] static ResourceLocation texturePathToLocation(StringView path);
};

} // namespace mr
