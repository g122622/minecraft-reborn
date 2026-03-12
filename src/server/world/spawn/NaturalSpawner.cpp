#include "NaturalSpawner.hpp"
#include "SpawnConditions.hpp"
#include "../ServerWorld.hpp"
#include "../../../common/entity/EntityRegistry.hpp"
#include "../../../common/entity/EntityClassification.hpp"
#include "../../../common/entity/Entity.hpp"
#include "../../../common/world/IWorld.hpp"
#include "../../../common/world/spawn/MobSpawnInfo.hpp"
#include "../../../common/world/WorldConstants.hpp"
#include "../../../common/math/random/Random.hpp"
#include <spdlog/spdlog.h>

namespace mc::world::spawn {

// ============================================================================
// 常量定义
// ============================================================================

/// 最大生成尝试次数
static constexpr i32 MAX_SPAWN_ATTEMPTS = 4;

NaturalSpawner::NaturalSpawner()
{
    // 初始化默认配置
}

void NaturalSpawner::spawnInChunk(mc::server::ServerWorld& world, i32 chunkX, i32 chunkZ,
                                   const MobSpawnInfo& spawnInfo, math::Random& random) {
    // 获取区块的世界坐标范围（使用工具函数）
    i32 minX = world::toWorldCoord(chunkX);
    i32 minZ = world::toWorldCoord(chunkZ);

    // 生成怪物（夜间或黑暗环境）
    const auto& monsters = spawnInfo.getMonsterSpawns();
    if (!monsters.empty()) {
        // 随机选择生成位置
        for (i32 attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; ++attempt) {
            i32 x = minX + random.nextInt(16);
            i32 z = minZ + random.nextInt(16);

            // 获取生成高度
            i32 y = getSpawnHeight(world, x, z);
            if (y < 0) continue;

            // 选择生成条目
            const SpawnEntry* entry = selectEntry(monsters, random);
            if (!entry) continue;

            // 检查光照条件
            if (!checkLightLevel(world, x, y, z, true)) continue;

            // 尝试生成
            trySpawnAt(world, x, y, z, *entry, random);
        }
    }

    // 生成动物（白天和足够光照）
    const auto& creatures = spawnInfo.getCreatureSpawns();
    if (!creatures.empty()) {
        // 动物生成概率较低 (1/400)
        if (random.nextInt(400) == 0) {
            i32 x = minX + random.nextInt(16);
            i32 z = minZ + random.nextInt(16);

            i32 y = getSpawnHeight(world, x, z);
            if (y < 0) return;

            const SpawnEntry* entry = selectEntry(creatures, random);
            if (!entry) return;

            // 动物需要足够光照
            if (!checkLightLevel(world, x, y, z, false)) return;

            trySpawnAt(world, x, y, z, *entry, random);
        }
    }

    // 生成环境生物（蝙蝠等）
    const auto& ambient = spawnInfo.getAmbientSpawns();
    if (!ambient.empty()) {
        // 环境生物生成概率较低
        if (random.nextInt(100) < 10) {
            i32 x = minX + random.nextInt(16);
            i32 z = minZ + random.nextInt(16);

            i32 y = getSpawnHeight(world, x, z);
            if (y < 0) return;

            const SpawnEntry* entry = selectEntry(ambient, random);
            if (!entry) return;

            trySpawnAt(world, x, y, z, *entry, random);
        }
    }

    // 生成水生生物
    const auto& waterCreatures = spawnInfo.getWaterCreatureSpawns();
    if (!waterCreatures.empty()) {
        // 水生生物生成概率较低
        if (random.nextInt(100) < 5) {
            // 暂时跳过水生生物生成
            // TODO: 实现水中生成检测
        }
    }
}

void NaturalSpawner::tick(mc::server::ServerWorld& /*world*/, bool /*hostile*/, bool /*passive*/) {
    // TODO: 实现每tick的自然生成
    // 1. 获取玩家位置
    // 2. 在玩家周围选择生成位置
    // 3. 根据生物群系获取生成配置
    // 4. 尝试生成实体
    //
    // 当前实现为存根，需要 ServerWorld 提供：
    // - 玩家迭代器
    // - 生物群系查询接口
    // - EntityManager 集成
}

i32 NaturalSpawner::trySpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z,
                                const SpawnEntry& entry, math::Random& random) {
    // 获取实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entry.entityTypeId);
    if (!entityType) {
        spdlog::debug("Unknown entity type for spawning: {}", entry.entityTypeId);
        return 0;
    }

    // 确定生成数量（使用 nextInt(min, max) 包含两端）
    i32 count = entry.minCount;
    if (entry.maxCount > entry.minCount) {
        count = random.nextInt(entry.minCount, entry.maxCount);
    }

    i32 spawned = 0;

    // 获取实体尺寸
    entity::EntitySize size = entityType->size();
    f32 width = size.width();
    [[maybe_unused]] f32 height = size.height();

    // 尝试生成多个实体
    for (i32 i = 0; i < count; ++i) {
        // 添加随机偏移，使群体分散
        f32 offsetX = (i % 3 - 1) * width;
        f32 offsetZ = (i / 3 - 1) * width;

        i32 spawnX = x + static_cast<i32>(offsetX);
        i32 spawnZ = z + static_cast<i32>(offsetZ);

        // 检查生成位置
        if (!canSpawnAt(world, spawnX, y, spawnZ, entry)) {
            continue;
        }

        // 检查是否有自定义位置检查
        auto checkIt = m_positionChecks.find(entry.entityTypeId);
        if (checkIt != m_positionChecks.end()) {
            if (!checkIt->second(world, spawnX, y, spawnZ, entry)) {
                continue;
            }
        }

        // TODO: 创建实体并添加到世界
        // 1. entityType->create(world) 创建实体
        // 2. 设置实体位置
        // 3. 添加到 EntityManager
        // 这需要 ServerWorld 提供创建实体的接口

        ++spawned;
    }

    return spawned;
}

const SpawnEntry* NaturalSpawner::selectEntry(
    const std::vector<SpawnEntry>& entries, math::Random& random) const {
    if (entries.empty()) {
        return nullptr;
    }

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& entry : entries) {
        totalWeight += entry.weight;
    }

    if (totalWeight <= 0) {
        return nullptr;
    }

    // 随机选择
    i32 value = random.nextInt(totalWeight);
    i32 current = 0;

    for (const auto& entry : entries) {
        current += entry.weight;
        if (value < current) {
            return &entry;
        }
    }

    return nullptr;
}

bool NaturalSpawner::canSpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z,
                                 const SpawnEntry& entry) const {
    // 获取实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entry.entityTypeId);
    if (!entityType) {
        return false;
    }

    // 边界检查（使用工具函数）
    if (!world::isValidY(y)) {
        return false;
    }

    // 获取实体尺寸
    [[maybe_unused]] entity::EntitySize size = entityType->size();

    // TODO: 当 ServerWorld 实现 IWorld 后使用 SpawnConditions::canSpawnAtPosition
    // 当前使用简化的检查

    // 检查分类特定的条件
    entity::EntityClassification classification = entityType->classification();

    switch (classification) {
        case entity::EntityClassification::Monster:
            // 怪物需要低光照
            return checkLightLevel(world, x, y, z, true);

        case entity::EntityClassification::Creature:
            // 动物需要足够光照
            // TODO: 检查脚下方块是否为草方块
            return checkLightLevel(world, x, y, z, false);

        case entity::EntityClassification::Ambient:
            // 环境生物（蝙蝠）需要低光照
            return checkLightLevel(world, x, y, z, true);

        case entity::EntityClassification::WaterCreature:
        case entity::EntityClassification::WaterAmbient:
            // 水生生物需要在水中
            // TODO: 实现水中检测
            return false;

        case entity::EntityClassification::Misc:
            // 其他类型无特殊限制
            return true;
    }

    return true;
}

bool NaturalSpawner::checkLightLevel(mc::server::ServerWorld& /*world*/,
                                      i32 /*x*/, i32 y, i32 /*z*/,
                                      bool isMonster) const {
    // 获取光照等级
    // TODO: 从 ServerWorld 获取光照
    // 当前使用简单的高度判断

    // 计算天空光照（基于高度和时间）
    // 白天地面天空光照为 15
    // 夜间天空光照为 4
    i32 skyLight = 15;

    // 如果位置在地下，天空光照减少
    if (y < 64) {
        // 每下降一层减少一定光照
        skyLight = std::max(0, 15 - (64 - y) / 4);
    }

    // 方块光照默认为 0（无光源）
    i32 blockLight = 0;

    // TODO: 从世界获取实际光照
    // i32 skyLight = world.getSkyLight(x, y, z);
    // i32 blockLight = world.getBlockLight(x, y, z);

    return SpawnConditions::checkLightLevel(skyLight, blockLight, isMonster);
}

i32 NaturalSpawner::getSpawnHeight(mc::server::ServerWorld& world, i32 x, i32 z) const {
    // 获取区块
    ChunkCoord chunkX = static_cast<ChunkCoord>(x >> 4);
    ChunkCoord chunkZ = static_cast<ChunkCoord>(z >> 4);

    const ChunkData* chunk = world.getChunk(chunkX, chunkZ);
    if (!chunk) {
        return -1;
    }

    // 从最高点向下搜索可站立位置
    // TODO: 使用 chunk 的高度图或遍历方块
    // 暂时返回固定高度
    (void)chunk; // 避免未使用警告

    // 使用默认高度
    return 64;
}

} // namespace mc::world::spawn
