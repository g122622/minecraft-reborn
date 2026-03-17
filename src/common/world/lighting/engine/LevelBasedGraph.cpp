#include "LevelBasedGraph.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

LevelBasedGraph::LevelBasedGraph(i32 levelCount, i32 expectedUpdates, i32 expectedPositions)
    : m_levelCount(levelCount)
    , m_minLevelToUpdate(levelCount)
    , m_needsUpdate(false) {

    // 预分配更新队列
    m_updatesByLevel.resize(static_cast<size_t>(levelCount));
    for (i32 i = 0; i < levelCount; ++i) {
        m_updatesByLevel[static_cast<size_t>(i)].reserve(static_cast<size_t>(expectedUpdates));
    }

    // 预分配位置映射
    m_propagationLevels.reserve(static_cast<size_t>(expectedPositions));
    m_propagationLevels.max_load_factor(0.5f);
}

// ============================================================================
// 公共接口
// ============================================================================

i32 LevelBasedGraph::processUpdates(i32 maxUpdates) {
    while (m_minLevelToUpdate < m_levelCount && maxUpdates > 0) {
        --maxUpdates;

        auto& levelSet = m_updatesByLevel[static_cast<size_t>(m_minLevelToUpdate)];
        if (levelSet.empty()) {
            // 找下一个非空级别
            updateMinLevel(m_levelCount);
            continue;
        }

        // 取出第一个位置
        i64 pos = *levelSet.begin();
        levelSet.erase(levelSet.begin());

        // 更新最小级别
        if (levelSet.empty()) {
            updateMinLevel(m_levelCount);
        }

        // 获取传播级别
        auto it = m_propagationLevels.find(pos);
        if (it == m_propagationLevels.end()) {
            continue;
        }

        u8 propagationLevel = it->second;
        m_propagationLevels.erase(it);

        // 获取当前级别
        i32 currentLevel = getLevel(pos);
        currentLevel = std::clamp(currentLevel, 0, m_levelCount - 1);

        if (propagationLevel < currentLevel) {
            // 级别减少（光照变亮）
            setLevel(pos, propagationLevel);
            notifyNeighbors(pos, propagationLevel, true);
        } else if (propagationLevel > currentLevel) {
            // 级别增加（光照变暗）
            addUpdate(pos, propagationLevel, minLevel(m_levelCount - 1, propagationLevel));
            setLevel(pos, m_levelCount - 1);
            notifyNeighbors(pos, currentLevel, false);
        }
    }

    m_needsUpdate = m_minLevelToUpdate < m_levelCount;
    return maxUpdates;
}

void LevelBasedGraph::scheduleUpdate(i64 pos) {
    scheduleUpdate(pos, pos, m_levelCount - 1, false);
}

void LevelBasedGraph::scheduleUpdate(i64 fromPos, i64 toPos, i32 newLevel, bool isDecreasing) {
    i32 currentLevel = getLevel(toPos);
    currentLevel = std::clamp(currentLevel, 0, m_levelCount - 1);

    i32 propagationLevel = m_propagationLevels.count(toPos) > 0
        ? static_cast<i32>(m_propagationLevels[toPos])
        : currentLevel;

    propagateLevelInternal(fromPos, toPos, newLevel, currentLevel, propagationLevel, isDecreasing);
    m_needsUpdate = m_minLevelToUpdate < m_levelCount;
}

void LevelBasedGraph::propagateLevel(i64 fromPos, i64 toPos, i32 newLevel, bool isDecreasing) {
    i32 propagationLevel = m_propagationLevels.count(toPos) > 0
        ? static_cast<i32>(m_propagationLevels[toPos])
        : INVALID_LEVEL;

    if (isDecreasing) {
        i32 currentLevel = getLevel(toPos);
        currentLevel = std::clamp(currentLevel, 0, m_levelCount - 1);
        propagateLevelInternal(fromPos, toPos, newLevel, currentLevel, propagationLevel, true);
    } else {
        if (propagationLevel == INVALID_LEVEL) {
            i32 currentLevel = getLevel(toPos);
            currentLevel = std::clamp(currentLevel, 0, m_levelCount - 1);

            if (newLevel == currentLevel) {
                propagateLevelInternal(fromPos, toPos, m_levelCount - 1, currentLevel, INVALID_LEVEL, false);
            }
        }
    }
}

void LevelBasedGraph::cancelUpdate(i64 pos) {
    auto it = m_propagationLevels.find(pos);
    if (it != m_propagationLevels.end()) {
        i32 propagationLevel = it->second;
        i32 currentLevel = getLevel(pos);
        i32 level = minLevel(currentLevel, propagationLevel);
        removeUpdate(pos, level, m_levelCount, true);
        m_needsUpdate = m_minLevelToUpdate < m_levelCount;
    }
}

void LevelBasedGraph::cancelUpdates(const std::function<bool(i64)>& predicate) {
    std::vector<i64> toCancel;
    toCancel.reserve(m_propagationLevels.size());

    for (const auto& [pos, level] : m_propagationLevels) {
        if (predicate(pos)) {
            toCancel.push_back(pos);
        }
    }

    for (i64 pos : toCancel) {
        cancelUpdate(pos);
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

i32 LevelBasedGraph::minLevel(i32 level1, i32 level2) const {
    i32 result = level1;
    if (level1 > level2) {
        result = level2;
    }
    if (result > m_levelCount - 1) {
        result = m_levelCount - 1;
    }
    return result;
}

void LevelBasedGraph::updateMinLevel(i32 maxLevel) {
    i32 oldMin = m_minLevelToUpdate;
    m_minLevelToUpdate = maxLevel;

    for (i32 i = oldMin + 1; i < maxLevel; ++i) {
        if (!m_updatesByLevel[static_cast<size_t>(i)].empty()) {
            m_minLevelToUpdate = i;
            break;
        }
    }
}

void LevelBasedGraph::removeUpdate(i64 pos, i32 level, i32 maxLevel, bool removeAll) {
    if (removeAll) {
        m_propagationLevels.erase(pos);
    }

    m_updatesByLevel[static_cast<size_t>(level)].erase(pos);

    if (m_updatesByLevel[static_cast<size_t>(level)].empty() && m_minLevelToUpdate == level) {
        updateMinLevel(maxLevel);
    }
}

void LevelBasedGraph::addUpdate(i64 pos, i32 levelToSet, i32 updateLevel) {
    m_propagationLevels[pos] = static_cast<u8>(levelToSet);
    m_updatesByLevel[static_cast<size_t>(updateLevel)].insert(pos);

    if (m_minLevelToUpdate > updateLevel) {
        m_minLevelToUpdate = updateLevel;
    }
}

void LevelBasedGraph::propagateLevelInternal(i64 fromPos, i64 toPos, i32 newLevel,
                                              i32 previousLevel, i32 propagationLevel,
                                              bool isDecreasing) {
    if (isRoot(toPos)) {
        return;
    }

    newLevel = std::clamp(newLevel, 0, m_levelCount - 1);
    previousLevel = std::clamp(previousLevel, 0, m_levelCount - 1);

    bool isNew = (propagationLevel == INVALID_LEVEL);
    if (isNew) {
        propagationLevel = previousLevel;
    }

    i32 computedLevel;
    if (isDecreasing) {
        computedLevel = std::min(propagationLevel, newLevel);
    } else {
        computedLevel = std::clamp(computeLevel(toPos, fromPos, newLevel), 0, m_levelCount - 1);
    }

    i32 oldUpdateLevel = minLevel(previousLevel, propagationLevel);

    if (previousLevel != computedLevel) {
        i32 newUpdateLevel = minLevel(previousLevel, computedLevel);

        if (oldUpdateLevel != newUpdateLevel && !isNew) {
            removeUpdate(toPos, oldUpdateLevel, newUpdateLevel, false);
        }

        addUpdate(toPos, computedLevel, newUpdateLevel);
    } else if (!isNew) {
        removeUpdate(toPos, oldUpdateLevel, m_levelCount, true);
    }
}

} // namespace mc
