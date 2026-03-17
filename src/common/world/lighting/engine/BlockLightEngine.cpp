#include "BlockLightEngine.hpp"
#include "LightEngineUtils.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/IChunk.hpp"
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

    i64 packedPos = LightEngineUtils::packPos(pos);
    if (m_storage.hasSection(LightEngineUtils::worldToSectionPos(packedPos))) {
        scheduleUpdate(packedPos);
    }

    // 通知相邻方块
    scheduleUpdate(LightEngineUtils::offsetPos(packedPos, Direction::Down));
    scheduleUpdate(LightEngineUtils::offsetPos(packedPos, Direction::Up));
    scheduleUpdate(LightEngineUtils::offsetPos(packedPos, Direction::North));
    scheduleUpdate(LightEngineUtils::offsetPos(packedPos, Direction::South));
    scheduleUpdate(LightEngineUtils::offsetPos(packedPos, Direction::West));
    scheduleUpdate(LightEngineUtils::offsetPos(packedPos, Direction::East));
}

void BlockLightEngine::onBlockEmissionIncrease(const BlockPos& pos, i32 lightLevel) {
    m_storage.processAllLevelUpdates();
    i64 packedPos = LightEngineUtils::packPos(pos);
    i64 sourcePos = LONG_MAX;  // 根节点表示光源

    scheduleUpdate(sourcePos, packedPos, 15 - lightLevel, true);
}

u8 BlockLightEngine::getLightFor(const BlockPos& pos) const {
    return m_storage.getLightOrDefault(LightEngineUtils::packPos(pos));
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
    (void)updateSkyLight;   // 方块光照引擎不处理天空光照
    (void)updateBlockLight; // 参数保留用于接口一致性

    // 处理存储更新（如区块段状态变化）
    m_storage.processAllLevelUpdates();

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

    i64 sectionPos = LightEngineUtils::worldToSectionPos(pos);
    const NibbleArray* array = m_storage.getArray(sectionPos, true);

    // 检查所有相邻方向
    static const Direction directions[] = {
        Direction::Down, Direction::Up, Direction::North,
        Direction::South, Direction::West, Direction::East
    };

    for (Direction dir : directions) {
        i64 neighborPos = LightEngineUtils::offsetPos(pos, dir);
        if (neighborPos == excludedSource) {
            continue;
        }

        i64 neighborSectionPos = LightEngineUtils::worldToSectionPos(neighborPos);
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
    i64 sectionPos = LightEngineUtils::worldToSectionPos(pos);

    static const Direction directions[] = {
        Direction::Down, Direction::Up, Direction::North,
        Direction::South, Direction::West, Direction::East
    };

    for (Direction dir : directions) {
        i64 neighborPos = LightEngineUtils::offsetPos(pos, dir);
        i64 neighborSectionPos = LightEngineUtils::worldToSectionPos(neighborPos);

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
    LightEngineUtils::unpackPos(fromPos, fromX, fromY, fromZ);
    LightEngineUtils::unpackPos(toPos, toX, toY, toZ);

    i32 dx = (toX > fromX) ? 1 : ((toX < fromX) ? -1 : 0);
    i32 dy = (toY > fromY) ? 1 : ((toY < fromY) ? -1 : 0);
    i32 dz = (toZ > fromZ) ? 1 : ((toZ < fromZ) ? -1 : 0);

    Direction direction = Directions::fromDelta(dx, dy, dz);
    if (direction == Direction::None) {
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
    if (LightEngineUtils::facesHaveOcclusion(world, *fromState, BlockPos(fromX, fromY, fromZ),
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
    i32 x, y, z;
    LightEngineUtils::unpackPos(worldPos, x, y, z);

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

    i32 x, y, z;
    LightEngineUtils::unpackPos(worldPos, x, y, z);

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

} // namespace mc
