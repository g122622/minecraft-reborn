#pragma once

#include "../core/Types.hpp"
#include "../core/Result.hpp"
#include <string>

namespace mr {

/**
 * @brief 资源包元数据
 *
 * 解析pack.mcmeta文件
 */
class PackMetadata {
public:
    PackMetadata() = default;

    // 从JSON字符串解析
    [[nodiscard]] static Result<PackMetadata> parse(StringView jsonContent);

    // 从文件解析
    [[nodiscard]] static Result<PackMetadata> parseFile(StringView filePath);

    // 获取pack_format版本
    [[nodiscard]] i32 packFormat() const noexcept { return m_packFormat; }

    // 获取描述文本
    [[nodiscard]] const String& description() const noexcept { return m_description; }

    // 验证版本兼容性
    [[nodiscard]] bool isCompatible(i32 minFormat, i32 maxFormat) const;

private:
    i32 m_packFormat = 0;
    String m_description;
};

} // namespace mr
