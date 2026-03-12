#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "../../common/resource/ResourceLocation.hpp"
#include "../renderer/entity/EntityTextureAtlas.hpp"
#include <memory>
#include <vector>
#include <string>

namespace mr {
class IResourcePack;
}

namespace mr::client {

/**
 * @brief 实体纹理加载器
 *
 * 从资源包加载实体纹理并构建纹理图集。
 * 支持 MC 1.12 和 MC 1.13+ 的纹理路径格式。
 *
 * 参考 MC 1.16.5 实体纹理加载
 */
class EntityTextureLoader {
public:
    EntityTextureLoader() = default;
    ~EntityTextureLoader() = default;

    // 禁止拷贝
    EntityTextureLoader(const EntityTextureLoader&) = delete;
    EntityTextureLoader& operator=(const EntityTextureLoader&) = delete;

    /**
     * @brief 加载默认实体纹理
     *
     * 加载常见实体（猪、牛、羊、鸡等）的纹理。
     *
     * @param pack 资源包
     * @param atlas 纹理图集（需要已初始化）
     * @return 加载的纹理数量
     */
    [[nodiscard]] Result<u32> loadDefaultTextures(mr::IResourcePack& pack, EntityTextureAtlas& atlas);

    /**
     * @brief 加载指定实体类型的纹理
     * @param pack 资源包
     * @param atlas 纹理图集
     * @param entityTypeId 实体类型ID（如 "minecraft:pig"）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadEntityTexture(mr::IResourcePack& pack,
                                                  EntityTextureAtlas& atlas,
                                                  const String& entityTypeId);

    /**
     * @brief 获取默认实体类型列表
     */
    [[nodiscard]] static const std::vector<String>& getDefaultEntityTypes();

private:
    /**
     * @brief 获取实体纹理路径
     * @param entityTypeId 实体类型ID
     * @return 纹理资源位置列表（尝试多个路径）
     */
    [[nodiscard]] std::vector<ResourceLocation> getTexturePaths(const String& entityTypeId);

    /**
     * @brief 解析实体类型名称
     * @param entityTypeId 实体类型ID（如 "minecraft:pig"）
     * @return 实体名称（如 "pig"）
     */
    [[nodiscard]] static String parseEntityName(const String& entityTypeId);
};

} // namespace mr::client
