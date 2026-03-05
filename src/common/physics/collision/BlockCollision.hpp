#pragma once

#include "CollisionShape.hpp"
#include "../../world/BlockID.hpp"
#include <array>
#include <unordered_map>
#include <functional>

namespace mr {

/**
 * @brief 方块碰撞注册表
 *
 * 管理所有方块类型的碰撞形状。
 * 支持状态相关的形状（如门、楼梯等）。
 *
 * 使用方法：
 * 1. 调用 initialize() 初始化所有原版方块
 * 2. 使用 getShape() 获取方块的碰撞形状
 * 3. 使用 registerShape() 注册自定义方块的碰撞形状
 *
 * 注意：
 * - 碰撞形状使用方块本地坐标（0-1范围）
 * - 形状工厂用于状态相关的方块（如楼梯、门等）
 * - 空形状用于无碰撞方块（空气、水、岩浆等）
 */
class BlockCollisionRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static BlockCollisionRegistry& instance();

    /**
     * @brief 初始化注册表，注册所有原版方块的碰撞形状
     *
     * 此方法只会执行一次，后续调用将被忽略。
     */
    void initialize();

    /**
     * @brief 注册方块碰撞形状
     * @param blockId 方块ID
     * @param shape 碰撞形状
     */
    void registerShape(BlockId blockId, const CollisionShape& shape);

    /**
     * @brief 注册完整方块碰撞形状
     * @param blockId 方块ID
     */
    void registerFullBlock(BlockId blockId);

    /**
     * @brief 注册空碰撞形状（无碰撞）
     * @param blockId 方块ID
     */
    void registerEmpty(BlockId blockId);

    /**
     * @brief 注册状态相关的形状工厂
     * @param blockId 方块ID
     * @param factory 形状工厂函数，根据方块状态返回碰撞形状
     */
    void registerShapeFactory(BlockId blockId,
                               std::function<CollisionShape(BlockState)> factory);

    /**
     * @brief 获取方块的碰撞形状
     * @param state 方块状态
     * @return 碰撞形状（如果未注册，返回空形状）
     *
     * 注意：此方法优先查找形状工厂，如果没有则返回预注册的形状。
     */
    [[nodiscard]] CollisionShape getShape(BlockState state) const;

    /**
     * @brief 获取方块的碰撞形状（仅根据方块ID）
     * @param blockId 方块ID
     * @return 碰撞形状
     */
    [[nodiscard]] CollisionShape getShapeById(BlockId blockId) const;

    /**
     * @brief 检查方块是否有碰撞
     * @param state 方块状态
     * @return 是否有碰撞
     */
    [[nodiscard]] bool hasCollision(BlockState state) const;

    // 缓存的常用形状
    [[nodiscard]] const CollisionShape& emptyShape() const noexcept { return m_emptyShape; }
    [[nodiscard]] const CollisionShape& fullBlockShape() const noexcept { return m_fullBlockShape; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

private:
    BlockCollisionRegistry();
    ~BlockCollisionRegistry() = default;

    // 禁止拷贝和移动
    BlockCollisionRegistry(const BlockCollisionRegistry&) = delete;
    BlockCollisionRegistry& operator=(const BlockCollisionRegistry&) = delete;

    void registerVanillaBlocks();

    std::array<CollisionShape, static_cast<size_t>(BlockId::MaxBlocks)> m_shapes;

    // 状态相关的形状工厂（用于门、楼梯等）
    std::unordered_map<BlockId, std::function<CollisionShape(BlockState)>> m_shapeFactories;

    CollisionShape m_emptyShape;       // 缓存的空形状
    CollisionShape m_fullBlockShape;   // 缓存的完整方块形状

    bool m_initialized = false;
};

} // namespace mr
