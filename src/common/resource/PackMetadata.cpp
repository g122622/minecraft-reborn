#include "PackMetadata.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace mc {

PackMetadata::PackMetadata(i32 packFormat, String description)
    : m_packFormat(packFormat)
    , m_description(std::move(description))
{
}

Result<PackMetadata> PackMetadata::parse(StringView jsonContent) {
    try {
        auto json = nlohmann::json::parse(jsonContent);

        PackMetadata metadata;

        if (json.contains("pack")) {
            const auto& pack = json["pack"];

            if (pack.contains("pack_format")) {
                metadata.m_packFormat = pack["pack_format"].get<i32>();
            }

            if (pack.contains("description")) {
                metadata.m_description = pack["description"].get<String>();
            }
        }

        return metadata;
    } catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError,
                     String("Failed to parse pack.mcmeta: ") + e.what());
    }
}

Result<PackMetadata> PackMetadata::parseFile(StringView filePath) {
    std::ifstream file(String(filePath), std::ios::binary);

    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, String("Cannot open: ") + String(filePath));
    }

    String content((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());

    return parse(content);
}

bool PackMetadata::isCompatible(i32 minFormat, i32 maxFormat) const {
    return m_packFormat >= minFormat && m_packFormat <= maxFormat;
}

} // namespace mc
