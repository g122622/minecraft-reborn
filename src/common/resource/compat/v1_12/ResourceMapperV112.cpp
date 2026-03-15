#include "ResourceMapperV112.hpp"
#include <algorithm>

namespace mc {
namespace resource {
namespace compat {
namespace v1_12 {

ResourceMapperV112::ResourceMapperV112()
    : m_textureMapper(TextureMapper::instance())
{
}

String ResourceMapperV112::toUnifiedTexturePath(StringView path) const {
    String result(path);

    // 将 textures/blocks/ 替换为 textures/block/
    const String oldPrefix = "textures/blocks/";
    const String newPrefix = "textures/block/";
    if (result.find(oldPrefix) == 0) {
        result = newPrefix + result.substr(oldPrefix.length());
    }

    // 将 textures/items/ 替换为 textures/item/
    const String oldItemPrefix = "textures/items/";
    const String newItemPrefix = "textures/item/";
    if (result.find(oldItemPrefix) == 0) {
        result = newItemPrefix + result.substr(oldItemPrefix.length());
    }

    // 将旧版名称转换为现代名称
    size_t lastSlash = result.find_last_of("/\\");
    size_t dotPos = result.find_last_of('.');
    if (lastSlash != String::npos && dotPos != String::npos && dotPos > lastSlash) {
        String dirPath = result.substr(0, lastSlash + 1);
        String baseName = result.substr(lastSlash + 1, dotPos - lastSlash - 1);
        String ext = result.substr(dotPos);

        String modernName = m_textureMapper.getModernName(baseName);
        if (!modernName.empty()) {
            result = dirPath + modernName + ext;
        }
    }

    return result;
}

std::vector<String> ResourceMapperV112::getTexturePathVariants(StringView unifiedPath) const {
    std::vector<String> variants;

    // 首先，将统一（现代）路径转换为旧版格式
    String legacyPath = m_textureMapper.toLegacyPath(unifiedPath);

    // 始终先尝试统一路径（现代格式）
    variants.push_back(String(unifiedPath));

    // 然后尝试旧版路径（如果无映射可能与原路径相同）
    if (legacyPath != unifiedPath) {
        variants.push_back(legacyPath);
    }

    // 还要尝试 textures/blocks/ 前缀变体
    const String modernPrefix = "textures/block/";
    const String legacyPrefix = "textures/blocks/";

    if (unifiedPath.find(modernPrefix) != String::npos) {
        // 找到现代路径，添加旧版目录变体
        String altPath = String(unifiedPath);
        size_t pos = altPath.find(modernPrefix);
        if (pos != String::npos) {
            altPath.replace(pos, modernPrefix.length(), legacyPrefix);
            if (std::find(variants.begin(), variants.end(), altPath) == variants.end()) {
                variants.push_back(altPath);
            }
        }
    } else if (unifiedPath.find(legacyPrefix) != String::npos) {
        // 找到旧版路径，添加现代目录变体
        String altPath = String(unifiedPath);
        size_t pos = altPath.find(legacyPrefix);
        if (pos != String::npos) {
            altPath.replace(pos, legacyPrefix.length(), modernPrefix);
            if (std::find(variants.begin(), variants.end(), altPath) == variants.end()) {
                variants.push_back(altPath);
            }
        }
    }

    // 提取基本名称并尝试名称变体
    String pathStr(unifiedPath);
    size_t lastSlash = pathStr.find_last_of("/\\");
    size_t dotPos = pathStr.find_last_of('.');
    if (lastSlash != String::npos && dotPos != String::npos && dotPos > lastSlash) {
        String dirPath = pathStr.substr(0, lastSlash + 1);
        String baseName = pathStr.substr(lastSlash + 1, dotPos - lastSlash - 1);
        String ext = pathStr.substr(dotPos);

        auto nameVariants = m_textureMapper.getNameVariants(baseName);
        for (const auto& name : nameVariants) {
            if (name != baseName) {
                // 添加相同目录的变体
                String variantPath = dirPath + name + ext;
                if (std::find(variants.begin(), variants.end(), variantPath) == variants.end()) {
                    variants.push_back(variantPath);
                }

                // 添加交换目录的变体 (block <-> blocks)
                if (dirPath.find(modernPrefix) != String::npos) {
                    String legacyDir = dirPath;
                    size_t pos = legacyDir.find(modernPrefix);
                    legacyDir.replace(pos, modernPrefix.length(), legacyPrefix);
                    String legacyVariant = legacyDir + name + ext;
                    if (std::find(variants.begin(), variants.end(), legacyVariant) == variants.end()) {
                        variants.push_back(legacyVariant);
                    }
                }
            }
        }
    }

    return variants;
}

String ResourceMapperV112::toModernTextureName(StringView name) const {
    return m_textureMapper.getModernName(name);
}

String ResourceMapperV112::toLegacyTextureName(StringView name) const {
    return m_textureMapper.getLegacyName(name);
}

String ResourceMapperV112::toUnifiedModelPath(StringView path) const {
    // 模型路径在 1.12 和 1.13+ 之间通常是一致的
    // 大多数模型不需要转换
    return String(path);
}

std::vector<String> ResourceMapperV112::getModelPathVariants(StringView unifiedPath) const {
    // 模型路径通常相同
    // 只返回统一路径
    return { String(unifiedPath) };
}

String ResourceMapperV112::toUnifiedBlockStatePath(StringView path) const {
    // 方块状态路径是一致的
    // 注意: 方块 ID 在 1.13 中有变化（扁平化），但文件路径相似
    return String(path);
}

} // namespace v1_12
} // namespace compat
} // namespace resource
} // namespace mc
