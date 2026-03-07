#pragma once

#include "common/resource/IResourcePack.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mr {

/**
 * @brief 内存资源包
 *
 * 从内存中提供资源，用于内置资源（如原版基础模型）。
 * 优先级最高，始终加载。
 */
class InMemoryResourcePack : public IResourcePack {
public:
    /**
     * @brief 构造函数
     * @param name 资源包名称
     */
    explicit InMemoryResourcePack(String name);

    /**
     * @brief 析构函数
     */
    ~InMemoryResourcePack() override = default;

    /**
     * @brief 添加资源
     * @param path 资源路径（如 "assets/minecraft/models/block/cube_all.json"）
     * @param content 资源内容
     */
    void addResource(String path, String content);

    /**
     * @brief 添加二进制资源
     * @param path 资源路径
     * @param data 资源数据
     */
    void addResource(String path, std::vector<u8> data);

    /**
     * @brief 添加目录条目（用于 listResources）
     * @param directory 目录路径
     */
    void addDirectory(String directory);

    // IResourcePack 接口实现

    [[nodiscard]] Result<void> initialize() override;
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }
    [[nodiscard]] bool hasResource(StringView resourcePath) const override;
    [[nodiscard]] Result<std::vector<u8>> readResource(StringView resourcePath) const override;
    [[nodiscard]] Result<std::vector<String>> listResources(
        StringView directory,
        StringView extension = "") const override;
    [[nodiscard]] String name() const override { return m_name; }

private:
    String m_name;
    PackMetadata m_metadata;
    std::unordered_map<String, std::vector<u8>> m_resources;
    std::unordered_set<String> m_directories;  // 用于 listResources

    /**
     * @brief 规范化路径
     */
    [[nodiscard]] static String normalizePath(StringView path);
};

} // namespace mr
