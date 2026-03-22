#include "BreakProgressManager.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace mc {
namespace client {
namespace renderer {
namespace trident {
namespace block {

// ============================================================================
// 静态成员
// ============================================================================

BreakProgressManager& BreakProgressManager::instance() {
    static BreakProgressManager instance;
    return instance;
}

// ============================================================================
// 初始化
// ============================================================================

void BreakProgressManager::initialize() {
    spdlog::info("BreakProgressManager: Initialized");
}

void BreakProgressManager::cleanup() {
    m_localBreaking = false;
    m_localProgress = 0.0f;
    m_localDamageStage = 0;
    m_remoteProgressByEntity.clear();
    m_remoteProgressByPos.clear();
    spdlog::info("BreakProgressManager: Cleaned up");
}

// ============================================================================
// 每帧更新
// ============================================================================

void BreakProgressManager::tick(f32 deltaTime, u64 currentTick) {
    m_currentTick = currentTick;

    // 清理超时的远程进度
    cleanupStaleProgress(currentTick);
}

// ============================================================================
// 本地玩家挖掘进度
// ============================================================================

void BreakProgressManager::startBreaking(const BlockPos& pos) {
    m_localBreaking = true;
    m_localBreakPos = pos;
    m_localProgress = 0.0f;
    m_localDamageStage = 0;

    spdlog::debug("BreakProgressManager: Started breaking at ({}, {}, {})",
                  pos.x, pos.y, pos.z);
}

u8 BreakProgressManager::updateLocalProgress(const BlockPos& pos, f32 progress) {
    // 检查位置是否匹配
    if (!m_localBreaking || m_localBreakPos != pos) {
        // 位置不匹配，可能是新的挖掘
        startBreaking(pos);
    }

    m_localProgress = std::clamp(progress, 0.0f, 1.0f);

    // 计算破坏阶段：progress * 10 - 1
    // progress = 0.0 -> stage = 0
    // progress = 0.1 -> stage = 0
    // progress = 0.2 -> stage = 1
    // ...
    // progress = 1.0 -> stage = 9
    u8 newStage = static_cast<u8>(std::min(9.0f, progress * 10.0f));

    if (newStage != m_localDamageStage) {
        m_localDamageStage = newStage;
        spdlog::trace("BreakProgressManager: Damage stage updated to {}", newStage);
    }

    return m_localDamageStage;
}

void BreakProgressManager::stopBreaking() {
    if (m_localBreaking) {
        spdlog::debug("BreakProgressManager: Stopped breaking at ({}, {}, {})",
                      m_localBreakPos.x, m_localBreakPos.y, m_localBreakPos.z);
    }
    m_localBreaking = false;
    m_localProgress = 0.0f;
    m_localDamageStage = 0;
}

// ============================================================================
// 远程玩家挖掘进度
// ============================================================================

void BreakProgressManager::updateRemoteProgress(EntityId breakerId, const BlockPos& pos,
                                                 i8 stage, u64 currentTick) {
    if (stage < 0 || stage > static_cast<i8>(MAX_DAMAGE_STAGE)) {
        // 无效阶段，移除进度
        removeRemoteProgress(breakerId);
        return;
    }

    // 查找或创建进度条目
    auto it = m_remoteProgressByEntity.find(breakerId);
    if (it == m_remoteProgressByEntity.end()) {
        // 新建进度
        BlockBreakProgress progress;
        progress.breakerId = breakerId;
        progress.position = pos;
        progress.damageStage = static_cast<u8>(stage);
        progress.creationTick = currentTick;
        progress.lastUpdateTick = currentTick;

        m_remoteProgressByEntity[breakerId] = progress;
        updatePositionIndex(progress);

        spdlog::debug("BreakProgressManager: Created remote progress for entity {} at ({}, {}, {}), stage {}",
                      breakerId, pos.x, pos.y, pos.z, stage);
    } else {
        // 更新现有进度
        BlockPos oldPos = it->second.position;

        // 如果位置改变，需要更新位置索引
        if (oldPos != pos) {
            removeFromPositionIndex(oldPos, breakerId);
            it->second.position = pos;
            updatePositionIndex(it->second);
        }

        it->second.damageStage = static_cast<u8>(stage);
        it->second.lastUpdateTick = currentTick;

        spdlog::trace("BreakProgressManager: Updated remote progress for entity {}, stage {}",
                      breakerId, stage);
    }
}

void BreakProgressManager::removeRemoteProgress(EntityId breakerId) {
    auto it = m_remoteProgressByEntity.find(breakerId);
    if (it != m_remoteProgressByEntity.end()) {
        BlockPos pos = it->second.position;
        removeFromPositionIndex(pos, breakerId);
        m_remoteProgressByEntity.erase(it);

        spdlog::debug("BreakProgressManager: Removed remote progress for entity {}", breakerId);
    }
}

void BreakProgressManager::clearRemoteProgress() {
    m_remoteProgressByEntity.clear();
    m_remoteProgressByPos.clear();
}

// ============================================================================
// 查询接口
// ============================================================================

u8 BreakProgressManager::getDamageStage(const BlockPos& pos) const {
    u8 maxStage = 255;

    // 检查本地进度
    if (m_localBreaking && m_localBreakPos == pos) {
        maxStage = std::max(maxStage, m_localDamageStage);
    }

    // 检查远程进度
    auto posIt = m_remoteProgressByPos.find(pos);
    if (posIt != m_remoteProgressByPos.end()) {
        for (EntityId breakerId : posIt->second) {
            auto entityIt = m_remoteProgressByEntity.find(breakerId);
            if (entityIt != m_remoteProgressByEntity.end()) {
                if (maxStage == 255 || entityIt->second.damageStage > maxStage) {
                    maxStage = entityIt->second.damageStage;
                }
            }
        }
    }

    return maxStage;
}

std::vector<const BlockBreakProgress*>
BreakProgressManager::getProgressAtPos(const BlockPos& pos) const {
    std::vector<const BlockBreakProgress*> result;

    // 检查远程进度
    auto posIt = m_remoteProgressByPos.find(pos);
    if (posIt != m_remoteProgressByPos.end()) {
        for (EntityId breakerId : posIt->second) {
            auto entityIt = m_remoteProgressByEntity.find(breakerId);
            if (entityIt != m_remoteProgressByEntity.end()) {
                result.push_back(&entityIt->second);
            }
        }
    }

    return result;
}

std::vector<std::pair<BlockPos, u8>>
BreakProgressManager::getVisibleProgress(const Vector3& cameraPos) const {
    std::vector<std::pair<BlockPos, u8>> result;

    // 添加本地进度
    if (m_localBreaking) {
        f32 dx = static_cast<f32>(m_localBreakPos.x) - cameraPos.x;
        f32 dy = static_cast<f32>(m_localBreakPos.y) - cameraPos.y;
        f32 dz = static_cast<f32>(m_localBreakPos.z) - cameraPos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= MAX_RENDER_DISTANCE_SQ) {
            result.emplace_back(m_localBreakPos, m_localDamageStage);
        }
    }

    // 添加远程进度
    for (const auto& [breakerId, progress] : m_remoteProgressByEntity) {
        f32 dx = static_cast<f32>(progress.position.x) - cameraPos.x;
        f32 dy = static_cast<f32>(progress.position.y) - cameraPos.y;
        f32 dz = static_cast<f32>(progress.position.z) - cameraPos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= MAX_RENDER_DISTANCE_SQ) {
            // 避免重复添加同一位置（取最大阶段）
            bool found = false;
            for (auto& [pos, stage] : result) {
                if (pos == progress.position) {
                    stage = std::max(stage, progress.damageStage);
                    found = true;
                    break;
                }
            }
            if (!found) {
                result.emplace_back(progress.position, progress.damageStage);
            }
        }
    }

    return result;
}

bool BreakProgressManager::hasProgressAt(const BlockPos& pos) const {
    // 检查本地进度
    if (m_localBreaking && m_localBreakPos == pos) {
        return true;
    }

    // 检查远程进度
    return m_remoteProgressByPos.find(pos) != m_remoteProgressByPos.end();
}

// ============================================================================
// 私有方法
// ============================================================================

void BreakProgressManager::cleanupStaleProgress(u64 currentTick) {
    std::vector<EntityId> toRemove;

    for (const auto& [breakerId, progress] : m_remoteProgressByEntity) {
        if (currentTick - progress.lastUpdateTick > PROGRESS_TIMEOUT_TICKS) {
            toRemove.push_back(breakerId);
        }
    }

    for (EntityId breakerId : toRemove) {
        removeRemoteProgress(breakerId);
    }
}

void BreakProgressManager::updatePositionIndex(const BlockBreakProgress& progress) {
    auto& entityList = m_remoteProgressByPos[progress.position];

    // 检查是否已存在
    for (EntityId id : entityList) {
        if (id == progress.breakerId) {
            return;
        }
    }

    entityList.push_back(progress.breakerId);
}

void BreakProgressManager::removeFromPositionIndex(const BlockPos& pos, EntityId breakerId) {
    auto posIt = m_remoteProgressByPos.find(pos);
    if (posIt != m_remoteProgressByPos.end()) {
        auto& entityList = posIt->second;
        entityList.erase(
            std::remove(entityList.begin(), entityList.end(), breakerId),
            entityList.end()
        );

        // 如果列表为空，移除位置索引
        if (entityList.empty()) {
            m_remoteProgressByPos.erase(posIt);
        }
    }
}

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
