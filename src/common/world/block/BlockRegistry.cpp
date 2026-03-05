#include "BlockRegistry.hpp"
#include <mutex>
#include <stdexcept>

namespace mr {

namespace {
    std::mutex g_registryMutex;
}

BlockRegistry& BlockRegistry::instance() {
    static BlockRegistry instance;
    return instance;
}

BlockRegistry::BlockRegistry() {
    // 预留空间
    m_blocksById.reserve(4096);
    m_blocksById.push_back(nullptr);  // ID 0 预留给空气
    m_statesById.reserve(65536);
    m_statesById.push_back(nullptr);  // 状态ID 0 预留给空气
}

u32 BlockRegistry::allocateBlockId() {
    if (m_nextBlockId >= 65536) {
        throw std::runtime_error("Block ID overflow: maximum 65536 blocks allowed");
    }
    return m_nextBlockId++;
}

u32 BlockRegistry::allocateStateId(u32 count) {
    if (m_nextStateId + count > 16777216) {  // 2^24
        throw std::runtime_error("State ID overflow: maximum 16777216 states allowed");
    }
    u32 startId = m_nextStateId;
    m_nextStateId += count;
    return startId;
}

Block* BlockRegistry::getBlock(u32 blockId) const {
    if (blockId >= m_blocksById.size()) {
        return nullptr;
    }
    return m_blocksById[blockId];
}

Block* BlockRegistry::getBlock(const ResourceLocation& id) const {
    auto it = m_blocks.find(id);
    return it != m_blocks.end() ? it->second.get() : nullptr;
}

BlockState* BlockRegistry::getBlockState(u32 stateId) const {
    if (stateId >= m_statesById.size()) {
        return nullptr;
    }
    return m_statesById[stateId];
}

void BlockRegistry::forEachBlock(std::function<void(Block&)> callback) {
    for (auto& [id, block] : m_blocks) {
        if (block) {
            callback(*block);
        }
    }
}

void BlockRegistry::forEachBlockState(std::function<void(const BlockState&)> callback) {
    for (auto* state : m_statesById) {
        if (state) {
            callback(*state);
        }
    }
}

void BlockRegistry::initializeVanillaBlocks() {
    if (m_initialized) {
        return;
    }

    // 原版方块将在 blocks/VanillaBlocks.cpp 中注册
    // 这里只设置初始化标志
    m_initialized = true;
}

void BlockRegistry::clear() {
    m_blocks.clear();
    m_blocksById.clear();
    m_statesById.clear();
    m_blocksById.push_back(nullptr);  // ID 0 预留给空气
    m_statesById.push_back(nullptr);  // 状态ID 0 预留给空气
    m_nextBlockId = 1;
    m_nextStateId = 1;
    m_initialized = false;
}

// 内部注册函数（不暴露给头文件）
namespace detail {

void registerBlockInternal(BlockRegistry& registry,
                          std::unique_ptr<Block> block,
                          const ResourceLocation& id) {
    // 分配方块ID
    u32 blockId = registry.m_nextBlockId++;

    // 计算状态数量
    size_t stateCount = block->stateCount();

    // 分配状态ID范围
    u32 startStateId = registry.m_nextStateId;
    registry.m_nextStateId += static_cast<u32>(stateCount);

    // 设置方块属性
    block->setBlockId(blockId);
    block->setBlockLocation(id);

    // 扩展状态映射
    if (registry.m_statesById.size() <= startStateId + stateCount) {
        registry.m_statesById.resize(startStateId + stateCount + 1, nullptr);
    }

    // 扩展方块映射
    if (registry.m_blocksById.size() <= blockId) {
        registry.m_blocksById.resize(blockId + 1, nullptr);
    }

    // 注册所有状态
    const auto& states = block->validStates();
    for (size_t i = 0; i < states.size(); ++i) {
        // 注意：这里我们假设状态ID已经在StateContainer中正确设置
        // 实际的状态ID应该是 startStateId + i
        u32 stateId = startStateId + static_cast<u32>(i);
        registry.m_statesById[stateId] = const_cast<BlockState*>(states[i].get());
    }

    // 存储方块
    registry.m_blocksById[blockId] = block.get();
    registry.m_blocks[id] = std::move(block);
}

} // namespace detail

} // namespace mr
