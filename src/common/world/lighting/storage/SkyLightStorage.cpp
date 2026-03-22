#include "SkyLightStorage.hpp"
#include "../engine/LightEngineUtils.hpp"
#include <climits>

namespace mc {

// ============================================================================
// SkyLightDataMap 实现
// ============================================================================

SkyLightDataMap::SkyLightDataMap()
    : LightDataMap()
    , m_minY(INT_MAX)
    , m_surfaceSections() {
}

SkyLightDataMap::SkyLightDataMap(
    std::unordered_map<i64, NibbleArray> arrays,
    std::unordered_map<i64, i32> surfaceSections,
    i32 minY)
    : LightDataMap(std::move(arrays))
    , m_minY(minY)
    , m_surfaceSections(std::move(surfaceSections)) {
}

SkyLightDataMap SkyLightDataMap::copy() const {
    std::unordered_map<i64, NibbleArray> copiedArrays;
    copiedArrays.reserve(m_arrays.size());
    for (const auto& [pos, array] : m_arrays) {
        copiedArrays[pos] = array.copy();
    }
    return SkyLightDataMap(std::move(copiedArrays), m_surfaceSections, m_minY);
}

std::unordered_map<i64, i32>& SkyLightDataMap::surfaceSections() {
    return m_surfaceSections;
}

const std::unordered_map<i64, i32>& SkyLightDataMap::surfaceSections() const {
    return m_surfaceSections;
}

i32 SkyLightDataMap::minY() const {
    return m_minY;
}

void SkyLightDataMap::setMinY(i32 minY) {
    m_minY = minY;
}

i32 SkyLightDataMap::getSurfaceHeight(i64 columnPos) const {
    auto it = m_surfaceSections.find(columnPos);
    if (it != m_surfaceSections.end()) {
        return it->second;
    }
    return m_minY;
}

void SkyLightDataMap::setSurfaceHeight(i64 columnPos, i32 height) {
    m_surfaceSections[columnPos] = height;
}

void SkyLightDataMap::removeSurfaceHeight(i64 columnPos) {
    m_surfaceSections.erase(columnPos);
}

// ============================================================================
// SkyLightStorage 实现
// ============================================================================

SkyLightStorage::SkyLightStorage(IChunkLightProvider* provider)
    : SectionLightStorage<SkyLightDataMap>(
        LightType::SKY,
        provider,
        SkyLightDataMap()) {
}

u8 SkyLightStorage::getLightOrDefault(i64 worldPos) const {
    i64 sectionPos = worldToSectionPos(worldPos);
    i32 sectionY = SectionPos::fromLong(sectionPos).y;

    i32 surfaceHeight = m_cachedLightData.getSurfaceHeight(
        SectionPos::fromLong(sectionPos).toColumnLong());

    // 如果在表面高度之上，返回15（全亮）
    if (surfaceHeight != m_cachedLightData.minY() && sectionY < surfaceHeight) {
        const NibbleArray* array = getArray(sectionPos, true);
        i64 currentPos = worldPos;

        if (array == nullptr) {
            // 向上查找有效的区块段
            i64 currentSectionPos = sectionPos;
            i32 currentY = sectionY;

            while (array == nullptr) {
                currentSectionPos = SectionPos::fromLong(currentSectionPos).offset(Direction::Up).toLong();
                ++currentY;

                if (currentY >= surfaceHeight) {
                    return 15;
                }

                array = getArray(currentSectionPos, true);
        i32 currentX, currentBlockY, currentZ;
        LightEngineUtils::unpackPos(currentPos, currentX, currentBlockY, currentZ);
        currentPos = LightEngineUtils::packPos(currentX, currentBlockY + 16, currentZ);
            }
        }

        // 解码坐标
    i32 x = static_cast<i32>((currentPos >> 38) & 0xF);
    i32 y = static_cast<i32>(currentPos & 0xFFF);
    i32 z = static_cast<i32>((currentPos >> 12) & 0xF);
        i32 localY = y & 0xF;

        return array->get(x, localY, z);
    }

    // 在表面之上，天空光照为15
    return 15;
}

u8 SkyLightStorage::getLight(i64 worldPos) const {
    i64 sectionPos = worldToSectionPos(worldPos);
    const NibbleArray* array = getArray(sectionPos, true);

    if (array == nullptr) {
        return 0;
    }

    i32 x = static_cast<i32>((worldPos >> 38) & 0xF);
    i32 y = static_cast<i32>(worldPos & 0xFFF);
    i32 z = static_cast<i32>((worldPos >> 12) & 0xF);
    i32 localY = y & 0xF;

    return array->get(x, localY, z);
}

void SkyLightStorage::setLight(i64 worldPos, u8 light) {
    i64 sectionPos = worldToSectionPos(worldPos);

    // 标记为脏
    if (m_dirtyCachedSections.insert(sectionPos).second) {
        m_cachedLightData.copyArray(sectionPos);
    }

    NibbleArray* array = getArray(sectionPos, true);
    if (array == nullptr) {
        return;
    }

    i32 x = static_cast<i32>((worldPos >> 38) & 0xF);
    i32 y = static_cast<i32>(worldPos & 0xFFF);
    i32 z = static_cast<i32>((worldPos >> 12) & 0xF);
    i32 localY = y & 0xF;

    array->set(x, localY, z, light);

    // 标记相邻区块段变更
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                SectionPos pos = SectionPos::fromLong(sectionPos);
                i64 neighborPos = SectionPos(pos.x + dx, pos.y + dy, pos.z + dz).toLong();
                m_changedLightPositions.insert(neighborPos);
            }
        }
    }
}

void SkyLightStorage::setColumnEnabled(i64 columnPos, bool enabled) {
    processAllLevelUpdates();

    if (enabled && m_enabledColumns.insert(columnPos).second) {
        i32 surfaceHeight = m_cachedLightData.getSurfaceHeight(columnPos);

        if (surfaceHeight != m_cachedLightData.minY()) {
            i64 sectionPos = SectionPos(
                static_cast<i32>((columnPos >> 42) & 0xFFFFFFF),
                surfaceHeight - 1,
                static_cast<i32>((columnPos << 22) >> 42)
            ).toLong();

            scheduleFullUpdate(sectionPos);
            updateHasPendingUpdates();
        }
    } else if (!enabled) {
        m_enabledColumns.erase(columnPos);
    }
}

bool SkyLightStorage::isSectionEnabled(i64 sectionPos) const {
    i64 columnPos = SectionPos::fromLong(sectionPos).toColumnLong();
    return m_enabledColumns.count(columnPos) > 0;
}

bool SkyLightStorage::isAboveWorld(i64 sectionPos) const {
    i64 columnPos = SectionPos::fromLong(sectionPos).toColumnLong();
    i32 surfaceHeight = m_cachedLightData.getSurfaceHeight(columnPos);
    i32 sectionY = SectionPos::fromLong(sectionPos).y;

    return surfaceHeight == m_cachedLightData.minY() || sectionY >= surfaceHeight;
}

bool SkyLightStorage::isAtSurfaceTop(i64 worldPos) const {
    i32 y = static_cast<i32>(worldPos & 0xFFF);

    // 检查是否在区块段顶部 (y & 15 == 15)
    if ((y & 0xF) != 15) {
        return false;
    }

    i64 sectionPos = worldToSectionPos(worldPos);
    i64 columnPos = SectionPos::fromLong(sectionPos).toColumnLong();

    if (m_enabledColumns.count(columnPos) == 0) {
        return false;
    }

    i32 surfaceHeight = m_cachedLightData.getSurfaceHeight(columnPos);
    return surfaceHeight == (y + 16) / 16;
}

bool SkyLightStorage::hasSectionsToUpdate() const {
    return SectionLightStorage<SkyLightDataMap>::hasSectionsToUpdate() || m_hasPendingUpdates;
}

void SkyLightStorage::addSection(i64 sectionPos) {
    SectionPos pos = SectionPos::fromLong(sectionPos);
    i32 sectionY = pos.y;

    // 更新最小Y
    if (m_cachedLightData.minY() > sectionY) {
        m_cachedLightData.setMinY(sectionY);
    }

    i64 columnPos = pos.toColumnLong();
    i32 currentSurfaceHeight = m_cachedLightData.getSurfaceHeight(columnPos);

    // 更新表面高度
    if (currentSurfaceHeight < sectionY + 1) {
        m_cachedLightData.setSurfaceHeight(columnPos, sectionY + 1);

        if (m_enabledColumns.count(columnPos) > 0) {
            scheduleFullUpdate(sectionPos);

            if (currentSurfaceHeight > m_cachedLightData.minY()) {
                i64 belowPos = SectionPos(pos.x, currentSurfaceHeight - 1, pos.z).toLong();
                scheduleSurfaceUpdate(belowPos);
            }

            updateHasPendingUpdates();
        }
    }
}

void SkyLightStorage::removeSection(i64 sectionPos) {
    SectionPos pos = SectionPos::fromLong(sectionPos);
    i64 columnPos = pos.toColumnLong();
    bool isEnabled = m_enabledColumns.count(columnPos) > 0;

    if (isEnabled) {
        scheduleSurfaceUpdate(sectionPos);
    }

    i32 sectionY = pos.y;
    i32 currentSurfaceHeight = m_cachedLightData.getSurfaceHeight(columnPos);

    // 如果移除的是当前表面区块段
    if (currentSurfaceHeight == sectionY + 1) {
        i64 searchPos = sectionPos;
        i32 searchY = sectionY;

        // 向下查找有数据的区块段
        while (!hasSection(searchPos) && isAboveBottom(searchY)) {
            searchPos = SectionPos::fromLong(searchPos).offset(Direction::Down).toLong();
            --searchY;
        }

        if (hasSection(searchPos)) {
            m_cachedLightData.setSurfaceHeight(columnPos, searchY + 1);
            if (isEnabled) {
                scheduleFullUpdate(searchPos);
            }
        } else {
            m_cachedLightData.removeSurfaceHeight(columnPos);
        }
    }

    if (isEnabled) {
        updateHasPendingUpdates();
    }
}

NibbleArray SkyLightStorage::getOrCreateArray(i64 sectionPos) {
    NibbleArray* newArray = m_newArrays.getArray(sectionPos);
    if (newArray != nullptr) {
        return newArray->copy();
    }

    // 向上查找有效的光照数据
    i64 abovePos = SectionPos::fromLong(sectionPos).offset(Direction::Up).toLong();
    i32 surfaceHeight = m_cachedLightData.getSurfaceHeight(
        SectionPos::fromLong(sectionPos).toColumnLong());

    if (surfaceHeight != m_cachedLightData.minY() &&
        SectionPos::fromLong(abovePos).y < surfaceHeight) {
        const NibbleArray* aboveArray = getArray(abovePos, true);

        while (aboveArray == nullptr) {
            abovePos = SectionPos::fromLong(abovePos).offset(Direction::Up).toLong();
            aboveArray = getArray(abovePos, true);
        }

        // 复制上方的数据
        if (aboveArray != nullptr) {
            return aboveArray->copy();
        }
    }

    // 默认全亮
    return NibbleArray();
}

bool SkyLightStorage::isAboveBottom(i32 sectionY) const {
    return sectionY >= m_cachedLightData.minY();
}

void SkyLightStorage::scheduleFullUpdate(i64 sectionPos) {
    m_pendingAdditions.insert(sectionPos);
    m_pendingRemovals.erase(sectionPos);
}

void SkyLightStorage::scheduleSurfaceUpdate(i64 sectionPos) {
    m_pendingRemovals.insert(sectionPos);
    m_pendingAdditions.erase(sectionPos);
}

void SkyLightStorage::updateHasPendingUpdates() {
    m_hasPendingUpdates = !m_pendingAdditions.empty() || !m_pendingRemovals.empty();
}

// ============================================================================
// 辅助函数
// ============================================================================

// 静态成员定义
constexpr Direction SkyLightStorage::HORIZONTAL_DIRECTIONS[4];

} // namespace mc
