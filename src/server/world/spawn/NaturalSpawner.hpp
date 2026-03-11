#pragma once

#include "../../../common/world/spawn/MobSpawnInfo.hpp"
#include "../../../common/entity/Entity.hpp"
#include "../../../common/math/random/Random.hpp"
#include <functional>
#include <memory>

namespace mr {

// 前向声明
class ServerWorld;
class ChunkData;

namespace world::spawn {

/**
 * @brief 生成位置检查函数类型
 *
 * 检查指定位置是否可以生成特定类型的实体。
 * 返回 true 表示可以生成。
 */
using SpawnPositionCheck = std::function<bool(ServerWorld&, i32 x, i32 y, i32 z, const SpawnEntry&)>;

/**
 * @brief 自然生成器
 *
 * 负责在区块中自然生成实体。
 * 根据生物群系生成配置、光照条件、实体密度等进行生成。
 *
 * 参考 MC 1.16.5 NaturalSpawner
 */
class NaturalSpawner {
public:
    NaturalSpawner();
    ~NaturalSpawner() = default;

    // ========== 区块生成 ==========

    /**
     * @brief 在区块中生成实体
     * @param world 世界
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param spawnInfo 生物群系生成信息
     * @param random 随机数生成器
     */
    void spawnInChunk(ServerWorld& world, i32 chunkX, i32 chunkZ,
                      const MobSpawnInfo& spawnInfo, math::Random& random);

    /**
     * @brief 在世界中进行自然生成（每tick调用）
     * @param world 世界
     * @param hostile 是否生成敌对生物
     * @param passive 是否生成被动生物
     */
    void tick(ServerWorld& world, bool hostile, bool passive);

    // ========== 生成配置 ==========

    /**
     * @brief 设置生成距离（区块）
     */
    void setSpawnDistance(i32 chunks) { m_spawnDistance = chunks; }

    /**
     * @brief 获取生成距离
     */
    [[nodiscard]] i32 getSpawnDistance() const { return m_spawnDistance; }

    /**
     * @brief 设置玩家周围生成范围
     */
    void setSpawnRange(i32 range) { m_spawnRange = range; }

    /**
     * @brief 设置最大实体数量
     */
    void setMaxEntities(i32 max) { m_maxEntities = max; }

    // ========== 生成位置检查 ==========

    /**
     * @brief 注册生成位置检查函数
     */
    void registerPositionCheck(const String& entityTypeId, SpawnPositionCheck check) {
        m_positionChecks[entityTypeId] = std::move(check);
    }

private:
    i32 m_spawnDistance = 8;   // 生成距离（区块）
    i32 m_spawnRange = 20;      // 玩家周围生成范围（区块）
    i32 m_maxEntities = 200;    // 最大实体数量

    /// 位置检查函数映射
    std::unordered_map<String, SpawnPositionCheck> m_positionChecks;

    // ========== 内部方法 ==========

    /**
     * @brief 在指定位置尝试生成实体
     * @return 生成的实体数量
     */
    i32 trySpawnAt(ServerWorld& world, i32 x, i32 y, i32 z,
                   const SpawnEntry& entry, math::Random& random);

    /**
     * @brief 随机选择生成条目
     */
    [[nodiscard]] const SpawnEntry* selectEntry(
        const std::vector<SpawnEntry>& entries, math::Random& random) const;

    /**
     * @brief 检查位置是否可以生成
     */
    [[nodiscard]] bool canSpawnAt(ServerWorld& world, i32 x, i32 y, i32 z,
                                   const SpawnEntry& entry) const;

    /**
     * @brief 检查光照条件
     */
    [[nodiscard]] bool checkLightLevel(ServerWorld& world, i32 x, i32 y, i32 z,
                                        bool isMonster) const;

    /**
     * @brief 获取生成高度
     */
    [[nodiscard]] i32 getSpawnHeight(ServerWorld& world, i32 x, i32 z) const;
};

} // namespace world::spawn
} // namespace mr
