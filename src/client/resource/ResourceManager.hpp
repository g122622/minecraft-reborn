#pragma once

#include "../common/core/Types.hpp"
#include "../common/core/Result.hpp"
#include "../common/resource/ResourceLocation.hpp"
#include "../common/resource/IResourcePack.hpp"
#include "../common/renderer/MeshTypes.hpp"
#include "BlockModelLoader.hpp"
#include "BlockStateLoader.hpp"
#include "TextureAtlasBuilder.hpp"
#include <memory>
#include <map>

namespace mr {

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

    // 添加资源包
    [[nodiscard]] Result<void> addResourcePack(ResourcePackPtr resourcePack);

    // 加载所有资源
    [[nodiscard]] Result<void> loadAllResources();

    // 构建纹理图集
    [[nodiscard]] Result<AtlasBuildResult> buildTextureAtlas();

    // 获取方块外观
    [[nodiscard]] const BlockAppearance* getBlockAppearance(
        const ResourceLocation& blockId,
        const std::map<String, String>& properties = {}) const;

    // 获取纹理区域
    [[nodiscard]] const TextureRegion* getTextureRegion(
        const ResourceLocation& textureLocation) const;

    // 获取已烘焙的模型
    [[nodiscard]] const BakedBlockModel* getBakedModel(
        const ResourceLocation& modelLocation);

    // 获取方块状态定义
    [[nodiscard]] const BlockStateDefinition* getBlockState(
        const ResourceLocation& blockId) const;

    // 获取模型加载器
    [[nodiscard]] BlockModelLoader& modelLoader() { return m_modelLoader; }
    [[nodiscard]] const BlockModelLoader& modelLoader() const { return m_modelLoader; }

    // 获取方块状态加载器
    [[nodiscard]] BlockStateLoader& blockStateLoader() { return m_blockStateLoader; }
    [[nodiscard]] const BlockStateLoader& blockStateLoader() const { return m_blockStateLoader; }

    // 清除所有缓存
    void clear();

    // 获取资源包数量
    [[nodiscard]] size_t resourcePackCount() const { return m_resourcePacks.size(); }

    // 设置是否在烘焙模型时自动构建图集
    void setAutoBuildAtlas(bool enable) { m_autoBuildAtlas = enable; }

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
