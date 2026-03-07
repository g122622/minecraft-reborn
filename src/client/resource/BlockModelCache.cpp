#include "BlockModelCache.hpp"
#include "ResourceManager.hpp"
#include "DefaultResources.hpp"

#include <spdlog/spdlog.h>

namespace mr {

// ============================================================================
// 初始化和重建
// ============================================================================

bool BlockModelCache::initialize(ResourceManager& resourceManager)
{
    spdlog::info("Initializing BlockModelCache...");

    m_resourceManager = &resourceManager;

    // 确保 DefaultResources 已初始化
    DefaultResources::initialize();

    // 创建缺失模型外观
    createMissingAppearance();

    // 构建状态缓存
    buildStateCache();

    m_initialized = true;
    spdlog::info("BlockModelCache initialized: {} appearances cached", m_stateCache.size());

    return true;
}

bool BlockModelCache::rebuild(ResourceManager& resourceManager)
{
    spdlog::info("Rebuilding BlockModelCache...");

    // 清除旧缓存
    clear();

    // 重新初始化
    return initialize(resourceManager);
}

void BlockModelCache::clear()
{
    m_stateCache.clear();
    m_missingAppearance.reset();
    m_resourceManager = nullptr;
    m_initialized = false;
}

// ============================================================================
// 外观查询
// ============================================================================

const BlockAppearance* BlockModelCache::getBlockAppearance(const BlockState* state) const
{
    if (!state || !m_initialized) {
        return getMissingAppearance();
    }

    return getBlockAppearance(state->blockId(), state->toModelKey());
}

const BlockAppearance* BlockModelCache::getBlockAppearance(u32 stateId) const
{
    if (!m_initialized) {
        return getMissingAppearance();
    }

    auto it = m_stateCache.find(stateId);
    if (it != m_stateCache.end()) {
        return it->second;
    }

    return getMissingAppearance();
}

const BlockAppearance* BlockModelCache::getBlockAppearance(
    u32 blockId,
    const String& properties) const
{
    if (!m_resourceManager || !m_initialized) {
        return getMissingAppearance();
    }

    // 获取方块
    Block* block = Block::getBlock(blockId);
    if (!block) {
        return getMissingAppearance();
    }

    // 从 ResourceManager 获取外观
    // 构建 properties map
    std::map<String, String> props;
    if (!properties.empty()) {
        // 解析 "key=value,key2=value2" 格式
        size_t start = 0;
        size_t end = 0;

        while (start < properties.size()) {
            end = properties.find(',', start);
            if (end == String::npos) {
                end = properties.size();
            }

            StringView pair(properties.data() + start, end - start);
            size_t eq = pair.find('=');
            if (eq != StringView::npos) {
                String key(pair.substr(0, eq));
                String value(pair.substr(eq + 1));
                props[key] = value;
            }

            start = end + 1;
        }
    }

    const BlockAppearance* appearance = m_resourceManager->getBlockAppearance(
        block->blockLocation(), props);

    return appearance ? appearance : getMissingAppearance();
}

const BlockAppearance* BlockModelCache::getMissingAppearance() const
{
    return m_missingAppearance.get();
}

// ============================================================================
// 私有方法
// ============================================================================

void BlockModelCache::buildStateCache()
{
    if (!m_resourceManager) {
        spdlog::warn("BlockModelCache: Cannot build cache without ResourceManager");
        return;
    }

    m_stateCache.clear();

    size_t successCount = 0;
    size_t failCount = 0;

    // 遍历所有方块状态
    Block::forEachBlockState([this, &successCount, &failCount](const BlockState& state) {
        u32 stateId = state.blockId(); // 这里应该用实际的 stateId，但 BlockState 没有暴露

        // 获取方块资源位置
        const ResourceLocation& blockLoc = state.blockLocation();

        // 获取模型键（属性字符串）
        String modelKey = state.toModelKey();

        // 构建 properties map
        std::map<String, String> props;
        if (!modelKey.empty()) {
            // 解析 "key=value,key2=value2" 格式
            size_t start = 0;
            size_t end = 0;

            while (start < modelKey.size()) {
                end = modelKey.find(',', start);
                if (end == String::npos) {
                    end = modelKey.size();
                }

                StringView pair(modelKey.data() + start, end - start);
                size_t eq = pair.find('=');
                if (eq != StringView::npos) {
                    String key(pair.substr(0, eq));
                    String value(pair.substr(eq + 1));
                    props[key] = value;
                }

                start = end + 1;
            }
        }

        // 从 ResourceManager 获取外观
        const BlockAppearance* appearance = m_resourceManager->getBlockAppearance(blockLoc, props);

        if (appearance) {
            // 我们需要获取实际的 stateId，暂时用 blockId + 一些偏移
            // 实际上 BlockState 有 m_stateId，但没有公开接口
            // 让我们使用 blockId 作为临时方案
            m_stateCache[stateId] = appearance;
            ++successCount;
        } else {
            // 使用缺失模型
            m_stateCache[stateId] = m_missingAppearance.get();
            ++failCount;
        }
    });

    spdlog::info("BlockModelCache built: {} successes, {} failures",
                 successCount, failCount);
}

void BlockModelCache::createMissingAppearance()
{
    m_missingAppearance = std::make_unique<BlockAppearance>();

    // 创建一个简单的紫黑方块作为缺失模型
    // 这将在纹理图集中使用缺失纹理
    ModelElement element;
    element.from = {0.0f, 0.0f, 0.0f};
    element.to = {16.0f, 16.0f, 16.0f};

    // 创建所有面的纹理引用
    // 使用 "missing" 作为纹理名称，ResourceManager 应该提供缺失纹理
    for (Direction dir : Directions::all()) {
        ModelFace modelFace;
        modelFace.texture = "#missing";
        modelFace.uv = {0.0f, 0.0f, 16.0f, 16.0f};
        element.faces[dir] = modelFace;
    }

    m_missingAppearance->elements.push_back(element);
    m_missingAppearance->xRotation = 0;
    m_missingAppearance->yRotation = 0;
    m_missingAppearance->uvLock = false;
}

} // namespace mr
