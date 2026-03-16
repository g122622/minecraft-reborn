#include "ResourceMapperV113.hpp"
#include "../TextureMapper.hpp"
#include <algorithm>

namespace mc {
namespace resource {
namespace compat {
namespace v1_13 {

String ResourceMapperV113::toLegacyTextureName(StringView name) const {
    // 为了与旧版资源包兼容
    return TextureMapper::instance().getLegacyName(name);
}

std::vector<String> ResourceMapperV113::getTexturePathVariants(StringView unifiedPath) const {
    std::vector<String> variants;

    // 主要: 先尝试现代路径
    variants.push_back(String(unifiedPath));

    // 对于 1.13+ 包，也要尝试旧版路径作为回退
    // 这处理纹理可能以旧版名称存储的情况
    const TextureMapper& mapper = TextureMapper::instance();
    String legacyPath = mapper.toLegacyPath(unifiedPath);

    if (legacyPath != unifiedPath) {
        variants.push_back(legacyPath);
    }

    // 提取基本名称并尝试名称变体
    String pathStr(unifiedPath);
    size_t lastSlash = pathStr.find_last_of("/\\");
    size_t dotPos = pathStr.find_last_of('.');
    if (lastSlash != String::npos && dotPos != String::npos && dotPos > lastSlash) {
        String dirPath = pathStr.substr(0, lastSlash + 1);
        String baseName = pathStr.substr(lastSlash + 1, dotPos - lastSlash - 1);
        String ext = pathStr.substr(dotPos);

        auto nameVariants = mapper.getNameVariants(baseName);
        for (const auto& name : nameVariants) {
            if (name != baseName) {
                String variantPath = dirPath + name + ext;
                if (std::find(variants.begin(), variants.end(), variantPath) == variants.end()) {
                    variants.push_back(variantPath);
                }
            }
        }

        // 还要尝试旧版目录 (textures/blocks/)
        const String modernPrefix = "textures/block/";
        const String legacyPrefix = "textures/blocks/";
        if (dirPath.find(modernPrefix) != String::npos) {
            String legacyDir = dirPath;
            size_t pos = legacyDir.find(modernPrefix);
            legacyDir.replace(pos, modernPrefix.length(), legacyPrefix);

            for (const auto& name : nameVariants) {
                String variantPath = legacyDir + name + ext;
                if (std::find(variants.begin(), variants.end(), variantPath) == variants.end()) {
                    variants.push_back(variantPath);
                }
            }
        }
    }

    return variants;
}

} // namespace v1_13
} // namespace compat
} // namespace resource
} // namespace mc
