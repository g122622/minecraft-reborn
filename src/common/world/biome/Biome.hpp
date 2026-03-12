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
// 预定义生物群系ID常量（参考 MC 1.16.5）
// ============================================================================

namespace Biomes {

// 基础生物群系
constexpr BiomeId Ocean = 0;
constexpr BiomeId Plains = 1;
constexpr BiomeId Desert = 2;
constexpr BiomeId Mountains = 3;
constexpr BiomeId Forest = 4;
constexpr BiomeId Taiga = 5;
constexpr BiomeId Swamp = 6;
constexpr BiomeId River = 7;
constexpr BiomeId NetherWastes = 8;
constexpr BiomeId TheEnd = 9;
constexpr BiomeId FrozenOcean = 10;
constexpr BiomeId FrozenRiver = 11;
constexpr BiomeId SnowyPlains = 12;
constexpr BiomeId SnowyMountains = 13;
constexpr BiomeId MushroomFields = 14;
constexpr BiomeId Beach = 15;
constexpr BiomeId Jungle = 16;
constexpr BiomeId Savanna = 17;
constexpr BiomeId Badlands = 18;

// 海洋变体
constexpr BiomeId DeepOcean = 19;
constexpr BiomeId WarmOcean = 20;
constexpr BiomeId ColdOcean = 21;
constexpr BiomeId LukewarmOcean = 22;
constexpr BiomeId DeepWarmOcean = 23;
constexpr BiomeId DeepColdOcean = 24;
constexpr BiomeId DeepLukewarmOcean = 25;
constexpr BiomeId DeepFrozenOcean = 26;

// 山地变体
constexpr BiomeId WoodedHills = 27;
constexpr BiomeId DesertHills = 28;
constexpr BiomeId MountainEdge = 29;
constexpr BiomeId StoneShore = 30;
constexpr BiomeId SnowyBeach = 31;

// 森林变体
constexpr BiomeId BirchForest = 32;
constexpr BiomeId DarkForest = 33;
constexpr BiomeId SnowyTaiga = 34;
constexpr BiomeId GiantTreeTaiga = 35;
constexpr BiomeId WoodedMountains = 36;

// 热带/沙漠变体
constexpr BiomeId SavannaPlateau = 37;
constexpr BiomeId BadlandsPlateau = 38;
constexpr BiomeId WoodedBadlandsPlateau = 39;
constexpr BiomeId ErodedBadlands = 40;
constexpr BiomeId ShatteredSavanna = 41;

// 生物群系总数
constexpr BiomeId Count = 42;

} // namespace Biomes

} // namespace mc
