#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/util/math/Vector3.hpp"
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace mc {
namespace client {
namespace renderer {
namespace trident {
namespace block {

/**
 * @brief 单个方块的破坏进度状态
 *
 * 存储某个玩家在某个方块上的挖掘进度。
 * 参考 MC 1.16.5 DestroyBlockProgress
 */
struct BlockBreakProgress {
    /// 挖掘者的实体ID
    EntityId breakerId;

    /// 方块位置
    BlockPos position;

    /// 破坏阶段 (0-9)
    /// 0 = 刚开始, 9 = 即将破坏
    u8 damageStage = 0;

    /// 创建时的游戏tick（用于超时清理）
    u64 creationTick = 0;

    /// 最后更新的tick
    u64 lastUpdateTick = 0;
};

/**
 * @brief 客户端挖掘进度管理器
 *
 * 管理所有可见的方块破坏进度状态。
 * 支持本地玩家的挖掘进度和其他玩家的挖掘进度（多人游戏）。
 *
 * 数据结构设计：
 * - m_localProgress: 本地玩家的挖掘进度（客户端计算）
 * - m_remoteProgressByEntity: 按实体ID索引（快速更新/移除）
 * - m_remoteProgressByPos: 按位置索引（快速渲染查询）
 *
 * 参考 MC 1.16.5 WorldRenderer.damagedBlocks / damageProgress
 */
class BreakProgressManager {
public:
    /// 破坏阶段数量
    static constexpr size_t MAX_DAMAGE_STAGE = 9;

    /// 进度超时时间（tick），超过此时间自动清理
    static constexpr u64 PROGRESS_TIMEOUT_TICKS = 400; // 20秒

    /// 最大渲染距离（方块）
    static constexpr f32 MAX_RENDER_DISTANCE_SQ = 1024.0f; // 32格

    /**
     * @brief 获取单例实例
     */
    static BreakProgressManager& instance();

    /**
     * @brief 初始化管理器
     */
    void initialize();

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 每帧更新
     *
     * 更新本地挖掘进度，清理超时的远程进度。
     *
     * @param deltaTime 帧间隔时间（秒）
     * @param currentTick 当前游戏tick
     */
    void tick(f32 deltaTime, u64 currentTick);

    // ========================================================================
    // 本地玩家挖掘进度
    // ========================================================================

    /**
     * @brief 开始挖掘方块
     *
     * @param pos 方块位置
     */
    void startBreaking(const BlockPos& pos);

    /**
     * @brief 更新本地挖掘进度
     *
     * @param pos 方块位置
     * @param progress 进度值 (0.0 - 1.0)
     * @return 计算后的破坏阶段 (0-9)
     */
    u8 updateLocalProgress(const BlockPos& pos, f32 progress);

    /**
     * @brief 停止挖掘（中断或完成）
     */
    void stopBreaking();

    /**
     * @brief 检查是否正在挖掘
     */
    [[nodiscard]] bool isBreaking() const { return m_localBreaking; }

    /**
     * @brief 获取本地挖掘位置
     */
    [[nodiscard]] const BlockPos& getLocalBreakPos() const { return m_localBreakPos; }

    /**
     * @brief 获取本地挖掘进度
     */
    [[nodiscard]] f32 getLocalProgress() const { return m_localProgress; }

    /**
     * @brief 获取本地破坏阶段
     */
    [[nodiscard]] u8 getLocalDamageStage() const { return m_localDamageStage; }

    // ========================================================================
    // 远程玩家挖掘进度（多人游戏）
    // ========================================================================

    /**
     * @brief 更新远程玩家的挖掘进度
     *
     * 从服务端收到 BlockBreakAnimPacket 时调用。
     *
     * @param breakerId 挖掘者实体ID
     * @param pos 方块位置
     * @param stage 破坏阶段 (0-9)，-1 表示移除
     * @param currentTick 当前tick
     */
    void updateRemoteProgress(EntityId breakerId, const BlockPos& pos,
                               i8 stage, u64 currentTick);

    /**
     * @brief 移除远程玩家的挖掘进度
     *
     * @param breakerId 挖掘者实体ID
     */
    void removeRemoteProgress(EntityId breakerId);

    /**
     * @brief 清除所有远程挖掘进度
     */
    void clearRemoteProgress();

    // ========================================================================
    // 查询接口
    // ========================================================================

    /**
     * @brief 获取指定位置的破坏阶段
     *
     * 如果有多个玩家在同一位置挖掘，返回最大阶段。
     *
     * @param pos 方块位置
     * @return 破坏阶段 (0-9)，无进度返回 255
     */
    [[nodiscard]] u8 getDamageStage(const BlockPos& pos) const;

    /**
     * @brief 获取指定位置的所有破坏进度
     *
     * @param pos 方块位置
     * @return 破坏进度列表
     */
    [[nodiscard]] std::vector<const BlockBreakProgress*>
    getProgressAtPos(const BlockPos& pos) const;

    /**
     * @brief 获取所有需要渲染的破坏进度
     *
     * 过滤掉超出渲染距离的进度。
     *
     * @param cameraPos 摄像机位置
     * @return 破坏进度列表（位置，阶段）
     */
    [[nodiscard]] std::vector<std::pair<BlockPos, u8>>
    getVisibleProgress(const Vector3& cameraPos) const;

    /**
     * @brief 检查指定位置是否有破坏进度
     */
    [[nodiscard]] bool hasProgressAt(const BlockPos& pos) const;

private:
    BreakProgressManager() = default;
    ~BreakProgressManager() = default;

    // 禁止拷贝
    BreakProgressManager(const BreakProgressManager&) = delete;
    BreakProgressManager& operator=(const BreakProgressManager&) = delete;

    /**
     * @brief 清理超时的远程进度
     */
    void cleanupStaleProgress(u64 currentTick);

    /**
     * @brief 更新位置索引
     */
    void updatePositionIndex(const BlockBreakProgress& progress);

    /**
     * @brief 从位置索引移除
     */
    void removeFromPositionIndex(const BlockPos& pos, EntityId breakerId);

    // ========================================================================
    // 本地玩家状态
    // ========================================================================

    /// 是否正在挖掘
    bool m_localBreaking = false;

    /// 挖掘位置
    BlockPos m_localBreakPos;

    /// 挖掘进度 (0.0 - 1.0)
    f32 m_localProgress = 0.0f;

    /// 破坏阶段 (0-9)
    u8 m_localDamageStage = 0;

    // ========================================================================
    // 远程玩家状态（多人游戏）
    // ========================================================================

    /// 按实体ID索引的破坏进度
    std::unordered_map<EntityId, BlockBreakProgress> m_remoteProgressByEntity;

    /// 按位置索引的破坏进度（支持同一位置多个玩家挖掘）
    std::unordered_map<BlockPos, std::vector<EntityId>> m_remoteProgressByPos;

    /// 当前tick
    u64 m_currentTick = 0;
};

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
