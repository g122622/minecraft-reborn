#include "BlockLightStorage.hpp"
#include <climits>

namespace mc {

BlockLightStorage::BlockLightStorage(IChunkLightProvider* provider)
    : SectionLightStorage<BlockLightDataMap>(LightType::BLOCK, provider, BlockLightDataMap()) {
}

u8 BlockLightStorage::getLightOrDefault(i64 worldPos) const {
    i64 sectionPos = worldToSectionPos(worldPos);
    const NibbleArray* array = getArray(sectionPos, true);

    if (array == nullptr) {
        return 0;
    }

    // 从世界位置解码坐标
    i32 x = static_cast<i32>((worldPos >> 38) & 0xF);
    i32 y = static_cast<i32>(worldPos & 0xFFF);
    i32 z = static_cast<i32>((worldPos >> 12) & 0xF);

    // Y坐标转换为区块段内坐标
    i32 localY = y & 0xF;

    return array->get(x, localY, z);
}

u8 BlockLightStorage::getLight(i64 worldPos) const {
    i64 sectionPos = worldToSectionPos(worldPos);
    const NibbleArray* array = getArray(sectionPos, true);

    if (array == nullptr) {
        return 0;
    }

    // 从世界位置解码坐标
    i32 x = static_cast<i32>((worldPos >> 38) & 0xF);
    i32 y = static_cast<i32>(worldPos & 0xFFF);
    i32 z = static_cast<i32>((worldPos >> 12) & 0xF);

    // Y坐标转换为区块段内坐标
    i32 localY = y & 0xF;

    return array->get(x, localY, z);
}

void BlockLightStorage::setLight(i64 worldPos, u8 light) {
    i64 sectionPos = worldToSectionPos(worldPos);

    // 标记为脏
    if (m_dirtyCachedSections.insert(sectionPos).second) {
        m_cachedLightData.copyArray(sectionPos);
    }

    NibbleArray* array = getArray(sectionPos, true);
    if (array == nullptr) {
        return;
    }

    // 从世界位置解码坐标
    i32 x = static_cast<i32>((worldPos >> 38) & 0xF);
    i32 y = static_cast<i32>(worldPos & 0xFFF);
    i32 z = static_cast<i32>((worldPos >> 12) & 0xF);

    // Y坐标转换为区块段内坐标
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

const NibbleArray* BlockLightStorage::getArrayInSection(i64 sectionPos, bool useCache) const {
    return getArray(sectionPos, useCache);
}

} // namespace mc
