#include "ResourceManager.hpp"
#include "../common/resource/FolderResourcePack.hpp"
#include <spdlog/spdlog.h>

namespace mr {

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

    // 计算方块外观
    computeBlockAppearances();

    spdlog::info("ResourceManager: Loaded {} block states, {} models, {} appearances",
                m_blockStateLoader.getLoadedBlockStates().size(),
                m_bakedModels.size(),
                m_blockAppearances.size());

    return Result<void>::ok();
}

Result<AtlasBuildResult> ResourceManager::buildTextureAtlas() {
    // 收集所需纹理
    auto textures = collectRequiredTextures();

    TextureAtlasBuilder builder;
    builder.setMaxSize(4096, 4096);
    builder.setPadding(0);

    // 添加所有纹理
    for (const auto& texLoc : textures) {
        for (auto& pack : m_resourcePacks) {
            auto result = builder.addTexture(*pack, texLoc);
            if (result.success()) {
                break;
            }
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
                        if (failCount < 5) {
                            spdlog::debug("Failed to bake model '{}': {}",
                                         variant.model.toString(), result.error().toString());
                        }
                        failCount++;
                    }
                }
            }
        }
    }

    if (failCount > 0) {
        spdlog::warn("ResourceManager: Baked {} models ({} failed - missing parent models)",
                     successCount, failCount);
    } else {
        spdlog::info("ResourceManager: Baked {} models successfully", successCount);
    }

    return Result<void>::ok();
}

void ResourceManager::computeBlockAppearances() {
    // 获取所有方块状态
    auto blockStates = m_blockStateLoader.getLoadedBlockStates();

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

                    // 纹理区域将在构建图集后填充
                    // 这里先记录纹理位置
                    if (m_textureRegions.count(texLoc)) {
                        appearance.faceTextures[dirStr] = m_textureRegions[texLoc];
                    }
                }
            }

            m_blockAppearances[cacheKey] = std::move(appearance);
        }
    }
}

std::set<ResourceLocation> ResourceManager::collectRequiredTextures() const {
    std::set<ResourceLocation> textures;

    // 从烘焙模型收集纹理
    for (const auto& [loc, model] : m_bakedModels) {
        for (const auto& [name, texLoc] : model.textures) {
            // 转换为纹理路径
            ResourceLocation textureLoc = texturePathToLocation(texLoc.path());
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
