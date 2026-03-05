#include "ChunkDistanceGraph.hpp"
#include "ChunkLoadTicket.hpp"
#include <cmath>
#include <algorithm>

namespace mr::world {

// ============================================================================
// ChunkDistanceGraph 实现
// ============================================================================

void ChunkDistanceGraph::updateSourceLevel(ChunkCoord x, ChunkCoord z, i32 level, bool isDecreasing) {
    m_updateQueue.emplace(x, z, level, isDecreasing);
}

i32 ChunkDistanceGraph::processUpdates(i32 maxToProcess) {
    i32 processed = 0;

    while (!m_updateQueue.empty() && processed < maxToProcess) {
        auto [x, z, level, isDecreasing] = m_updateQueue.front();
        m_updateQueue.pop();
        ++processed;

        u64 key = posToKey(x, z);
        i32 currentLevel = getLevel(x, z);

        if (isDecreasing) {
            // 级别降低（区块变得更重要）
            if (level < currentLevel) {
                // 更新级别
                m_levels[key] = level;
                onLevelChanged(x, z, currentLevel, level);
                // 传播到邻居
                propagateToNeighbors(x, z, level, true);
            }
        } else {
            // 级别升高（区块变得更不重要）
            if (level > currentLevel) {
                // 需要检查是否有其他源可以提供更低的级别
                i32 sourceLevel = getSourceLevel(x, z);
                i32 newLevel = std::min(level, sourceLevel);

                if (newLevel > currentLevel) {
                    m_levels[key] = newLevel;
                    onLevelChanged(x, z, currentLevel, newLevel);
                    // 传播到邻居
                    propagateToNeighbors(x, z, newLevel, false);
                }
            }
        }
    }

    return processed;
}

i32 ChunkDistanceGraph::getLevel(ChunkCoord x, ChunkCoord z) const {
    u64 key = posToKey(x, z);
    auto it = m_levels.find(key);
    if (it != m_levels.end()) {
        return it->second;
    }
    return MAX_LEVEL;  // 未加载
}

void ChunkDistanceGraph::clear() {
    m_levels.clear();
    while (!m_updateQueue.empty()) {
        m_updateQueue.pop();
    }
}

i32 ChunkDistanceGraph::getSourceLevel(ChunkCoord x, ChunkCoord z) const {
    // 默认实现：检查当前位置是否有票据
    // 子类可以重写此方法以提供票据源
    (void)x;
    (void)z;
    return MAX_LEVEL;
}

void ChunkDistanceGraph::onLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
    if (m_levelChangeCallback) {
        m_levelChangeCallback(x, z, oldLevel, newLevel);
    }
}

void ChunkDistanceGraph::propagateToNeighbors(ChunkCoord x, ChunkCoord z, i32 level, bool isDecreasing) {
    // 4个相邻区块：北、南、东、西
    struct Neighbor { ChunkCoord x, z; };
    Neighbor neighbors[4] = {
        {x, z - 1},  // 北
        {x, z + 1},  // 南
        {x + 1, z},  // 东
        {x - 1, z}   // 西
    };

    i32 propagatedLevel = computePropagatedLevel(level);

    for (i32 i = 0; i < 4; ++i) {
        ChunkCoord nx = neighbors[i].x;
        ChunkCoord nz = neighbors[i].z;

        if (isDecreasing) {
            // 级别降低时，检查邻居是否需要更新
            i32 neighborLevel = getLevel(nx, nz);
            if (propagatedLevel < neighborLevel) {
                m_updateQueue.emplace(nx, nz, propagatedLevel, true);
            }
        } else {
            // 级别升高时，邻居可能需要重新计算
            i32 neighborLevel = getLevel(nx, nz);
            if (neighborLevel > MAX_LEVEL - 2) {
                // 邻居已经是高级别，可能需要卸载
                m_updateQueue.emplace(nx, nz, propagatedLevel, false);
            }
        }
    }
}

// ============================================================================
// PlayerChunkTracker 实现
// ============================================================================

PlayerChunkTracker::PlayerChunkTracker(i32 viewDistance)
    : m_viewDistance(viewDistance)
    , m_ticketLevel(viewDistanceToLevel(viewDistance))
{
    // 初始化视距内的区块集合
    updateChunksInRange();
}

void PlayerChunkTracker::setPlayerPosition(ChunkCoord x, ChunkCoord z) {
    if (m_playerX == x && m_playerZ == z && m_positionSet) {
        return;  // 位置没变
    }

    ChunkCoord oldX = m_playerX;
    ChunkCoord oldZ = m_playerZ;

    m_playerX = x;
    m_playerZ = z;
    m_positionSet = true;

    // 更新视距内的区块
    updateChunksInRange();

    // 更新票据级别
    // 玩家离开旧位置（如果之前有设置过位置）
    if (oldX != x || oldZ != z) {
        // 只有当旧位置与当前位置不同时才需要处理
        updateSourceLevel(oldX, oldZ, MAX_LEVEL, false);
    }

    // 玩家进入新位置
    updateSourceLevel(m_playerX, m_playerZ, m_ticketLevel, true);
}

void PlayerChunkTracker::setViewDistance(i32 distance) {
    if (m_viewDistance == distance) {
        return;
    }

    m_viewDistance = distance;
    m_ticketLevel = viewDistanceToLevel(distance);

    // 更新票据级别
    updateSourceLevel(m_playerX, m_playerZ, m_ticketLevel, true);

    // 更新视距内的区块
    updateChunksInRange();
}

bool PlayerChunkTracker::isChunkInRange(ChunkCoord x, ChunkCoord z) const {
    u64 key = posToKey(x, z);
    return m_chunksInRange.count(key) > 0;
}

i32 PlayerChunkTracker::getSourceLevel(ChunkCoord x, ChunkCoord z) const {
    // 如果是玩家当前位置，返回票据级别
    if (x == m_playerX && z == m_playerZ) {
        return m_ticketLevel;
    }
    // 否则返回最大级别（没有票据）
    return MAX_LEVEL;
}

void PlayerChunkTracker::updateChunksInRange() {
    m_chunksInRange.clear();

    // 遍历视距范围内的所有区块
    for (i32 dx = -m_viewDistance; dx <= m_viewDistance; ++dx) {
        for (i32 dz = -m_viewDistance; dz <= m_viewDistance; ++dz) {
            // 使用切比雪夫距离（棋盘距离）
            i32 dist = std::max(std::abs(dx), std::abs(dz));
            if (dist <= m_viewDistance) {
                ChunkCoord cx = m_playerX + dx;
                ChunkCoord cz = m_playerZ + dz;
                u64 key = posToKey(cx, cz);
                m_chunksInRange.insert(key);
            }
        }
    }
}

} // namespace mr::world
