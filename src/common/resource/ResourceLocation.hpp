#pragma once

#include "../core/Types.hpp"
#include <string>

namespace mr {

/**
 * @brief 资源位置标识符
 *
 * Minecraft标准资源格式: namespace:path
 * 例如: minecraft:textures/blocks/stone
 *       minecraft:models/block/cube_all
 */
class ResourceLocation {
public:
    ResourceLocation();
    explicit ResourceLocation(StringView fullPath);
    ResourceLocation(String namespace_, String path);

    // 解析资源路径
    [[nodiscard]] static ResourceLocation parse(StringView fullPath);

    // 获取命名空间
    [[nodiscard]] const String& namespace_() const noexcept { return m_namespace; }

    // 获取路径
    [[nodiscard]] const String& path() const noexcept { return m_path; }

    // 转换为完整字符串 "namespace:path"
    [[nodiscard]] String toString() const;

    // 转换为文件路径 "assets/namespace/path"
    [[nodiscard]] String toFilePath() const;

    // 转换为文件路径（带扩展名）"assets/namespace/path.ext"
    [[nodiscard]] String toFilePath(StringView extension) const;

    // 比较
    [[nodiscard]] bool operator==(const ResourceLocation& other) const;
    [[nodiscard]] bool operator!=(const ResourceLocation& other) const;
    [[nodiscard]] bool operator<(const ResourceLocation& other) const;

    // 哈希
    [[nodiscard]] size_t hash() const;

private:
    String m_namespace;
    String m_path;
};

} // namespace mr

// std::hash特化
namespace std {
template<>
struct hash<mr::ResourceLocation> {
    size_t operator()(const mr::ResourceLocation& loc) const noexcept {
        return loc.hash();
    }
};
}
