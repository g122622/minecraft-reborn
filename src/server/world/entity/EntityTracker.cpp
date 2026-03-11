#include "EntityTracker.hpp"
#include "../ServerWorld.hpp"
#include "../../../common/network/EntityPackets.hpp"
#include "../../../common/network/PacketSerializer.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace mr::server {

EntityTracker::EntityTracker()
    : m_trackingDistance(10)
    , m_positionUpdateThreshold(0.1f)
    , m_rotationUpdateThreshold(1.0f)
{
}

void EntityTracker::trackEntity(Entity* entity) {
    if (!entity) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    EntityId entityId = entity->id();
    if (m_trackedEntities.find(entityId) != m_trackedEntities.end()) {
        return;  // 已经在追踪
    }

    TrackedEntity tracked;
    tracked.entityId = entityId;
    tracked.lastPosition = entity->position();
    tracked.lastYaw = entity->yaw();
    tracked.lastPitch = entity->pitch();
    tracked.needsFullUpdate = true;

    m_trackedEntities[entityId] = tracked;

    spdlog::debug("EntityTracker: Started tracking entity {}", entityId);
}

void EntityTracker::untrackEntity(EntityId entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_trackedEntities.find(entityId);
    if (it == m_trackedEntities.end()) {
        return;
    }

    // 通知所有正在追踪此实体的玩家
    for (PlayerId playerId : it->second.trackingPlayers) {
        auto playerIt = m_playerTrackedEntities.find(playerId);
        if (playerIt != m_playerTrackedEntities.end()) {
            playerIt->second.erase(entityId);
        }
    }

    m_trackedEntities.erase(it);
    spdlog::debug("EntityTracker: Stopped tracking entity {}", entityId);
}

bool EntityTracker::isTracking(EntityId entityId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trackedEntities.find(entityId) != m_trackedEntities.end();
}

size_t EntityTracker::trackedEntityCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trackedEntities.size();
}

void EntityTracker::updatePlayerTracking(ServerWorld& world, PlayerId playerId, const Vector3& playerPos) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 获取玩家当前追踪的实体集合
    auto& trackedSet = m_playerTrackedEntities[playerId];
    std::vector<EntityId> toStartTracking;
    std::vector<EntityId> toStopTracking;

    // 检查所有被追踪的实体
    for (auto& [entityId, tracked] : m_trackedEntities) {
        // TODO: 从 ServerWorld 获取实体
        // Entity* entity = world.getEntity(entityId);
        // if (!entity) continue;

        bool shouldTrack = true; // 暂时假设应该追踪

        bool isTracking = trackedSet.find(entityId) != trackedSet.end();

        if (shouldTrack && !isTracking) {
            toStartTracking.push_back(entityId);
        } else if (!shouldTrack && isTracking) {
            toStopTracking.push_back(entityId);
        }
    }

    // 开始追踪新实体
    for (EntityId entityId : toStartTracking) {
        // TODO: 发送生成包
        trackedSet.insert(entityId);
        m_trackedEntities[entityId].trackingPlayers.insert(playerId);
    }

    // 停止追踪实体
    for (EntityId entityId : toStopTracking) {
        // TODO: 发送销毁包
        trackedSet.erase(entityId);
        m_trackedEntities[entityId].trackingPlayers.erase(playerId);
    }
}

void EntityTracker::removePlayer(PlayerId playerId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto trackedSet = m_playerTrackedEntities.find(playerId);
    if (trackedSet == m_playerTrackedEntities.end()) {
        return;
    }

    // 从所有实体的追踪玩家列表中移除此玩家
    for (EntityId entityId : trackedSet->second) {
        auto it = m_trackedEntities.find(entityId);
        if (it != m_trackedEntities.end()) {
            it->second.trackingPlayers.erase(playerId);
        }
    }

    m_playerTrackedEntities.erase(trackedSet);
    spdlog::debug("EntityTracker: Removed player {} from tracking", playerId);
}

std::vector<EntityId> EntityTracker::getPlayerTrackedEntities(PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<EntityId> result;
    auto it = m_playerTrackedEntities.find(playerId);
    if (it != m_playerTrackedEntities.end()) {
        result.reserve(it->second.size());
        for (EntityId entityId : it->second) {
            result.push_back(entityId);
        }
    }
    return result;
}

void EntityTracker::tick(ServerWorld& /*world*/) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 更新所有被追踪实体的位置
    for (auto& [entityId, tracked] : m_trackedEntities) {
        // TODO: 从 ServerWorld 获取实体并检查位置变化
        // Entity* entity = world.getEntity(entityId);
        // if (!entity || entity->isRemoved()) {
        //     continue;
        // }

        tracked.updateCounter++;
    }
}

bool EntityTracker::shouldTrack(const Vector3& playerPos, const Vector3& entityPos, i32 trackingRange) const {
    f32 dx = playerPos.x - entityPos.x;
    f32 dz = playerPos.z - entityPos.z;
    f32 distanceSq = dx * dx + dz * dz;

    f32 rangeBlocks = static_cast<f32>(trackingRange * 16);
    return distanceSq <= rangeBlocks * rangeBlocks;
}

void EntityTracker::sendSpawnPacket(ServerWorld& /*world*/, PlayerId /*playerId*/, Entity* /*entity*/) {
    // TODO: 实现
}

void EntityTracker::sendDestroyPacket(ServerWorld& /*world*/, PlayerId /*playerId*/, EntityId /*entityId*/) {
    // TODO: 实现
}

void EntityTracker::sendMovePacket(ServerWorld& /*world*/, PlayerId /*playerId*/, Entity* /*entity*/) {
    // TODO: 实现
}

} // namespace mr::server
