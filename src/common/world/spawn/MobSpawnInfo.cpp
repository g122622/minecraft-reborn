#include "MobSpawnInfo.hpp"

namespace mc::world::spawn {

// ============================================================================
// 工厂方法实现（参考 MC 1.16.5 生物群系生成配置）
// ============================================================================

MobSpawnInfo MobSpawnInfo::createPlains() {
    // 参考 MC 1.16.5 PlainsBiome
    // creature_spawn_probability = 0.1F
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // 怪物（怪物在夜间/黑暗环境自然生成）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物（区块生成时放置）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:horse", 5, 2, 6));
    info.addCreatureSpawn(SpawnEntry("minecraft:donkey", 1, 1, 3));

    // 环境生物
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createForest() {
    // 参考 MC 1.16.5 ForestBiome
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物 - 森林有狼
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:wolf", 8, 4, 4));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createDesert() {
    // 参考 MC 1.16.5 DesertBiome
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 19, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 80, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:husk", 80, 4, 4)); // 沙漠僵尸
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物 - 沙漠只有兔子
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:rabbit", 4, 2, 3));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createOcean() {
    // 参考 MC 1.16.5 OceanBiome
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物 - 海洋有溺尸
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 100, 4, 4));

    // 水生生物
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:cod", 10, 4, 7));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:salmon", 5, 3, 5));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 10, 4, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createTaiga() {
    // 参考 MC 1.16.5 TaigaBiome
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物 - 针叶林有狐狸和狼
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:wolf", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:fox", 8, 2, 4));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createJungle() {
    // 参考 MC 1.16.5 JungleBiome
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:ocelot", 2, 1, 1)); // 丛林也有豹猫怪物

    // 动物 - 丛林有鹦鹉和熊猫
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:parrot", 10, 1, 2));
    info.addCreatureSpawn(SpawnEntry("minecraft:panda", 1, 1, 2));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSavanna() {
    // 参考 MC 1.16.5 SavannaBiome
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物 - 热带草原有马、驴、羊驼
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:horse", 1, 2, 6));
    info.addCreatureSpawn(SpawnEntry("minecraft:donkey", 1, 1, 1));
    info.addCreatureSpawn(SpawnEntry("minecraft:llama", 8, 4, 4));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSwamp() {
    // 参考 MC 1.16.5 SwampBiome
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物 - 沼泽有更多女巫
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 50, 1, 1)); // 沼泽女巫更多
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 1, 1)); // 沼泽史莱姆

    // 动物
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createMountains() {
    // 参考 MC 1.16.5 MountainsBiome
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物 - 山地有羊驼
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:llama", 5, 4, 6));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSnowy() {
    // 参考 MC 1.16.5 SnowyBiome（雪地平原等）
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物 - 雪地有流浪者
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 20, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:stray", 80, 4, 4)); // 流浪者
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物 - 雪地有北极熊和兔子
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:rabbit", 10, 2, 3));
    info.addCreatureSpawn(SpawnEntry("minecraft:polar_bear", 1, 1, 2));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createEmpty() {
    // 空生成信息，用于没有生物生成的生物群系（如虚空）
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;
    return info;
}

} // namespace mc::world::spawn
