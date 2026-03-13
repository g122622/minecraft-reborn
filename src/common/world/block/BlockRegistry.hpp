#pragma once

#include "Block.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <algorithm>

namespace mc {

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

        // 对预定义方块使用固定 ID，保证与 BlockId 枚举一致。
        u32 blockId = 0;
        if (auto fixedId = fixedBlockIdForResource(id); fixedId.has_value()) {
            blockId = *fixedId;
        } else {
            blockId = allocateBlockId();
        }
        block->m_blockId = blockId;

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

        // 保证动态分配 ID 始终从已用最大值之后开始。
        m_nextBlockId = std::max(m_nextBlockId, blockId + 1);

        return *ptr;
    }

    /**
     * @brief 根据ID获取方块
     */
    [[nodiscard]] Block* getBlock(u32 blockId) const {
        if (blockId >= m_blocksById.size()) {
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
        while (m_nextBlockId < m_blocksById.size() && m_blocksById[m_nextBlockId] != nullptr) {
            ++m_nextBlockId;
        }
        return m_nextBlockId++;
    }

    [[nodiscard]] std::optional<u32> fixedBlockIdForResource(const ResourceLocation& id) const {
        if (id.namespace_() != "minecraft") {
            return std::nullopt;
        }

        const String& path = id.path();
        if (path == "air") return static_cast<u32>(BlockId::Air);
        if (path == "stone") return static_cast<u32>(BlockId::Stone);
        if (path == "grass_block") return static_cast<u32>(BlockId::Grass);
        if (path == "dirt") return static_cast<u32>(BlockId::Dirt);
        if (path == "cobblestone") return static_cast<u32>(BlockId::Cobblestone);
        if (path == "oak_planks") return static_cast<u32>(BlockId::OakPlanks);
        if (path == "bedrock") return static_cast<u32>(BlockId::Bedrock);
        if (path == "water") return static_cast<u32>(BlockId::Water);
        if (path == "lava") return static_cast<u32>(BlockId::Lava);
        if (path == "sand") return static_cast<u32>(BlockId::Sand);
        if (path == "gravel") return static_cast<u32>(BlockId::Gravel);
        if (path == "gold_ore") return static_cast<u32>(BlockId::GoldOre);
        if (path == "iron_ore") return static_cast<u32>(BlockId::IronOre);
        if (path == "coal_ore") return static_cast<u32>(BlockId::CoalOre);
        if (path == "oak_log") return static_cast<u32>(BlockId::OakLog);
        if (path == "oak_leaves") return static_cast<u32>(BlockId::OakLeaves);
        if (path == "sponge") return static_cast<u32>(BlockId::Sponge);
        if (path == "netherrack") return static_cast<u32>(BlockId::Netherrack);
        if (path == "soul_sand") return static_cast<u32>(BlockId::SoulSand);
        if (path == "glowstone") return static_cast<u32>(BlockId::Glowstone);
        if (path == "end_stone") return static_cast<u32>(BlockId::EndStone);
        if (path == "terracotta") return static_cast<u32>(BlockId::Terracotta);
        if (path == "red_sand") return static_cast<u32>(BlockId::RedSand);
        if (path == "snow") return static_cast<u32>(BlockId::Snow);
        if (path == "ice") return static_cast<u32>(BlockId::Ice);

        return std::nullopt;
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
    u32 m_nextBlockId = 0;
    u32 m_nextStateId = 0;
};

} // namespace mc
