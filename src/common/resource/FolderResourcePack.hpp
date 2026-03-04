#pragma once

#include "IResourcePack.hpp"
#include <filesystem>

namespace mr {

/**
 * @brief 文件夹资源包实现
 *
 * 从文件系统目录读取资源
 * 目录结构应符合Minecraft资源包格式
 */
class FolderResourcePack : public IResourcePack {
public:
    explicit FolderResourcePack(String rootPath);
    ~FolderResourcePack() override = default;

    // IResourcePack接口实现
    [[nodiscard]] Result<void> initialize() override;
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }
    [[nodiscard]] bool hasResource(StringView resourcePath) const override;
    [[nodiscard]] Result<std::vector<u8>> readResource(StringView resourcePath) const override;
    [[nodiscard]] Result<std::vector<String>> listResources(
        StringView directory,
        StringView extension = "") const override;
    [[nodiscard]] String name() const override { return m_name; }

    // 获取根路径
    [[nodiscard]] const String& rootPath() const { return m_rootPath; }

private:
    String m_rootPath;
    String m_name;
    PackMetadata m_metadata;

    // 规范化路径
    [[nodiscard]] String normalizePath(StringView resourcePath) const;
};

} // namespace mr
