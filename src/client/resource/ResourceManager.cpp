#include "ResourceManager.hpp"
#include "../common/resource/FolderResourcePack.hpp"
#include <spdlog/spdlog.h>

// stb_image for PNG loading (implementation in TextureAtlasBuilder.cpp)
#include <stb_image.h>

namespace mr {

namespace {
    /**
     * @brief 尝试多种纹理路径变体来加载纹理
     *
     * MC 1.12 使用 textures/blocks/ 路径
     * MC 1.13+ 使用 textures/block/ 路径
     *
     * 此函数会尝试所有可能的路径组合：
     * 1. 原始路径
     * 2. block/ -> blocks/
     * 3. blocks/ -> block/
     */
    bool tryLoadTextureVariants(
        IResourcePack& pack,
        TextureAtlasBuilder& builder,
        const ResourceLocation& originalLoc,
        std::vector<String>& triedPaths)
    {
        const String& originalPath = originalLoc.path();
        const String& ns = originalLoc.namespace_();
        triedPaths.push_back(originalPath);

        // 1. 尝试原始路径
        auto result = builder.addTexture(pack, originalLoc);
        if (result.success()) {
            return true;
        }

        // 生成所有可能的路径变体
        std::vector<String> pathVariants;

        // 2. block <-> blocks 路径互换
        if (originalPath.find("textures/block/") != String::npos) {
            // MC 1.13+ 路径 -> MC 1.12 路径
            String oldPath = originalPath;
            size_t pos = oldPath.find("textures/block/");
            if (pos != String::npos) {
                oldPath.replace(pos, 15, "textures/blocks/");
                pathVariants.push_back(oldPath);
            }
        } else if (originalPath.find("textures/blocks/") != String::npos) {
            // MC 1.12 路径 -> MC 1.13+ 路径
            String newPath = originalPath;
            size_t pos = newPath.find("textures/blocks/");
            if (pos != String::npos) {
                newPath.replace(pos, 16, "textures/block/");
                pathVariants.push_back(newPath);
            }
        }

        // 3. 尝试路径变体
        for (const auto& variantPath : pathVariants) {
            triedPaths.push_back(variantPath);
            ResourceLocation variantLoc(ns, variantPath);
            String filePath = variantLoc.toFilePath("png");

            if (pack.hasResource(filePath)) {
                auto readResult = pack.readResource(filePath);
                if (readResult.success()) {
                    int width, height, channels;
                    stbi_uc* pixels = stbi_load_from_memory(
                        readResult.value().data(),
                        static_cast<int>(readResult.value().size()),
                        &width, &height, &channels, 4);

                    if (pixels) {
                        std::vector<u8> pixelData(pixels, pixels + width * height * 4);
                        stbi_image_free(pixels);

                        // 使用原始路径作为键
                        builder.addTexture(originalLoc, pixelData,
                                           static_cast<u32>(width),
                                           static_cast<u32>(height));
                        return true;
                    }
                }
            }
        }

        return false;
    }

    /**
     * @brief 尝试纹理名称变体（处理命名差异）
     *
     * 某些资源包使用不同的纹理命名，例如：
     * - wool_colored_white vs white_wool
     * - acacia_planks vs planks_acacia
     * - grass_block_top vs grass_top
     */
    std::vector<String> generateTextureNameVariants(const String& textureName) {
        std::vector<String> variants;

        // 提取文件名部分（去掉路径和扩展名）
        size_t lastSlash = textureName.find_last_of("/\\");
        String dirPath = (lastSlash != String::npos) ? textureName.substr(0, lastSlash + 1) : "";
        String fileName = (lastSlash != String::npos) ? textureName.substr(lastSlash + 1) : textureName;

        // 去掉扩展名
        size_t dotPos = fileName.find_last_of('.');
        String baseName = (dotPos != String::npos) ? fileName.substr(0, dotPos) : fileName;
        String extension = (dotPos != String::npos) ? fileName.substr(dotPos) : "";

        // 常见的名称映射 (MC 1.13+ -> MC 1.12)
        static const std::vector<std::pair<String, String>> nameMappings = {
            // Wool variants (MC 1.13+ -> MC 1.12)
            {"white_wool", "wool_colored_white"},
            {"orange_wool", "wool_colored_orange"},
            {"magenta_wool", "wool_colored_magenta"},
            {"light_blue_wool", "wool_colored_light_blue"},
            {"yellow_wool", "wool_colored_yellow"},
            {"lime_wool", "wool_colored_lime"},
            {"pink_wool", "wool_colored_pink"},
            {"gray_wool", "wool_colored_gray"},
            {"light_gray_wool", "wool_colored_silver"},
            {"cyan_wool", "wool_colored_cyan"},
            {"purple_wool", "wool_colored_purple"},
            {"blue_wool", "wool_colored_blue"},
            {"brown_wool", "wool_colored_brown"},
            {"green_wool", "wool_colored_green"},
            {"red_wool", "wool_colored_red"},
            {"black_wool", "wool_colored_black"},

            // Planks variants (MC 1.13+ -> MC 1.12)
            {"oak_planks", "planks_oak"},
            {"spruce_planks", "planks_spruce"},
            {"birch_planks", "planks_birch"},
            {"jungle_planks", "planks_jungle"},
            {"acacia_planks", "planks_acacia"},
            {"dark_oak_planks", "planks_big_oak"},

            // Terracotta variants (MC 1.12 naming)
            {"white_terracotta", "hardened_clay_stained_white"},
            {"orange_terracotta", "hardened_clay_stained_orange"},
            {"magenta_terracotta", "hardened_clay_stained_magenta"},
            {"light_blue_terracotta", "hardened_clay_stained_light_blue"},
            {"yellow_terracotta", "hardened_clay_stained_yellow"},
            {"lime_terracotta", "hardened_clay_stained_lime"},
            {"pink_terracotta", "hardened_clay_stained_pink"},
            {"gray_terracotta", "hardened_clay_stained_gray"},
            {"light_gray_terracotta", "hardened_clay_stained_silver"},
            {"cyan_terracotta", "hardened_clay_stained_cyan"},
            {"purple_terracotta", "hardened_clay_stained_purple"},
            {"blue_terracotta", "hardened_clay_stained_blue"},
            {"brown_terracotta", "hardened_clay_stained_brown"},
            {"green_terracotta", "hardened_clay_stained_green"},
            {"red_terracotta", "hardened_clay_stained_red"},
            {"black_terracotta", "hardened_clay_stained_black"},

            // Grass block (MC 1.13+ -> MC 1.12)
            {"grass_block_top", "grass_top"},
            {"grass_block_side", "grass_side"},
            {"grass_block_bottom", "dirt"},
            {"grass_block_snow", "grass_side_snowed"},

            // Stone variants (MC 1.13+ -> MC 1.12)
            {"granite", "stone_granite"},
            {"polished_granite", "stone_granite_smooth"},
            {"diorite", "stone_diorite"},
            {"polished_diorite", "stone_diorite_smooth"},
            {"andesite", "stone_andesite"},
            {"polished_andesite", "stone_andesite_smooth"},

            // Sandstone variants
            {"sandstone_top", "sandstone_top"},
            {"sandstone_bottom", "sandstone_bottom"},
            {"sandstone_side", "sandstone_normal"},
            {"cut_sandstone", "sandstone_carved"},
            {"chiseled_sandstone", "sandstone_smooth"},
            {"red_sandstone_top", "red_sandstone_top"},
            {"red_sandstone_bottom", "red_sandstone_bottom"},
            {"red_sandstone_side", "red_sandstone_normal"},
            {"red_sandstone", "red_sandstone_normal"},

            // Podzol (MC 1.13+ -> MC 1.12)
            {"podzol_top", "dirt_podzol_top"},
            {"podzol_side", "dirt_podzol_side"},
            {"podzol", "dirt_podzol_top"},

            // Basalt (note: may not exist in MC 1.12 packs)
            {"basalt_side", "basalt_side"},
            {"basalt_top", "basalt_top"},
            {"polished_basalt_side", "basalt_polished_side"},
            {"polished_basalt_top", "basalt_polished_top"},

            // Other blocks
            {"blackstone", "blackstone"},
            {"polished_blackstone", "polished_blackstone"},
            {"soul_soil", "soul_soil"},
            {"wet_sponge", "sponge_wet"},
            {"bricks", "brick"},
        };

        // 检查基础名称是否匹配任何映射
        for (const auto& [newName, oldName] : nameMappings) {
            if (baseName == newName) {
                // 使用 MC 1.12 名称
                String variant = dirPath + oldName + extension;
                variants.push_back(variant);
            } else if (baseName == oldName) {
                // 使用 MC 1.13+ 名称
                String variant = dirPath + newName + extension;
                variants.push_back(variant);
            }
        }

        return variants;
    }
}

Result<void> ResourceManager::addResourcePack(ResourcePackPtr resourcePack) {
    if (!resourcePack) {
        return Error(ErrorCode::InvalidArgument, "Resource pack is null");
    }

    // 初始化资源包
    auto result = resourcePack->initialize();
    if (result.failed()) {
        return result.error();
    }

    spdlog::info("ResourceManager: Added resource pack '{}'", resourcePack->name());
    m_resourcePacks.push_back(std::move(resourcePack));
    return Result<void>::ok();
}

Result<void> ResourceManager::loadAllResources() {
    // 设置模型加载器的资源包列表
    m_modelLoader.setResourcePackList(m_resourcePacks);

    // 加载方块状态
    for (auto& pack : m_resourcePacks) {
        m_blockStateLoader.loadFromResourcePack(*pack);
    }

    // 加载模型 (按需加载，不预加载)
    for (auto& pack : m_resourcePacks) {
        m_modelLoader.loadFromResourcePack(*pack);
    }

    // 烘焙模型
    bakeAllModels();

    // 注意：computeBlockAppearances 在 buildTextureAtlas 后调用
    // 因为需要纹理区域数据

    spdlog::info("ResourceManager: Loaded {} block states, {} models",
                m_blockStateLoader.getLoadedBlockStates().size(),
                m_bakedModels.size());

    return Result<void>::ok();
}

Result<AtlasBuildResult> ResourceManager::buildTextureAtlas() {
    // 收集所需纹理
    auto textures = collectRequiredTextures();

    spdlog::info("Collecting {} textures for atlas", textures.size());

    TextureAtlasBuilder builder;
    builder.setMaxSize(4096, 4096);
    builder.setPadding(0);

    // 添加所有纹理
    // MC 1.12/1.13+ 兼容性：支持 block/ <-> blocks/ 路径互换
    size_t addedCount = 0;
    size_t fallbackCount = 0;
    size_t nameVariantCount = 0;
    size_t failedCount = 0;
    std::vector<String> failedTextures;

    for (const auto& texLoc : textures) {
        bool added = false;
        std::vector<String> triedPaths;

        // 遍历资源包（后添加的优先级更高）
        for (auto it = m_resourcePacks.rbegin(); it != m_resourcePacks.rend() && !added; ++it) {
            auto& pack = *it;

            // 尝试原始路径和 block/blocks 互换
            if (tryLoadTextureVariants(*pack, builder, texLoc, triedPaths)) {
                added = true;
                addedCount++;
                if (triedPaths.size() > 1) {
                    fallbackCount++;
                }
                break;
            }

            // 尝试纹理名称变体（处理 wool_colored_white vs white_wool 等命名差异）
            if (!added) {
                String texName = texLoc.path();

                auto nameVariants = generateTextureNameVariants(texName);

                for (const auto& variantName : nameVariants) {
                    ResourceLocation variantLoc(texLoc.namespace_(), variantName);

                    // 尝试直接路径和 block/blocks 变体
                    std::vector<String> pathsToTry;

                    // 原始变体路径
                    pathsToTry.push_back(variantLoc.toFilePath("png"));

                    // block -> blocks 变体
                    const String& variantPath = variantLoc.path();
                    if (variantPath.find("textures/block/") != String::npos) {
                        String oldPath = variantPath;
                        size_t pos = oldPath.find("textures/block/");
                        if (pos != String::npos) {
                            oldPath.replace(pos, 15, "textures/blocks/");
                            ResourceLocation oldLoc(variantLoc.namespace_(), oldPath);
                            pathsToTry.push_back(oldLoc.toFilePath("png"));
                        }
                    }

                    for (const auto& filePath : pathsToTry) {
                        if (pack->hasResource(filePath)) {
                            auto readResult = pack->readResource(filePath);
                            if (readResult.success()) {
                                int width, height, channels;
                                stbi_uc* pixels = stbi_load_from_memory(
                                    readResult.value().data(),
                                    static_cast<int>(readResult.value().size()),
                                    &width, &height, &channels, 4);

                                if (pixels) {
                                    std::vector<u8> pixelData(pixels, pixels + width * height * 4);
                                    stbi_image_free(pixels);

                                    // 用原始请求名称注册，而不是变体名称
                                    builder.addTexture(texLoc, pixelData,
                                                       static_cast<u32>(width),
                                                       static_cast<u32>(height));
                                    added = true;
                                    addedCount++;
                                    nameVariantCount++;
                                    break;
                                }
                            }
                        }
                    }

                    if (added) break;
                }
            }
        }

        if (!added) {
            failedTextures.push_back(texLoc.toString() + " -> " + texLoc.toFilePath("png"));
            failedCount++;
        }
    }

    spdlog::info("Texture atlas: {} added ({} via block/blocks fallback, {} via name variant), {} failed",
                 addedCount, fallbackCount, nameVariantCount, failedCount);

    // 输出失败纹理的详细信息
    if (failedCount > 0) {
        spdlog::warn("Failed textures (first 50 of {}):", failedCount);
        for (size_t i = 0; i < std::min(failedTextures.size(), size_t(50)); ++i) {
            spdlog::warn("  - {}", failedTextures[i]);
        }
    }

    // 构建图集
    auto result = builder.build();
    if (result.failed()) {
        return result.error();
    }

    // 缓存结果
    m_atlasResult = result.value();
    m_textureRegions = m_atlasResult.regions;
    m_atlasBuilt = true;

    // 构建纹理图集后计算方块外观（这样纹理区域可用）
    computeBlockAppearances();

    spdlog::info("ResourceManager: {} appearances computed", m_blockAppearances.size());

    return m_atlasResult;
}

const BlockAppearance* ResourceManager::getBlockAppearance(
    const ResourceLocation& blockId,
    const std::map<String, String>& properties) const
{
    // 构建缓存键
    String cacheKey = blockId.toString();
    if (!properties.empty()) {
        cacheKey += "?";
        bool first = true;
        for (const auto& [key, value] : properties) {
            if (!first) cacheKey += ",";
            cacheKey += key + "=" + value;
            first = false;
        }
    }

    auto it = m_blockAppearances.find(cacheKey);
    if (it != m_blockAppearances.end()) {
        return &it->second;
    }

    return nullptr;
}

const TextureRegion* ResourceManager::getTextureRegion(
    const ResourceLocation& textureLocation) const
{
    auto it = m_textureRegions.find(textureLocation);
    if (it != m_textureRegions.end()) {
        return &it->second;
    }
    return nullptr;
}

const BakedBlockModel* ResourceManager::getBakedModel(
    const ResourceLocation& modelLocation)
{
    // 检查缓存
    auto it = m_bakedModels.find(modelLocation);
    if (it != m_bakedModels.end()) {
        return &it->second;
    }

    // 烘焙模型
    // 首先需要设置资源包
    if (!m_resourcePacks.empty()) {
        m_modelLoader.loadFromResourcePack(*m_resourcePacks[0]);
    }

    auto result = m_modelLoader.bakeModel(modelLocation);
    if (result.failed()) {
        return nullptr;
    }

    m_bakedModels[modelLocation] = result.value();
    return &m_bakedModels[modelLocation];
}

const BlockStateDefinition* ResourceManager::getBlockState(
    const ResourceLocation& blockId) const
{
    return m_blockStateLoader.getBlockState(blockId);
}

void ResourceManager::clear() {
    m_resourcePacks.clear();
    m_modelLoader.clearCache();
    m_blockStateLoader.clearCache();
    m_bakedModels.clear();
    m_blockAppearances.clear();
    m_textureRegions.clear();
    m_atlasResult = AtlasBuildResult();
    m_atlasBuilt = false;
}

void ResourceManager::clearResourcePacks() {
    m_resourcePacks.clear();
}

Result<void> ResourceManager::reload() {
    // 清除缓存但保留资源包列表
    m_modelLoader.clearCache();
    m_blockStateLoader.clearCache();
    m_bakedModels.clear();
    m_blockAppearances.clear();
    m_textureRegions.clear();
    m_atlasResult = AtlasBuildResult();
    m_atlasBuilt = false;

    // 重新加载所有资源
    return loadAllResources();
}

Result<void> ResourceManager::bakeAllModels() {
    // 获取所有方块状态
    auto blockStates = m_blockStateLoader.getLoadedBlockStates();

    size_t successCount = 0;
    size_t failCount = 0;

    for (const auto& blockId : blockStates) {
        const auto* def = m_blockStateLoader.getBlockState(blockId);
        if (!def) continue;

        // 烘焙所有变体的模型
        for (const auto& [stateStr, variantList] : def->getAllVariants()) {
            for (const auto& variant : variantList.variants) {
                if (!m_bakedModels.count(variant.model)) {
                    auto result = m_modelLoader.bakeModel(variant.model);
                    if (result.success()) {
                        m_bakedModels[variant.model] = result.value();
                        successCount++;
                    } else {
                        if (failCount < 50) {
                            spdlog::warn("Failed to bake model '{}' for block '{}': {}",
                                         variant.model.toString(), blockId.toString(),
                                         result.error().toString());
                        } else if (failCount == 50) {
                            spdlog::warn("More model bake failures... (suppressed)");
                        }
                        failCount++;
                    }
                }
            }
        }
    }

    if (failCount > 0) {
        spdlog::warn("ResourceManager: Baked {} models ({} failed)",
                     successCount, failCount);
    } else {
        spdlog::info("ResourceManager: Baked {} models successfully", successCount);
    }

    return Result<void>::ok();
}

void ResourceManager::computeBlockAppearances() {
    // 获取所有方块状态
    auto blockStates = m_blockStateLoader.getLoadedBlockStates();

    u32 totalAppearances = 0;
    u32 appearancesWithTextures = 0;

    for (const auto& blockId : blockStates) {
        const auto* def = m_blockStateLoader.getBlockState(blockId);
        if (!def) continue;

        // 处理每个状态变体
        for (const auto& [stateStr, variantList] : def->getAllVariants()) {
            // 构建缓存键
            String cacheKey = blockId.toString();
            if (stateStr != "normal" && !stateStr.empty()) {
                cacheKey += "?" + stateStr;
            }

            // 选择第一个变体 (或默认变体)
            if (variantList.variants.empty()) continue;

            const auto& variant = variantList.variants[0];

            // 获取烘焙模型
            auto it = m_bakedModels.find(variant.model);
            if (it == m_bakedModels.end()) continue;

            const auto& bakedModel = it->second;

            // 创建外观
            BlockAppearance appearance;
            appearance.elements = bakedModel.elements;
            appearance.xRotation = variant.x;
            appearance.yRotation = variant.y;
            appearance.uvLock = variant.uvLock;

            // 解析面纹理
            for (const auto& element : bakedModel.elements) {
                for (const auto& [dir, face] : element.faces) {
                    String dirStr = directionToString(dir);
                    ResourceLocation texLoc = bakedModel.resolveTexture(face.texture);

                    // 如果纹理区域已存在，跳过
                    if (appearance.faceTextures.count(dirStr)) continue;

                    // 转换纹理路径为完整的 textures/ 路径
                    ResourceLocation fullTexLoc = texturePathToLocation(texLoc.path());

                    // 查找纹理区域（尝试多种路径变体）
                    const TextureRegion* region = nullptr;

                    // 1. 尝试原始路径
                    auto texIt = m_textureRegions.find(fullTexLoc);
                    if (texIt != m_textureRegions.end()) {
                        region = &texIt->second;
                    }

                    // 2. 尝试不带 textures/ 前缀的原始位置
                    if (!region) {
                        texIt = m_textureRegions.find(texLoc);
                        if (texIt != m_textureRegions.end()) {
                            region = &texIt->second;
                        }
                    }

                    // 3. 尝试 block <-> blocks 路径互换
                    if (!region) {
                        const String& texPath = fullTexLoc.path();
                        String altPath;

                        if (texPath.find("textures/block/") != String::npos) {
                            // MC 1.13+ 路径 -> MC 1.12 路径
                            altPath = texPath;
                            size_t pos = altPath.find("textures/block/");
                            if (pos != String::npos) {
                                altPath.replace(pos, 15, "textures/blocks/");
                                texIt = m_textureRegions.find(ResourceLocation(fullTexLoc.namespace_(), altPath));
                                if (texIt != m_textureRegions.end()) {
                                    region = &texIt->second;
                                }
                            }
                        } else if (texPath.find("textures/blocks/") != String::npos) {
                            // MC 1.12 路径 -> MC 1.13+ 路径
                            altPath = texPath;
                            size_t pos = altPath.find("textures/blocks/");
                            if (pos != String::npos) {
                                altPath.replace(pos, 16, "textures/block/");
                                texIt = m_textureRegions.find(ResourceLocation(fullTexLoc.namespace_(), altPath));
                                if (texIt != m_textureRegions.end()) {
                                    region = &texIt->second;
                                }
                            }
                        }
                    }

                    // 4. 尝试名称变体（wool_colored_white vs white_wool 等）
                    if (!region) {
                        const String& texPath = fullTexLoc.path();
                        auto nameVariants = generateTextureNameVariants(texPath);
                        for (const auto& variantName : nameVariants) {
                            ResourceLocation variantLoc(fullTexLoc.namespace_(), variantName);
                            texIt = m_textureRegions.find(variantLoc);
                            if (texIt != m_textureRegions.end()) {
                                region = &texIt->second;
                                break;
                            }
                        }
                    }

                    if (region) {
                        appearance.faceTextures[dirStr] = *region;
                    }
                }
            }

            totalAppearances++;
            if (!appearance.faceTextures.empty()) {
                appearancesWithTextures++;
            }

            m_blockAppearances[cacheKey] = std::move(appearance);
        }
    }

    spdlog::info("computeBlockAppearances: {} total, {} with textures",
                 totalAppearances, appearancesWithTextures);
}

std::set<ResourceLocation> ResourceManager::collectRequiredTextures() const {
    std::set<ResourceLocation> textures;

    // 从烘焙模型收集纹理
    for (const auto& [loc, model] : m_bakedModels) {
        for (const auto& [name, texLoc] : model.textures) {
            // 跳过纹理变量引用（以 # 开头的值）
            String texPath = texLoc.path();
            if (!texPath.empty() && texPath[0] == '#') {
                // 纹理变量引用，不应该出现在烘焙模型中
                spdlog::warn("Texture variable reference found in baked model {}: {}={}",
                            loc.toString(), name, texPath);
                continue;
            }

            // 转换为纹理路径
            ResourceLocation textureLoc = texturePathToLocation(texPath);
            textures.insert(textureLoc);
        }
    }

    return textures;
}

ResourceLocation ResourceManager::texturePathToLocation(StringView path) {
    // 处理纹理路径
    // 例如: "blocks/stone" -> "minecraft:textures/blocks/stone"
    String p(path);

    // 移除前导的 "#"
    if (!p.empty() && p[0] == '#') {
        p = p.substr(1);
    }

    // 添加 textures/ 前缀
    if (p.find("textures/") != 0 && p.find("textures\\") != 0) {
        p = "textures/" + p;
    }

    return ResourceLocation(p);
}

} // namespace mr
