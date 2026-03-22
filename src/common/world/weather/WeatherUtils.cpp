#include "WeatherUtils.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../../world/biome/Biome.hpp"
#include "../../util/math/random/Random.hpp"
#include <cmath>

namespace mc::weather {

bool WeatherUtils::canSeeSky(const mc::IWorld& world, const mc::BlockPos& pos) {
    // 检查该位置上方是否有非透明方块
    // 简化实现：检查高度是否为最高点
    i32 height = world.getHeight(pos.x, pos.z);
    return pos.y >= height;
}

bool WeatherUtils::canRainAt(const mc::IWorld& world, const mc::BlockPos& pos) {
    // 检查生物群系是否允许降雨
    // 目前简化实现，后续需要获取生物群系信息
    // 参考 MC: Biome.getPrecipitation() == RainType.RAIN

    // 检查是否可以看到天空
    if (!canSeeSky(world, pos)) {
        return false;
    }

    // TODO: 获取生物群系并检查降水类型
    // const Biome* biome = world.getBiome(pos);
    // if (!biome || biome->climate().precipitation != BiomeClimate::Precipitation::Rain) {
    //     return false;
    // }

    return true;
}

bool WeatherUtils::canSnowAt(const mc::IWorld& world, const mc::BlockPos& pos) {
    // 检查是否可以看到天空
    if (!canSeeSky(world, pos)) {
        return false;
    }

    // TODO: 获取生物群系并检查温度
    // const Biome* biome = world.getBiome(pos);
    // if (!biome || biome->temperature() > 0.15f) {
    //     return false;
    // }

    return true;
}

i32 WeatherUtils::getRandomWeatherDuration(mc::math::IRandom& rng, i32 minTime, i32 maxTime) {
    if (minTime >= maxTime) {
        return minTime;
    }
    i32 range = maxTime - minTime;
    return minTime + rng.nextInt(range);
}

i32 WeatherUtils::getRandomClearDuration(mc::math::IRandom& rng) {
    return getRandomWeatherDuration(rng,
        WeatherConstants::MIN_CLEAR_TIME,
        WeatherConstants::MAX_CLEAR_TIME);
}

i32 WeatherUtils::getRandomRainDuration(mc::math::IRandom& rng) {
    return getRandomWeatherDuration(rng,
        WeatherConstants::MIN_RAIN_TIME,
        WeatherConstants::MAX_RAIN_TIME);
}

i32 WeatherUtils::getRandomThunderDuration(mc::math::IRandom& rng) {
    return getRandomWeatherDuration(rng,
        WeatherConstants::MIN_THUNDER_TIME,
        WeatherConstants::MAX_THUNDER_TIME);
}

f32 WeatherUtils::calculateStarBrightness(f32 rainStrength, i64 dayTime) {
    // 星星只在夜晚可见
    // 夜晚时间: 约 12542 - 23459 (日落到日出)
    if (dayTime < 12542 || dayTime > 23459) {
        return 0.0f;
    }

    // 计算夜晚深度 (0.0 - 1.0)
    // 最暗时约在 18000 (午夜)
    f32 nightDepth = 0.0f;
    if (dayTime < 18000) {
        // 日落到午夜 (12542 -> 18000)
        nightDepth = static_cast<f32>(dayTime - 12542) / (18000 - 12542);
    } else {
        // 午夜到日出 (18000 -> 23459)
        nightDepth = static_cast<f32>(23459 - dayTime) / (23459 - 18000);
    }

    // 降雨时星星不可见
    return nightDepth * (1.0f - rainStrength);
}

} // namespace mc::weather
