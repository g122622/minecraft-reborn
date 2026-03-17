#include "InternalLight.hpp"
#include <cmath>
#include <algorithm>

namespace mc {
namespace InternalLight {

// ============================================================================
// 天空减暗计算
// ============================================================================

i32 calculateSkyDarkening(i64 dayTime, bool isRaining, bool isThundering) {
    // 基础天空减暗（基于时间）
    i32 baseDarkening = calculateDefaultSkyDarkening(dayTime);

    // 天气影响
    if (isThundering) {
        // 雷暴时天空更暗
        baseDarkening = std::min(baseDarkening + 10, 11);
    } else if (isRaining) {
        // 下雨时天空变暗
        baseDarkening = std::min(baseDarkening + 3, 11);
    }

    return baseDarkening;
}

i32 calculateDefaultSkyDarkening(i64 dayTime) {
    f32 celestialAngle = getCelestialAngle(dayTime);
    return calculateSkyDarkeningFromAngle(celestialAngle);
}

i32 calculateSkyDarkeningFromAngle(f32 celestialAngle) {
    // 天空减暗范围: 0 (正午) 到 11 (午夜)
    // 使用余弦函数计算

    // 将角度转换为太阳高度
    // celestialAngle = 0.0 时日出，0.25 时正午，0.5 时日落，0.75 时午夜
    f32 sunAngle = celestialAngle * 2.0f * 3.14159265f;

    // 计算天空减暗
    // 正午时为0，午夜时最大
    f32 factor = 1.0f - std::cos(sunAngle);
    factor = (factor + 1.0f) / 2.0f;  // 归一化到0-1

    // 映射到0-11
    i32 darkening = static_cast<i32>(factor * 11.0f);
    return std::clamp(darkening, 0, 11);
}

// ============================================================================
// 内部光照计算
// ============================================================================

i32 calculateInternalLight(u8 blockLight, u8 skyLight) {
    return std::max(static_cast<i32>(blockLight), static_cast<i32>(skyLight));
}

i32 calculateRawBrightness(u8 blockLight, u8 skyLight, i32 skyDarkening) {
    // 天空光照减去减暗因子，但不低于0
    i32 effectiveSkyLight = static_cast<i32>(skyLight) - skyDarkening;
    effectiveSkyLight = std::max(0, effectiveSkyLight);

    // 取最大值
    return std::max(static_cast<i32>(blockLight), effectiveSkyLight);
}

// ============================================================================
// 生物生成条件
// ============================================================================

bool isDarkEnoughForSpawning(i32 rawBrightness) {
    // 亮度 <= 7 时可以生成敌对生物
    return rawBrightness <= 7;
}

bool isBrightEnoughToPreventSpawning(i32 rawBrightness) {
    // 亮度 > 7 时不能生成敌对生物
    return rawBrightness > 7;
}

bool canSleep(i32 rawBrightness, bool isThundering) {
    // 夜晚或雷暴时可以睡觉
    // 亮度 <= 7 表示夜晚
    return rawBrightness <= 7 || isThundering;
}

// ============================================================================
// 天体计算
// ============================================================================

f32 getCelestialAngle(i64 dayTime) {
    // 一天有24000 ticks
    // celestialAngle = 0.0 时日出
    // celestialAngle = 0.25 时正午
    // celestialAngle = 0.5 时日落
    // celestialAngle = 0.75 时午夜

    // 调整时间偏移（MC中0是日出而非午夜）
    i64 adjustedTime = (dayTime + 6000) % 24000;
    return static_cast<f32>(adjustedTime) / 24000.0f;
}

f32 getSunAngle(f32 celestialAngle) {
    // 太阳高度角
    // 0 = 地平线，正值 = 白天，负值 = 夜晚

    f32 angle = celestialAngle * 2.0f * 3.14159265f;
    return std::sin(angle);
}

bool isDaytime(i64 dayTime) {
    // 白天: 0 - 12000 (日出到日落)
    // 注意：MC中时间从日出开始
    i64 timeOfDay = dayTime % 24000;
    return timeOfDay < 12000;
}

bool isNighttime(i64 dayTime) {
    // 夜晚: 12000 - 24000 (日落到日出)
    i64 timeOfDay = dayTime % 24000;
    return timeOfDay >= 12000;
}

i32 getMoonPhase(i64 dayTime) {
    // 月相周期为8天
    // 每天有24000 ticks
    i64 dayNumber = dayTime / 24000;
    return static_cast<i32>(dayNumber % 8);
}

f32 getMoonBrightness(i32 moonPhase) {
    // 月相亮度：
    // 0: 满月 (1.0)
    // 1: 亏凸月 (0.75)
    // 2: 下弦月 (0.5)
    // 3: 亏月 (0.25)
    // 4: 新月 (0.0)
    // 5: 盈月 (0.25)
    // 6: 上弦月 (0.5)
    // 7: 盈凸月 (0.75)

    static const f32 PHASE_BRIGHTNESS[8] = {
        1.0f, 0.75f, 0.5f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f
    };

    return PHASE_BRIGHTNESS[moonPhase % 8];
}

} // namespace InternalLight
} // namespace mc
