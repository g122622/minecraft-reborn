#pragma once

#include "../ResourceMapper.hpp"

namespace mc {
namespace resource {
namespace compat {
namespace v1_13 {

/**
 * @brief MC 1.13+ 资源包的资源映射器
 *
 * MC 1.13+ 使用现代命名约定:
 * - 纹理路径: textures/block/, textures/item/
 * - 纹理名称: jungle_log, white_wool, granite 等
 * - 模型路径: models/block/, models/item/
 *
 * 此映射器本质上是直通的，但提供旧版名称回退以实现兼容性。
 */
class ResourceMapperV113 : public BaseResourceMapper {
public:
    ResourceMapperV113() = default;
    ~ResourceMapperV113() override = default;

    // -------------------------------------------------------------------------
    // 纹理路径转换
    // -------------------------------------------------------------------------

    String toUnifiedTexturePath(StringView path) const override {
        // 现代路径已经是统一的
        return String(path);
    }

    std::vector<String> getTexturePathVariants(StringView unifiedPath) const override;

    String toModernTextureName(StringView name) const override {
        // 已经是现代格式
        return String(name);
    }

    String toLegacyTextureName(StringView name) const override;

    // -------------------------------------------------------------------------
    // 模型路径转换
    // -------------------------------------------------------------------------

    String toUnifiedModelPath(StringView path) const override {
        // 现代模型路径已经是统一的
        return String(path);
    }

    std::vector<String> getModelPathVariants(StringView unifiedPath) const override {
        // 只返回统一路径
        return { String(unifiedPath) };
    }

    // -------------------------------------------------------------------------
    // 方块状态路径转换
    // -------------------------------------------------------------------------

    String toUnifiedBlockStatePath(StringView path) const override {
        // 现代方块状态路径已经是统一的
        return String(path);
    }

    // -------------------------------------------------------------------------
    // 包格式
    // -------------------------------------------------------------------------

    PackFormat getTargetFormat() const override {
        return PackFormat::V1_13_to_1_14;
    }
};

} // namespace v1_13
} // namespace compat
} // namespace resource
} // namespace mc
