#pragma once

#include "Block.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

namespace mr {

/**
 * @brief 方块注册表
 *
 * 单例模式，管理所有方块的注册和查找。
 *
 * 参考: net.minecraft.block.Block.Registry
 *
 * 用法示例:
 * @code
 * // 注册方块
 * auto& stone = BlockRegistry::instance().registerBlock<SimpleBlock>(
 *     ResourceLocation("minecraft:stone"),
 *     BlockProperties(Material::ROCK).hardness(1.5f)
 * );
 *
 * // 获取方块
 * Block* block = BlockRegistry::instance().getBlock(
 *     ResourceLocation("minecraft:stone")
 * );
 *
 * // 获取方块状态
 * BlockState* state = BlockRegistry::instance().getBlockState(1);
 * @endcode
 */
class BlockRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static BlockRegistry& instance();

    // 禁止拷贝和移动
    BlockRegistry(const BlockRegistry&) = delete;
    BlockRegistry& operator=(const BlockRegistry&) = delete;

    /**
     * @brief 注册方块
     * @tparam BlockType 方块类型
     * @param id 方块资源位置
     * @param args 方块构造函数参数
     * @return 方块引用
     */
    template<typename BlockType, typename... Args>
    BlockType& registerBlock(const ResourceLocation& id, Args&&... args) {
        auto block = std::make_unique<BlockType>(std::forward<Args>(args)...);
        block->m_blockLocation = id;
        block->m_blockId = allocateBlockId();

        // 注册所有状态
        for (const auto& state : block->stateContainer().validStates()) {
            state->m_blockId = block->m_blockId;
            u32 stateId = allocateStateId();
            const_cast<BlockState*>(state.get())->m_stateId = stateId;
            m_statesById[stateId] = state.get();
        }

        BlockType* ptr = block.get();
        // 确保 vector 大小足够，用 blockId 作为索引
        if (m_blocksById.size() <= block->m_blockId) {
            m_blocksById.resize(block->m_blockId + 1, nullptr);
        }
        m_blocksById[block->m_blockId] = ptr;
        m_blocks[id] = std::move(block);
        return *ptr;
    }

    /**
     * @brief 根据ID获取方块
     */
    [[nodiscard]] Block* getBlock(u32 blockId) const {
        if (blockId == 0 || blockId >= m_blocksById.size()) {
            return nullptr;
        }
        return m_blocksById[blockId];
    }

    /**
     * @brief 根据资源位置获取方块
     */
    [[nodiscard]] Block* getBlock(const ResourceLocation& id) const {
        auto it = m_blocks.find(id);
        return it != m_blocks.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 根据状态ID获取方块状态
     */
    [[nodiscard]] BlockState* getBlockState(u32 stateId) const {
        auto it = m_statesById.find(stateId);
        return it != m_statesById.end() ? it->second : nullptr;
    }

    /**
     * @brief 遍历所有方块
     */
    void forEachBlock(std::function<void(Block&)> callback) {
        for (auto& [id, block] : m_blocks) {
            callback(*block);
        }
    }

    /**
     * @brief 遍历所有方块状态
     */
    void forEachBlockState(std::function<void(const BlockState&)> callback) {
        for (const auto& [id, state] : m_statesById) {
            callback(*state);
        }
    }

    /**
     * @brief 获取方块数量
     */
    [[nodiscard]] size_t blockCount() const {
        return m_blocks.size();
    }

    /**
     * @brief 获取状态数量
     */
    [[nodiscard]] size_t stateCount() const {
        return m_statesById.size();
    }

    /**
     * @brief 根据方块ID获取方块状态（便捷方法）
     * @param blockId 方块ID（枚举值）
     * @return 默认方块状态，如果不存在返回空气状态
     */
    [[nodiscard]] const BlockState* get(BlockId blockId) const {
        Block* block = getBlock(static_cast<u32>(blockId));
        return block ? &block->defaultState() : airState();
    }

    /**
     * @brief 获取空气方块状态
     */
    [[nodiscard]] const BlockState* airState() const {
        auto* air = getBlock(static_cast<u32>(BlockId::Air));
        return air ? &air->defaultState() : nullptr;
    }

private:
    BlockRegistry() = default;

    /**
     * @brief 分配方块ID
     */
    u32 allocateBlockId() {
        return m_nextBlockId++;
    }

    /**
     * @brief 分配状态ID
     */
    u32 allocateStateId() {
        return m_nextStateId++;
    }

    std::unordered_map<ResourceLocation, std::unique_ptr<Block>> m_blocks;
    std::vector<Block*> m_blocksById;
    std::unordered_map<u32, BlockState*> m_statesById;
    u32 m_nextBlockId = 1;  // 0 reserved for air
    u32 m_nextStateId = 1;  // 0 reserved for air state
};

} // namespace mr
