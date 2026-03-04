#pragma once

#include "../core/Types.hpp"
#include "../core/Result.hpp"
#include "PackMetadata.hpp"
#include <memory>
#include <vector>

namespace mr {

/**
 * @brief 资源包抽象接口
 *
 * 提供统一的资源读取接口，支持文件夹和ZIP资源包
 */
class IResourcePack {
public:
    virtual ~IResourcePack() = default;

    // 初始化资源包
    [[nodiscard]] virtual Result<void> initialize() = 0;

    // 获取元数据
    [[nodiscard]] virtual const PackMetadata& metadata() const = 0;

    // 检查资源是否存在
    [[nodiscard]] virtual bool hasResource(StringView resourcePath) const = 0;

    // 读取资源内容
    [[nodiscard]] virtual Result<std::vector<u8>> readResource(StringView resourcePath) const = 0;

    // 读取文本资源
    [[nodiscard]] virtual Result<String> readTextResource(StringView resourcePath) const;

    // 列出目录下的所有资源
    [[nodiscard]] virtual Result<std::vector<String>> listResources(
        StringView directory,
        StringView extension = "") const = 0;

    // 获取资源包路径/名称
    [[nodiscard]] virtual String name() const = 0;
};

using ResourcePackPtr = std::shared_ptr<IResourcePack>;

} // namespace mr
