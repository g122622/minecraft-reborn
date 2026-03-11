#pragma once

#include "../../core/Types.hpp"
#include <vector>
#include <string>

namespace mr::world::spawn {

/**
 * @brief 生成成本
 *
 * 定义在特定位置生成实体的成本，
 * 用于限制高密度区域内的实体数量。
 *
 * 参考 MC 1.16.5 SpawnCosts
 */
struct SpawnCosts {
    /// 基础成本（单个实体的成本）
    f64 entityCost = 1.0;

    /// 最大成本上限（超过此值不再生成）
    f64 maxCost = 0.0;

    SpawnCosts() = default;
    SpawnCosts(f64 entity, f64 max) : entityCost(entity), maxCost(max) {}
};

/**
 * @brief 生成权重条目
 *
 * 定义单个实体类型的生成配置。
 *
 * 参考 MC 1.16.5 MobSpawnInfo.SpawnCosts
 */
struct SpawnEntry {
    /// 实体类型ID（字符串标识符）
    String entityTypeId;

    /// 生成权重（越高越容易被选中）
    i32 weight = 0;

    /// 最小生成数量
    i32 minCount = 1;

    /// 最大生成数量
    i32 maxCount = 4;

    /// 生成成本（可选）
    SpawnCosts costs;

    SpawnEntry() = default;

    SpawnEntry(String typeId, i32 w, i32 minC, i32 maxC)
        : entityTypeId(std::move(typeId))
        , weight(w)
        , minCount(minC)
        , maxCount(maxC)
    {}

    SpawnEntry(String typeId, i32 w, i32 minC, i32 maxC, SpawnCosts c)
        : entityTypeId(std::move(typeId))
        , weight(w)
        , minCount(minC)
        , maxCount(maxC)
        , costs(c)
    {}
};

/**
 * @brief 生成分类信息
 *
 * 定义特定实体分类（怪物、动物、环境等）的生成配置。
 *
 * 参考 MC 1.16.5 MobSpawnInfo.SpawnCosts
 */
struct SpawnCategory {
    /// 实体分类（怪物、动物、环境等）
    // TODO: 使用 EntityClassification enum

    /// 该分类的生成条目列表
    std::vector<SpawnEntry> entries;

    /// 该分类的最大实例数
    i32 maxInstances = 0;

    /// 是否启用
    bool enabled = true;

    SpawnCategory() = default;

    void addEntry(const SpawnEntry& entry) {
        entries.push_back(entry);
    }

    void addEntry(SpawnEntry&& entry) {
        entries.push_back(std::move(entry));
    }
};

/**
 * @brief 生物群系实体生成信息
 *
 * 定义在特定生物群系中可以生成的实体类型及其配置。
 *
 * 参考 MC 1.16.5 MobSpawnInfo
 */
class MobSpawnInfo {
public:
    MobSpawnInfo() = default;

    // ========== 生成条目管理 ==========

    /**
     * @brief 添加怪物生成条目
     */
    void addMonsterSpawn(const SpawnEntry& entry) {
        m_monsters.addEntry(entry);
    }

    /**
     * @brief 添加动物生成条目
     */
    void addCreatureSpawn(const SpawnEntry& entry) {
        m_creatures.addEntry(entry);
    }

    /**
     * @brief 添加环境生物生成条目（蝙蝠等）
     */
    void addAmbientSpawn(const SpawnEntry& entry) {
        m_ambient.addEntry(entry);
    }

    /**
     * @brief 添加水生生物生成条目
     */
    void addWaterCreatureSpawn(const SpawnEntry& entry) {
        m_waterCreatures.addEntry(entry);
    }

    /**
     * @brief 添加其他生成条目
     */
    void addMiscSpawn(const SpawnEntry& entry) {
        m_misc.addEntry(entry);
    }

    // ========== 获取生成条目 ==========

    [[nodiscard]] const std::vector<SpawnEntry>& getMonsterSpawns() const {
        return m_monsters.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getCreatureSpawns() const {
        return m_creatures.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getAmbientSpawns() const {
        return m_ambient.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getWaterCreatureSpawns() const {
        return m_waterCreatures.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getMiscSpawns() const {
        return m_misc.entries;
    }

    // ========== 分类配置 ==========

    [[nodiscard]] i32 getMaxMonsterInstances() const { return m_monsters.maxInstances; }
    void setMaxMonsterInstances(i32 max) { m_monsters.maxInstances = max; }

    [[nodiscard]] i32 getMaxCreatureInstances() const { return m_creatures.maxInstances; }
    void setMaxCreatureInstances(i32 max) { m_creatures.maxInstances = max; }

    // ========== 工厂方法 ==========

    /**
     * @brief 创建平原生物群系的生成信息
     */
    static MobSpawnInfo createPlains();

    /**
     * @brief 创建森林生物群系的生成信息
     */
    static MobSpawnInfo createForest();

    /**
     * @brief 创建沙漠生物群系的生成信息
     */
    static MobSpawnInfo createDesert();

    /**
     * @brief 创建海洋生物群系的生成信息
     */
    static MobSpawnInfo createOcean();

private:
    SpawnCategory m_monsters;        // 怪物（僵尸、骷髅等）
    SpawnCategory m_creatures;       // 动物（猪、牛、羊等）
    SpawnCategory m_ambient;         // 环境生物（蝙蝠）
    SpawnCategory m_waterCreatures;  // 水生生物（鱼、鱿鱼）
    SpawnCategory m_misc;            // 其他

    // 默认实例限制
    static constexpr i32 DEFAULT_MAX_MONSTERS = 70;
    static constexpr i32 DEFAULT_MAX_CREATURES = 10;
    static constexpr i32 DEFAULT_MAX_AMBIENT = 15;
    static constexpr i32 DEFAULT_MAX_WATER_CREATURES = 5;
};

} // namespace mr::world::spawn
