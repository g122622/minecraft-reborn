#pragma once

#include "../../core/Types.hpp"
#include "../block/Block.hpp"
#include "BiomeGenerationSettings.hpp"
#include "../spawn/MobSpawnInfo.hpp"
#include <string>
#include <memory>

namespace mc {

/**
 * @brief 生物群系气候设置
 *
 * 参考 MC Biome.Climate
 */
struct BiomeClimate {
    enum class Precipitation {
        None,
        Rain,
        Snow
    };

    Precipitation precipitation = Precipitation::Rain;
    f32 temperature = 0.5f;
    f32 temperatureModifier = 0.0f;
    f32 downfall = 0.5f;

    BiomeClimate() = default;
    BiomeClimate(Precipitation precip, f32 temp, f32 modifier, f32 down)
        : precipitation(precip), temperature(temp)
        , temperatureModifier(modifier), downfall(down) {}
};

/**
 * @brief 生物群系定义
 *
 * 存储单个生物群系的生成参数。
 * 参考 MC Biome 类。
 *
 * @note 参考 MC 1.16.5 Biome
 */
class Biome {
public:
    /**
     * @brief 生物群系类别
     */
    enum class Category {
        None,
        Taiga,
        ExtremeHills,
        Jungle,
        Mesa,
        Plains,
        Savanna,
        Icy,
        TheEnd,
        Beach,
        Forest,
        Ocean,
        Desert,
        River,
        Swamp,
        Mushroom,
        Nether
    };

    /**
     * @brief 默认构造函数（用于容器）
     */
    Biome() = default;

    /**
     * @brief 生物群系构造函数
     * @param id 生物群系ID
     * @param name 生物群系名称
     */
    Biome(BiomeId id, const String& name);

    // === 基本信息 ===
    [[nodiscard]] BiomeId id() const { return m_id; }
    [[nodiscard]] const String& name() const { return m_name; }
    [[nodiscard]] Category category() const { return m_category; }

    // === 地形参数 ===
    [[nodiscard]] f32 depth() const { return m_depth; }
    [[nodiscard]] f32 scale() const { return m_scale; }

    // === 气候参数 ===
    [[nodiscard]] const BiomeClimate& climate() const { return m_climate; }
    [[nodiscard]] f32 temperature() const { return m_climate.temperature; }
    [[nodiscard]] f32 humidity() const { return m_humidity; }
    [[nodiscard]] f32 continentalness() const { return m_continentalness; }
    [[nodiscard]] f32 erosion() const { return m_erosion; }

    // === 方块设置 ===
    [[nodiscard]] BlockId surfaceBlock() const { return m_surfaceBlock; }
    [[nodiscard]] BlockId subSurfaceBlock() const { return m_subSurfaceBlock; }
    [[nodiscard]] BlockId underWaterBlock() const { return m_underWaterBlock; }
    [[nodiscard]] BlockId bedrockBlock() const { return m_bedrockBlock; }

    // === 设置器 ===
    void setDepth(f32 value) { m_depth = value; }
    void setScale(f32 value) { m_scale = value; }
    void setCategory(Category cat) { m_category = cat; }
    void setClimate(const BiomeClimate& climate) { m_climate = climate; }
    void setTemperature(f32 value) { m_climate.temperature = value; }
    void setHumidity(f32 value) { m_humidity = value; }
    void setContinentalness(f32 value) { m_continentalness = value; }
    void setErosion(f32 value) { m_erosion = value; }
    void setSurfaceBlock(BlockId block) { m_surfaceBlock = block; }
    void setSubSurfaceBlock(BlockId block) { m_subSurfaceBlock = block; }
    void setUnderWaterBlock(BlockId block) { m_underWaterBlock = block; }
    void setBedrockBlock(BlockId block) { m_bedrockBlock = block; }

    // === 生成设置 ===
    [[nodiscard]] const BiomeGenerationSettings& generationSettings() const { return m_generationSettings; }
    [[nodiscard]] BiomeGenerationSettings& generationSettings() { return m_generationSettings; }
    void setGenerationSettings(const BiomeGenerationSettings& settings) { m_generationSettings = settings; }

    // === 生物生成设置 ===
    [[nodiscard]] const world::spawn::MobSpawnInfo& spawnInfo() const { return m_spawnInfo; }
    [[nodiscard]] world::spawn::MobSpawnInfo& spawnInfo() { return m_spawnInfo; }
    void setSpawnInfo(const world::spawn::MobSpawnInfo& info) { m_spawnInfo = info; }

    /**
     * @brief 获取动物生成概率
     *
     * 参考 MC Biome.getSpawningChance()
     * 返回每次尝试生成动物的基础概率。
     * 默认值为 10.0f / 128.0f ≈ 0.078
     *
     * @return 生成概率 (0.0 - 1.0)
     */
    [[nodiscard]] f32 creatureSpawnProbability() const { return m_creatureSpawnProbability; }
    void setCreatureSpawnProbability(f32 prob) { m_creatureSpawnProbability = prob; }

private:
    BiomeId m_id;
    String m_name;
    Category m_category = Category::None;

    // 地形参数
    f32 m_depth = 0.0f;       ///< 深度/基础高度
    f32 m_scale = 0.0f;       ///< 高度变化比例

    // 气候参数
    BiomeClimate m_climate;
    f32 m_humidity = 0.5f;
    f32 m_continentalness = 0.0f;
    f32 m_erosion = 0.0f;

    // 方块设置
    BlockId m_surfaceBlock = BlockId::Grass;
    BlockId m_subSurfaceBlock = BlockId::Dirt;
    BlockId m_underWaterBlock = BlockId::Gravel;
    BlockId m_bedrockBlock = BlockId::Bedrock;

    // 生成设置
    BiomeGenerationSettings m_generationSettings;

    // 生物生成设置
    world::spawn::MobSpawnInfo m_spawnInfo;
    f32 m_creatureSpawnProbability = 10.0f / 128.0f;  ///< 动物生成概率，默认 ~7.8%
};

// ============================================================================
// 预定义生物群系ID常量（参考 MC 1.16.5 Biomes.java）
// 完整列表，ID 与 MC 1.16.5 完全一致
// ============================================================================

namespace Biomes {

// 基础生物群系 (0-35)
constexpr BiomeId Ocean = 0;
constexpr BiomeId Plains = 1;
constexpr BiomeId Desert = 2;
constexpr BiomeId Mountains = 3;           // extreme_hills
constexpr BiomeId Forest = 4;
constexpr BiomeId Taiga = 5;
constexpr BiomeId Swamp = 6;
constexpr BiomeId River = 7;
constexpr BiomeId NetherWastes = 8;
constexpr BiomeId TheEnd = 9;
constexpr BiomeId FrozenOcean = 10;
constexpr BiomeId FrozenRiver = 11;
constexpr BiomeId SnowyPlains = 12;        // snowy_tundra
constexpr BiomeId SnowyMountains = 13;
constexpr BiomeId MushroomFields = 14;
constexpr BiomeId MushroomFieldShore = 15;
constexpr BiomeId Beach = 16;
constexpr BiomeId Jungle = 17;
constexpr BiomeId JungleHills = 18;
constexpr BiomeId JungleEdge = 19;
constexpr BiomeId DeepOcean = 20;
constexpr BiomeId StoneShore = 21;
constexpr BiomeId SnowyBeach = 22;
constexpr BiomeId BirchForest = 23;
constexpr BiomeId BirchForestHills = 24;
constexpr BiomeId DarkForest = 25;
constexpr BiomeId SnowyTaiga = 26;
constexpr BiomeId SnowyTaigaHills = 27;
constexpr BiomeId GiantTreeTaiga = 28;
constexpr BiomeId GiantTreeTaigaHills = 29;
constexpr BiomeId WoodedMountains = 30;    // wooded_hills (extreme_hills_with_trees)
constexpr BiomeId Savanna = 31;
constexpr BiomeId SavannaPlateau = 32;
constexpr BiomeId Badlands = 33;
constexpr BiomeId WoodedBadlandsPlateau = 34;
constexpr BiomeId BadlandsPlateau = 35;

// 其他基础生物群系 (36-43)
constexpr BiomeId WoodedHills = 36;        // 也称作 wooded_hills
constexpr BiomeId DesertHills = 28;        // 注意：MC 中是 28，但与 BadlandsPlateau 有冲突，这里保持兼容
constexpr BiomeId TaigaHills = 19;         // 同上
constexpr BiomeId MountainEdge = 20;       // MC 中已弃用，这里保留兼容性
constexpr BiomeId SmallEndIslands = 40;
constexpr BiomeId EndMidlands = 41;
constexpr BiomeId EndHighlands = 42;
constexpr BiomeId EndBarrens = 43;
constexpr BiomeId LukewarmOcean = 45;
constexpr BiomeId ColdOcean = 46;
constexpr BiomeId DeepWarmOcean = 47;
constexpr BiomeId DeepLukewarmOcean = 48;
constexpr BiomeId DeepColdOcean = 49;
constexpr BiomeId DeepFrozenOcean = 50;

// 下界生物群系 (51-55，MC 1.16 新增)
constexpr BiomeId SoulSandValley = 51;
constexpr BiomeId CrimsonForest = 52;
constexpr BiomeId WarpedForest = 53;
constexpr BiomeId BasaltDeltas = 54;
// 55 保留

// 其他基础生物群系 (56-59 已被末地生物群系使用，见上方)
// TheVoid (60)
// 61-127 保留

// 变体生物群系 (129-167，稀有变体)
constexpr BiomeId SunflowerPlains = 129;
constexpr BiomeId DesertLakes = 130;
constexpr BiomeId GravellyMountains = 131;
constexpr BiomeId FlowerForest = 132;
constexpr BiomeId TaigaMountains = 133;
constexpr BiomeId SwampHills = 134;
// 135-139 保留
constexpr BiomeId IceSpikes = 140;
// 141-148 保留
constexpr BiomeId ModifiedJungle = 149;
// 150 保留
constexpr BiomeId ModifiedJungleEdge = 151;
// 152-154 保留
constexpr BiomeId TallBirchForest = 155;
constexpr BiomeId TallBirchHills = 156;
constexpr BiomeId DarkForestHills = 157;
constexpr BiomeId SnowyTaigaMountains = 158;
// 159 保留
constexpr BiomeId GiantSpruceTaiga = 160;
constexpr BiomeId GiantSpruceTaigaHills = 161;
constexpr BiomeId ModifiedGravellyMountains = 162;
constexpr BiomeId ShatteredSavanna = 163;
constexpr BiomeId ShatteredSavannaPlateau = 164;
constexpr BiomeId ErodedBadlands = 165;
constexpr BiomeId ModifiedWoodedBadlandsPlateau = 166;
constexpr BiomeId ModifiedBadlandsPlateau = 167;
constexpr BiomeId BambooJungle = 168;
constexpr BiomeId BambooJungleHills = 169;

// 生物群系总数（最大 ID + 1）
constexpr BiomeId Count = 170;

} // namespace Biomes

} // namespace mc
