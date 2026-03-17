#include "BlockLightEngine.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include <climits>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

BlockLightEngine::BlockLightEngine(IChunkLightProvider* provider)
    : LevelBasedGraph(16, 256, 8192)
    , m_chunkProvider(provider)
    , m_storage(provider) {
}

// ============================================================================
// 光照操作
// ============================================================================

void BlockLightEngine::checkLight(const BlockPos& pos) {
    m_storage.processAllLevelUpdates();

    i64 packedPos = packPos(pos);
    if (m_storage.hasSection(worldToSectionPos(packedPos))) {
        scheduleUpdate(packedPos);
    }

    // 通知相邻方块
    scheduleUpdate(offsetPos(packedPos, Direction::Down));
    scheduleUpdate(offsetPos(packedPos, Direction::Up));
    scheduleUpdate(offsetPos(packedPos, Direction::North));
    scheduleUpdate(offsetPos(packedPos, Direction::South));
    scheduleUpdate(offsetPos(packedPos, Direction::West));
    scheduleUpdate(offsetPos(packedPos, Direction::East));
}

void BlockLightEngine::onBlockEmissionIncrease(const BlockPos& pos, i32 lightLevel) {
    m_storage.processAllLevelUpdates();
    i64 packedPos = packPos(pos);
    i64 sourcePos = LONG_MAX;  // 根节点表示光源

    scheduleUpdate(sourcePos, packedPos, 15 - lightLevel, true);
}

u8 BlockLightEngine::getLightFor(const BlockPos& pos) const {
    return m_storage.getLightOrDefault(packPos(pos));
}

void BlockLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    m_storage.updateSectionStatus(pos.toLong(), isEmpty);
}

void BlockLightEngine::setData(const SectionPos& pos, NibbleArray* array, bool retain) {
    m_storage.setData(pos.toLong(), array, retain);
}

NibbleArray* BlockLightEngine::getData(const SectionPos& pos) {
    return m_storage.getArray(pos.toLong());
}

bool BlockLightEngine::hasWork() const {
    return needsUpdate() || m_storage.hasSectionsToUpdate();
}

i32 BlockLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) {
    // 处理存储更新
    if (m_storage.needsUpdate()) {
        maxUpdates = m_storage.processUpdates(maxUpdates);
        if (maxUpdates == 0) {
            return 0;
        }
    }

    // 处理光照传播
    if (needsUpdate()) {
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

bool BlockLightEngine::isRoot(i64 pos) const {
    return pos == LONG_MAX;
}

i32 BlockLightEngine::computeLevel(i64 pos, i64 excludedSource, i32 level) {
    i32 minLevel = level;

    // 如果不是从根节点排除，检查光源贡献
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

        if (neighborArray == nullptr) {
            continue;
        }

        // 获取相邻位置的光照等级
        i32 x = static_cast<i32>((neighborPos >> 38) & 0xF);
        i32 y = static_cast<i32>(neighborPos & 0xFFF);
        i32 z = static_cast<i32>((neighborPos >> 26) & 0xF);
        i32 localY = y & 0xF;

        i32 neighborLevel = 15 - neighborArray->get(x, localY, z);
        i32 edgeLevel = getEdgeLevel(neighborPos, pos, neighborLevel);

        if (minLevel > edgeLevel) {
            minLevel = edgeLevel;
        }

        if (minLevel == 0) {
            return 0;
        }
    }

    return minLevel;
}

void BlockLightEngine::notifyNeighbors(i64 pos, i32 level, bool isDecreasing) {
    i64 sectionPos = worldToSectionPos(pos);

    static const Direction directions[] = {
        Direction::Down, Direction::Up, Direction::North,
        Direction::South, Direction::West, Direction::East
    };

    for (Direction dir : directions) {
        i64 neighborPos = offsetPos(pos, dir);
        i64 neighborSectionPos = worldToSectionPos(neighborPos);

        if (sectionPos == neighborSectionPos || m_storage.hasSection(neighborSectionPos)) {
            propagateLevel(pos, neighborPos, level, isDecreasing);
        }
    }
}

i32 BlockLightEngine::getLevel(i64 pos) const {
    if (pos == LONG_MAX) {
        return 0;
    }
    return 15 - m_storage.getLightOrDefault(pos);
}

void BlockLightEngine::setLevel(i64 pos, i32 level) {
    m_storage.setLight(pos, static_cast<u8>(15 - std::min(level, 15)));
}

i32 BlockLightEngine::getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) {
    if (toPos == LONG_MAX) {
        return 15;
    }

    if (fromPos == LONG_MAX) {
        // 从光源发出
        return startLevel + 15 - getLightValue(toPos);
    }

    if (startLevel >= 15) {
        return startLevel;
    }

    // 计算方向
    i32 fromX, fromY, fromZ;
    i32 toX, toY, toZ;
    unpackPos(fromPos, fromX, fromY, fromZ);
    unpackPos(toPos, toX, toY, toZ);

    i32 dx = (toX > fromX) ? 1 : ((toX < fromX) ? -1 : 0);
    i32 dy = (toY > fromY) ? 1 : ((toY < fromY) ? -1 : 0);
    i32 dz = (toZ > fromZ) ? 1 : ((toZ < fromZ) ? -1 : 0);

    Direction direction = Direction::fromDelta(dx, dy, dz);
    if (!direction.has_value()) {
        return 15;
    }

    i32 opacity = 0;
    const BlockState* toState = getBlockAndOpacity(toPos, &opacity);

    if (opacity >= 15) {
        return 15;
    }

    const BlockState* fromState = getBlockAndOpacity(fromPos, nullptr);
    IWorld* world = m_chunkProvider->getWorld();

    // 检查面遮挡
    if (facesHaveOcclusion(world, *fromState, BlockPos(fromX, fromY, fromZ),
                          *toState, BlockPos(toX, toY, toZ),
                          direction, opacity)) {
        return 15;
    }

    return startLevel + std::max(1, opacity);
}

// ============================================================================
// 私有方法
// ============================================================================

i32 BlockLightEngine::getLightValue(i64 worldPos) const {
    i32 x = static_cast<i32>((worldPos >> 38) & 0xFFFFFFF);  // 28位
    i32 y = static_cast<i32>((worldPos >> 0) & 0xFFF);       // 12位
    i32 z = static_cast<i32>((worldPos >> 26) & 0xFFFFFFF);  // 28位

    // 符号扩展
    x = (x << 4) >> 4;
    z = (z << 4) >> 4;

    const IChunk* chunk = m_chunkProvider->getChunkForLight(x >> 4, z >> 4);
    if (chunk == nullptr) {
        return 0;
    }

    const BlockState* state = chunk->getBlock(x & 0xF, y, z & 0xF);
    if (state == nullptr) {
        return 0;
    }

    return state->lightLevel();
}

const BlockState* BlockLightEngine::getBlockAndOpacity(i64 worldPos, i32* opacityOut) const {
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

const CollisionShape& BlockLightEngine::getVoxelShape(const BlockState& state, i64 pos, Direction dir) const {
    (void)pos;
    (void)dir;
    // 对于光照，我们只关心方块是否是固体
    if (state.isSolid()) {
        return state.getOcclusionShape();
    }
    return VoxelShapes::empty();
}

bool BlockLightEngine::facesHaveOcclusion(
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

i64 BlockLightEngine::packPos(i32 x, i32 y, i32 z) {
    // 编码格式: X(28位) | Z(28位) | Y(12位)
    u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFLL);
    u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFLL);
    u64 uy = static_cast<u64>(y) & 0xFFF;
    return (ux << 38) | (uz << 12) | uy;
}

i64 BlockLightEngine::packPos(const BlockPos& pos) {
    return packPos(pos.x, pos.y, pos.z);
}

void BlockLightEngine::unpackPos(i64 packed, i32& x, i32& y, i32& z) {
    x = static_cast<i32>((packed >> 38) & 0xFFFFFFF);
    y = static_cast<i32>(packed & 0xFFF);
    z = static_cast<i32>((packed >> 12) & 0xFFFFFFF);

    // 符号扩展
    x = (x << 4) >> 4;
    z = (z << 4) >> 4;
}

i64 BlockLightEngine::offsetPos(i64 pos, Direction dir) {
    i32 x, y, z;
    unpackPos(pos, x, y, z);

    switch (dir.value()) {
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

} // namespace mc
