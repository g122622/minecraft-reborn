#include "SkyLightEngine.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/IChunk.hpp"
#include <climits>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

SkyLightEngine::SkyLightEngine(IChunkLightProvider* provider)
    : LevelBasedGraph(16, 256, 8192)
    , m_chunkProvider(provider)
    , m_storage(provider) {
}

// ============================================================================
// 光照操作
// ============================================================================

void SkyLightEngine::checkLight(const BlockPos& pos) {
    m_storage.processAllLevelUpdates();

    i64 packedPos = packPos(pos);
    i64 sectionPos = worldToSectionPos(packedPos);

    if (m_storage.hasSection(sectionPos)) {
        scheduleUpdate(packedPos);
    } else {
        // 向上查找有效的区块段
        i64 currentPos = packedPos;
        i64 currentSectionPos = sectionPos;

        while (!m_storage.hasSection(currentSectionPos) &&
               !m_storage.isAboveWorld(currentPos)) {
            currentPos = offsetPos(currentPos, Direction::Up);
            currentSectionPos = worldToSectionPos(currentPos);
            // 注意：这里需要移动16格
            currentPos = packPos(
                static_cast<i32>((currentPos >> 38) & 0xFFFFFFF),
                static_cast<i32>((currentPos & 0xFFF) + 16),
                static_cast<i32>((currentPos >> 26) & 0xFFFFFFF));
        }

        if (m_storage.hasSection(currentSectionPos)) {
            scheduleUpdate(currentPos);
        }
    }

    // 通知相邻方块
    scheduleUpdate(offsetPos(packedPos, Direction::Down));
    scheduleUpdate(offsetPos(packedPos, Direction::Up));
    scheduleUpdate(offsetPos(packedPos, Direction::North));
    scheduleUpdate(offsetPos(packedPos, Direction::South));
    scheduleUpdate(offsetPos(packedPos, Direction::West));
    scheduleUpdate(offsetPos(packedPos, Direction::East));
}

u8 SkyLightEngine::getLightFor(const BlockPos& pos) const {
    return m_storage.getLightOrDefault(packPos(pos));
}

void SkyLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    m_storage.updateSectionStatus(pos.toLong(), isEmpty);
}

void SkyLightEngine::setData(const SectionPos& pos, NibbleArray* array, bool retain) {
    m_storage.setData(pos.toLong(), array, retain);
}

NibbleArray* SkyLightEngine::getData(const SectionPos& pos) {
    return m_storage.getArray(pos.toLong());
}

void SkyLightEngine::setColumnEnabled(i64 columnPos, bool enabled) {
    m_storage.setColumnEnabled(columnPos, enabled);
}

bool SkyLightEngine::hasWork() const {
    return needsUpdate() || m_storage.hasSectionsToUpdate();
}

i32 SkyLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) {
    (void)updateBlockLight;  // 天空光照引擎不处理方块光照

    // 处理存储更新
    m_storage.processAllLevelUpdates();

    // 处理区块段更新（添加/移除）
    maxUpdates = m_storage.updateSections(this, updateSkyLight, false);
    if (maxUpdates == 0) {
        return 0;
    }

    // 处理光照传播
    if (needsUpdate() && updateSkyLight) {
        maxUpdates = processUpdates(maxUpdates);
        if (maxUpdates == 0) {
            return 0;
        }
    }

    m_storage.updateAndNotify();
    return maxUpdates;
}

// ============================================================================
// LevelBasedGraph 接口实现
// ============================================================================

bool SkyLightEngine::isRoot(i64 pos) const {
    (void)pos;
    return false;  // 天空光照没有根节点
}

i32 SkyLightEngine::computeLevel(i64 pos, i64 excludedSource, i32 level) {
    i32 minLevel = level;

    // 如果不是从根节点排除，检查天空光照贡献
    if (excludedSource != LONG_MAX) {
        i32 sourceContribution = getEdgeLevel(LONG_MAX, pos, 0);
        if (level > sourceContribution) {
            minLevel = sourceContribution;
        }

        if (minLevel == 0) {
            return 0;
        }
    }

    i64 sectionPos = worldToSectionPos(pos);
    const NibbleArray* array = m_storage.getArray(sectionPos, true);

    // 检查所有相邻方向
    static const Direction directions[] = {
        Direction::Down, Direction::Up, Direction::North,
        Direction::South, Direction::West, Direction::East
    };

    for (Direction dir : directions) {
        i64 neighborPos = offsetPos(pos, dir);
        if (neighborPos == excludedSource) {
            continue;
        }

        i64 neighborSectionPos = worldToSectionPos(neighborPos);
        const NibbleArray* neighborArray;

        if (neighborSectionPos == sectionPos) {
            neighborArray = array;
        } else {
            neighborArray = m_storage.getArray(neighborSectionPos, true);
        }

        if (neighborArray != nullptr) {
            i32 neighborLevel = getLevelFromArray(neighborArray, neighborPos);
            i32 edgeLevel = getEdgeLevel(neighborPos, pos, neighborLevel);

            if (minLevel > edgeLevel) {
                minLevel = edgeLevel;
            }

            if (minLevel == 0) {
                return 0;
            }
        } else if (dir != Direction::Down) {
            // 向上查找有效的区块段
            i64 searchPos = neighborPos;
            i64 searchSectionPos = neighborSectionPos;

            // 计算区块段底部Y坐标
            i32 neighborY = static_cast<i32>(neighborPos & 0xFFF);
            neighborY = (neighborY & 0xF);  // 局部Y

            while (!m_storage.hasSection(searchSectionPos) &&
                   !m_storage.isAboveWorld(searchPos)) {
                searchSectionPos = SectionPos::fromLong(searchSectionPos).offset(Direction::Up).toLong();
                // 向上移动16格
            }

            const NibbleArray* searchArray = m_storage.getArray(searchSectionPos, true);
            if (neighborPos != excludedSource) {
                i32 searchLevel;
                if (searchArray != nullptr) {
                    searchLevel = getLevelFromArray(searchArray, searchPos);
                } else {
                    searchLevel = m_storage.isSectionEnabled(searchSectionPos) ? 0 : 15;
                }

                if (minLevel > searchLevel) {
                    minLevel = searchLevel;
                }

                if (minLevel == 0) {
                    return 0;
                }
            }
        }
    }

    return minLevel;
}

void SkyLightEngine::notifyNeighbors(i64 pos, i32 level, bool isDecreasing) {
    i64 sectionPos = worldToSectionPos(pos);
    i32 y = static_cast<i32>(pos & 0xFFF);
    i32 localY = y & 0xF;
    i32 sectionY = y >> 4;

    // 计算需要向下传播多少区块段
    i32 skipSections = 0;
    if (localY == 0) {
        // 在区块段底部，需要检查下方有多少空区块段
        while (!m_storage.hasSection(
                SectionPos::fromLong(sectionPos).offset(Direction::Down).toLong()) &&
               m_storage.isAboveBottom(sectionY - skipSections - 1)) {
            ++skipSections;
        }
    }

    // 向下传播（可能跨越多个区块段）
    i64 downPos = packPos(
        static_cast<i32>((pos >> 38) & 0xFFFFFFF),
        static_cast<i32>((pos & 0xFFF) - 1 - skipSections * 16),
        static_cast<i32>((pos >> 26) & 0xFFFFFFF));
    i64 downSectionPos = worldToSectionPos(downPos);

    if (sectionPos == downSectionPos || m_storage.hasSection(downSectionPos)) {
        propagateLevel(pos, downPos, level, isDecreasing);
    }

    // 向上传播
    i64 upPos = offsetPos(pos, Direction::Up);
    i64 upSectionPos = worldToSectionPos(upPos);
    if (sectionPos == upSectionPos || m_storage.hasSection(upSectionPos)) {
        propagateLevel(pos, upPos, level, isDecreasing);
    }

    // 水平方向传播
    static const Direction horizontalDirs[] = {
        Direction::North, Direction::South, Direction::West, Direction::East
    };

    for (Direction dir : horizontalDirs) {
        i32 dx = (dir == Direction::East) ? 1 : ((dir == Direction::West) ? -1 : 0);
        i32 dz = (dir == Direction::South) ? 1 : ((dir == Direction::North) ? -1 : 0);

        i32 offset = 0;
        while (true) {
            i64 neighborPos = packPos(
                static_cast<i32>((pos >> 38) & 0xFFFFFFF) + dx,
                static_cast<i32>((pos & 0xFFF) - offset),
                static_cast<i32>((pos >> 26) & 0xFFFFFFF) + dz);
            i64 neighborSectionPos = worldToSectionPos(neighborPos);

            if (sectionPos == neighborSectionPos) {
                propagateLevel(pos, neighborPos, level, isDecreasing);
                break;
            }

            if (m_storage.hasSection(neighborSectionPos)) {
                propagateLevel(pos, neighborPos, level, isDecreasing);
            }

            ++offset;
            if (offset > skipSections * 16) {
                break;
            }
        }
    }
}

i32 SkyLightEngine::getLevel(i64 pos) const {
    return 15 - m_storage.getLightOrDefault(pos);
}

void SkyLightEngine::setLevel(i64 pos, i32 level) {
    m_storage.setLight(pos, static_cast<u8>(15 - std::min(level, 15)));
}

i32 SkyLightEngine::getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) {
    if (toPos == LONG_MAX) {
        return 15;
    }

    if (fromPos == LONG_MAX) {
        // 从天空发出
        if (!m_storage.isAtSurfaceTop(toPos)) {
            return 15;
        }
        startLevel = 0;
    }

    if (startLevel >= 15) {
        return startLevel;
    }

    i32 opacity = 0;
    const BlockState* toState = getBlockAndOpacity(toPos, &opacity);

    if (opacity >= 15) {
        return 15;
    }

    const BlockState* fromState = getBlockAndOpacity(fromPos, nullptr);
    IWorld* world = m_chunkProvider->getWorld();

    // 计算方向
    i32 fromX, fromY, fromZ;
    i32 toX, toY, toZ;
    unpackPos(fromPos, fromX, fromY, fromZ);
    unpackPos(toPos, toX, toY, toZ);

    bool sameXZ = (fromX == toX) && (fromZ == toZ);
    i32 dx = (toX > fromX) ? 1 : ((toX < fromX) ? -1 : 0);
    i32 dy = (toY > fromY) ? 1 : ((toY < fromY) ? -1 : 0);
    i32 dz = (toZ > fromZ) ? 1 : ((toZ < fromZ) ? -1 : 0);

    Direction direction;
    if (fromPos == LONG_MAX) {
        direction = Direction::Down;
    } else {
        direction = Directions::fromDelta(dx, dy, dz);
    }

    if (direction != Direction::None) {
        // 检查面遮挡
        if (facesHaveOcclusion(world, *fromState, BlockPos(fromX, fromY, fromZ),
                              *toState, BlockPos(toX, toY, toZ),
                              direction, opacity)) {
            return 15;
        }
    } else {
        // 非相邻方块，检查Y方向遮挡
        const CollisionShape& fromShape = getVoxelShape(*fromState, fromPos, Direction::Down);
        if (!fromShape.isEmpty()) {
            return 15;
        }

        i32 adjustedDy = sameXZ ? -1 : 0;
        Direction adjustedDir = Directions::fromDelta(dx, adjustedDy, dz);
        if (adjustedDir == Direction::None) {
            return 15;
        }

        const CollisionShape& toShape = getVoxelShape(*toState, toPos, Directions::opposite(adjustedDir));
        if (!toShape.isEmpty()) {
            return 15;
        }
    }

    // 天空光照特殊处理：从天空向下传播且无遮挡时，衰减为0
    bool isFromSky = (fromPos == LONG_MAX) || (sameXZ && fromY > toY);
    if (isFromSky && startLevel == 0 && opacity == 0) {
        return 0;
    }

    return startLevel + std::max(1, opacity);
}

// ============================================================================
// 私有方法
// ============================================================================

i32 SkyLightEngine::getLightValue(i64 worldPos) const {
    // 天空光照引擎不处理发光方块
    (void)worldPos;
    return 0;
}

const BlockState* SkyLightEngine::getBlockAndOpacity(i64 worldPos, i32* opacityOut) const {
    if (worldPos == LONG_MAX) {
        if (opacityOut != nullptr) {
            *opacityOut = 0;
        }
        return nullptr;  // 空气
    }

    i32 x = static_cast<i32>((worldPos >> 38) & 0xFFFFFFF);
    i32 y = static_cast<i32>((worldPos >> 0) & 0xFFF);
    i32 z = static_cast<i32>((worldPos >> 26) & 0xFFFFFFF);

    // 符号扩展
    x = (x << 4) >> 4;
    z = (z << 4) >> 4;

    const IChunk* chunk = m_chunkProvider->getChunkForLight(x >> 4, z >> 4);
    if (chunk == nullptr) {
        if (opacityOut != nullptr) {
            *opacityOut = 15;  // 视为不透明
        }
        return nullptr;  // 基岩
    }

    const BlockState* state = chunk->getBlock(x & 0xF, y, z & 0xF);
    if (state == nullptr) {
        if (opacityOut != nullptr) {
            *opacityOut = 0;
        }
        return nullptr;
    }

    if (opacityOut != nullptr) {
        IWorld* world = m_chunkProvider->getWorld();
        *opacityOut = state->getOpacity();
    }

    return state;
}

const CollisionShape& SkyLightEngine::getVoxelShape(const BlockState& state, i64 pos, Direction dir) const {
    (void)pos;
    (void)dir;
    // 对于光照，我们只关心方块是否是固体
    if (state.isSolid()) {
        return state.getOcclusionShape();
    }
    return VoxelShapes::empty();
}

bool SkyLightEngine::facesHaveOcclusion(
    IWorld* world,
    const BlockState& stateA, const BlockPos& posA,
    const BlockState& stateB, const BlockPos& posB,
    Direction dir, i32 opacity) {
    (void)world;
    (void)posA;
    (void)posB;
    (void)dir;
    (void)opacity;

    // 简化版本：检查两个方块是否都是固体且有完整遮挡面
    bool isSolidA = stateA.isSolid() && stateA.isTransparent();
    bool isSolidB = stateB.isSolid() && stateB.isTransparent();

    if (!isSolidA && !isSolidB) {
        return false;
    }

    // 完整实现需要检查VoxelShape的面遮挡
    // 这里使用简化版本
    return false;
}

i32 SkyLightEngine::getLevelFromArray(const NibbleArray* array, i64 worldPos) const {
    if (array == nullptr) {
        return 15;
    }

    i32 x = static_cast<i32>((worldPos >> 38) & 0xF);
    i32 y = static_cast<i32>(worldPos & 0xFFF);
    i32 z = static_cast<i32>((worldPos >> 26) & 0xF);
    i32 localY = y & 0xF;

    return 15 - array->get(x, localY, z);
}

i64 SkyLightEngine::packPos(i32 x, i32 y, i32 z) {
    // 编码格式: X(28位) | Z(28位) | Y(12位)
    u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFLL);
    u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFLL);
    u64 uy = static_cast<u64>(y) & 0xFFF;
    return (ux << 38) | (uz << 12) | uy;
}

i64 SkyLightEngine::packPos(const BlockPos& pos) {
    return packPos(pos.x, pos.y, pos.z);
}

void SkyLightEngine::unpackPos(i64 packed, i32& x, i32& y, i32& z) {
    x = static_cast<i32>((packed >> 38) & 0xFFFFFFF);
    y = static_cast<i32>(packed & 0xFFF);
    z = static_cast<i32>((packed >> 12) & 0xFFFFFFF);

    // 符号扩展
    x = (x << 4) >> 4;
    z = (z << 4) >> 4;
}

i64 SkyLightEngine::offsetPos(i64 pos, Direction dir) {
    i32 x, y, z;
    unpackPos(pos, x, y, z);

    switch (dir) {
        case Direction::Down:    y--; break;
        case Direction::Up:      y++; break;
        case Direction::North:   z--; break;
        case Direction::South:   z++; break;
        case Direction::West:    x--; break;
        case Direction::East:    x++; break;
        default: break;
    }

    return packPos(x, y, z);
}

i64 SkyLightEngine::worldToSectionPos(i64 worldPos) {
    i32 x = static_cast<i32>((worldPos >> 38) & 0xFFFFFFF);
    i32 y = static_cast<i32>(worldPos & 0xFFF);
    i32 z = static_cast<i32>((worldPos >> 12) & 0xFFFFFFF);

    // 符号扩展
    x = (x << 4) >> 4;
    z = (z << 4) >> 4;

    return SectionPos(x >> 4, y >> 4, z >> 4).toLong();
}

} // namespace mc
