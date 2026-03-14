#include "BiomeValues.hpp"
#include <unordered_map>

namespace mc {
namespace layer {

// ============================================================================
// 生物群系类别映射（用于 areBiomesSimilar）
// 参考 MC 1.16.5 LayerUtil.Type
// ============================================================================

namespace {

/**
 * @brief 生物群系类别枚举
 */
enum class BiomeCategory : i32 {
    None = 0,
    Taiga,
    ExtremeHills,
    Jungle,
    Mesa,
    BadlandsPlateau,
    Plains,
    Savanna,
    Icy,
    Beach,
    Forest,
    Ocean,
    Desert,
    River,
    Swamp,
    Mushroom
};

// 生物群系 ID 到类别的映射
// 参考 MC LayerUtil.field_242937_a
const std::unordered_map<i32, BiomeCategory>& getBiomeCategoryMap() {
    static const std::unordered_map<i32, BiomeCategory> map = {
        // Beach
        {BiomeValues::Beach, BiomeCategory::Beach},
        {BiomeValues::MushroomFieldShore, BiomeCategory::Beach},

        // Desert
        {BiomeValues::Desert, BiomeCategory::Desert},
        {BiomeValues::Savanna, BiomeCategory::Desert},
        {BiomeValues::DesertLakes, BiomeCategory::Desert},

        // ExtremeHills (Mountains)
        {BiomeValues::GravellyMountains, BiomeCategory::ExtremeHills},
        {BiomeValues::WoodedMountains, BiomeCategory::ExtremeHills},
        {BiomeValues::Mountains, BiomeCategory::ExtremeHills},
        {BiomeValues::SnowyTaigaMountains, BiomeCategory::ExtremeHills},
        {BiomeValues::ModifiedGravellyMountains, BiomeCategory::ExtremeHills},

        // Forest
        {BiomeValues::WoodedHills, BiomeCategory::Forest},
        {BiomeValues::BirchForestHills, BiomeCategory::Forest},
        {BiomeValues::DarkForestHills, BiomeCategory::Forest},
        {BiomeValues::FlowerForest, BiomeCategory::Forest},
        {BiomeValues::BirchForest, BiomeCategory::Forest},
        {BiomeValues::TallBirchForest, BiomeCategory::Forest},
        {BiomeValues::TallBirchHills, BiomeCategory::Forest},
        {BiomeValues::Forest, BiomeCategory::Forest},
        {BiomeValues::DarkForest, BiomeCategory::Forest},
        {BiomeValues::Jungle, BiomeCategory::Forest},

        // Icy (Snowy)
        {BiomeValues::IceSpikes, BiomeCategory::Icy},
        {BiomeValues::SnowyMountains, BiomeCategory::Icy},
        {BiomeValues::SnowyPlains, BiomeCategory::Icy},

        // Jungle
        {BiomeValues::BambooJungle, BiomeCategory::Jungle},
        {BiomeValues::BambooJungleHills, BiomeCategory::Jungle},
        {BiomeValues::JungleHills, BiomeCategory::Jungle},
        {BiomeValues::JungleEdge, BiomeCategory::Jungle},
        {BiomeValues::ModifiedJungle, BiomeCategory::Jungle},
        {BiomeValues::ModifiedJungleEdge, BiomeCategory::Jungle},

        // Mesa (Badlands)
        {BiomeValues::SavannaPlateau, BiomeCategory::Mesa},
        {BiomeValues::ShatteredSavannaPlateau, BiomeCategory::Mesa},
        {BiomeValues::ShatteredSavanna, BiomeCategory::Mesa},
        {BiomeValues::ErodedBadlands, BiomeCategory::Mesa},
        {BiomeValues::ModifiedWoodedBadlandsPlateau, BiomeCategory::Mesa},
        {BiomeValues::ModifiedBadlandsPlateau, BiomeCategory::Mesa},

        // BadlandsPlateau
        {BiomeValues::WoodedBadlandsPlateau, BiomeCategory::BadlandsPlateau},
        {BiomeValues::BadlandsPlateau, BiomeCategory::BadlandsPlateau},

        // Mushroom
        {BiomeValues::MushroomFields, BiomeCategory::Mushroom},
        {BiomeValues::MushroomFieldShore, BiomeCategory::Mushroom},

        // Ocean
        {BiomeValues::ColdOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepColdOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepFrozenOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepLukewarmOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepWarmOcean, BiomeCategory::Ocean},
        {BiomeValues::FrozenOcean, BiomeCategory::Ocean},
        {BiomeValues::LukewarmOcean, BiomeCategory::Ocean},
        {BiomeValues::Ocean, BiomeCategory::Ocean},
        {BiomeValues::WarmOcean, BiomeCategory::Ocean},

        // Plains
        {BiomeValues::Plains, BiomeCategory::Plains},
        {BiomeValues::SunflowerPlains, BiomeCategory::Plains},

        // River
        {BiomeValues::FrozenRiver, BiomeCategory::River},
        {BiomeValues::River, BiomeCategory::River},

        // Savanna
        {BiomeValues::Savanna, BiomeCategory::Savanna},
        {BiomeValues::SavannaPlateau, BiomeCategory::Savanna},
        {BiomeValues::ShatteredSavanna, BiomeCategory::Savanna},
        {BiomeValues::ShatteredSavannaPlateau, BiomeCategory::Savanna},

        // Swamp
        {BiomeValues::Swamp, BiomeCategory::Swamp},
        {BiomeValues::SwampHills, BiomeCategory::Swamp},

        // Taiga
        {BiomeValues::GiantSpruceTaiga, BiomeCategory::Taiga},
        {BiomeValues::GiantSpruceTaigaHills, BiomeCategory::Taiga},
        {BiomeValues::BirchForest, BiomeCategory::Taiga},
        {BiomeValues::BirchForestHills, BiomeCategory::Taiga},
        {BiomeValues::SnowyTaiga, BiomeCategory::Taiga},
        {BiomeValues::SnowyTaigaHills, BiomeCategory::Taiga},
        {BiomeValues::GiantTreeTaiga, BiomeCategory::Taiga},
        {BiomeValues::GiantTreeTaigaHills, BiomeCategory::Taiga},
        {BiomeValues::TaigaMountains, BiomeCategory::Taiga},
        {BiomeValues::Taiga, BiomeCategory::Taiga},
        {BiomeValues::SnowyTaigaMountains, BiomeCategory::Taiga},
        {BiomeValues::StoneShore, BiomeCategory::Taiga},
        {BiomeValues::SnowyBeach, BiomeCategory::Taiga},

        // Desert (没有在上面 Desert 的)
        {BiomeValues::TheVoid, BiomeCategory::None},
    };
    return map;
}

BiomeCategory getBiomeCategory(i32 biome) {
    const auto& map = getBiomeCategoryMap();
    auto it = map.find(biome);
    if (it != map.end()) {
        return it->second;
    }
    return BiomeCategory::None;
}

} // anonymous namespace

// ============================================================================
// areBiomesSimilar 实现
// ============================================================================

bool BiomeValues::areBiomesSimilar(i32 a, i32 b) {
    if (a == b) {
        return true;
    }
    return getBiomeCategory(a) == getBiomeCategory(b);
}

} // namespace layer
} // namespace mc
