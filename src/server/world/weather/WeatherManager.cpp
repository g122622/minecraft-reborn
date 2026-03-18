#include "WeatherManager.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/math/random/Random.hpp"
#include <algorithm>

namespace mc::server {

WeatherManager::WeatherManager()
    : m_random(std::make_unique<mc::math::Random>(0)) {
}

WeatherManager::~WeatherManager() = default;

void WeatherManager::initialize(u64 seed) {
    m_random = std::make_unique<mc::math::Random>(seed);

    // 初始化天气状态
    // 初始为晴天，设置随机时间
    m_state.clearWeatherTime = 0;
    m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
    m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);
    m_state.raining = false;
    m_state.thundering = false;
    m_state.rainStrength = 0.0f;
    m_state.thunderStrength = 0.0f;
    m_state.prevRainStrength = 0.0f;
    m_state.prevThunderStrength = 0.0f;
    m_state.weatherCycleEnabled = true;

    m_weatherChanged = false;
    m_strengthChanged = false;
}

void WeatherManager::tick() {
    // 重置变化标志
    m_weatherChanged = false;
    m_strengthChanged = false;

    // 保存上一帧强度
    m_state.prevRainStrength = m_state.rainStrength;
    m_state.prevThunderStrength = m_state.thunderStrength;

    // 处理天气周期
    if (m_state.weatherCycleEnabled) {
        tickWeatherCycle();
    }

    // 更新强度渐变
    updateStrength();

    // 检查天气变化
    checkWeatherChange();
}

void WeatherManager::tickWeatherCycle() {
    // 参考 MC 1.16.5 ServerWorld.tick() 中的天气逻辑

    // 处理晴天计时器
    if (m_state.clearWeatherTime > 0) {
        --m_state.clearWeatherTime;
        // 强制晴天时，重置降雨/雷暴计时器
        m_state.rainTime = m_state.raining ? 0 : 1;
        m_state.thunderTime = m_state.thundering ? 0 : 1;
        m_state.raining = false;
        m_state.thundering = false;
    } else {
        // 处理雷暴计时器
        if (m_state.thunderTime > 0) {
            --m_state.thunderTime;
            if (m_state.thunderTime == 0) {
                // 切换雷暴状态
                m_state.thundering = !m_state.thundering;
            }
        } else if (m_state.thundering) {
            // 雷暴结束，设置新的雷暴间隔
            m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);
        } else {
            // 晴天，设置到下次雷暴的时间
            m_state.thunderTime = m_random->nextInt(
                mc::weather::WeatherConstants::MIN_THUNDER_TIME,
                mc::weather::WeatherConstants::MAX_THUNDER_TIME);
        }

        // 处理降雨计时器
        if (m_state.rainTime > 0) {
            --m_state.rainTime;
            if (m_state.rainTime == 0) {
                // 切换降雨状态
                m_state.raining = !m_state.raining;
            }
        } else if (m_state.raining) {
            // 降雨结束，设置到下次降雨的时间
            m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
        } else {
            // 晴天，设置到下次降雨的时间
            m_state.rainTime = m_random->nextInt(
                mc::weather::WeatherConstants::MIN_RAIN_TIME,
                mc::weather::WeatherConstants::MAX_RAIN_TIME);
        }
    }
}

void WeatherManager::updateStrength() {
    // 参考 MC 1.16.5 ServerWorld.tick() 中的强度渐变逻辑
    // 每tick变化 ±0.01

    // 更新降雨强度
    if (m_state.raining) {
        m_state.rainStrength = std::min(1.0f, m_state.rainStrength + mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
    } else {
        m_state.rainStrength = std::max(0.0f, m_state.rainStrength - mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
    }

    // 更新雷暴强度（依赖于降雨）
    if (m_state.thundering && m_state.raining) {
        m_state.thunderStrength = std::min(1.0f, m_state.thunderStrength + mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
    } else {
        m_state.thunderStrength = std::max(0.0f, m_state.thunderStrength - mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
    }

    // 检查强度是否变化
    if (m_state.rainStrength != m_state.prevRainStrength ||
        m_state.thunderStrength != m_state.prevThunderStrength) {
        m_strengthChanged = true;
    }
}

void WeatherManager::checkWeatherChange() {
    // 检查降雨状态变化
    bool currentlyRaining = m_state.isRaining();

    if (currentlyRaining != m_lastRaining) {
        m_weatherChanged = true;
        if (m_weatherChangeCallback) {
            WeatherType oldType = m_lastRaining ?
                (m_state.isThundering() ? WeatherType::Thunder : WeatherType::Rain) :
                WeatherType::Clear;
            WeatherType newType = currentlyRaining ?
                (m_state.isThundering() ? WeatherType::Thunder : WeatherType::Rain) :
                WeatherType::Clear;
            m_weatherChangeCallback(oldType, newType);
        }
        m_lastRaining = currentlyRaining;
    }
}

void WeatherManager::setClear(i32 duration) {
    // 参考 MC 1.16.5 WeatherCommand
    // /weather clear [duration] - duration 单位是秒，乘以 20 转换为 ticks
    if (duration <= 0) {
        duration = mc::weather::WeatherConstants::DEFAULT_COMMAND_DURATION;
    }

    m_state.clearWeatherTime = duration;
    m_state.raining = false;
    m_state.thundering = false;
    m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
    m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);

    // 注意：设置 clearWeatherTime 后，tick() 会处理计时器递减
    // 强度会自然渐变到 0
}

void WeatherManager::setRain(i32 duration) {
    if (duration <= 0) {
        duration = mc::weather::WeatherConstants::DEFAULT_COMMAND_DURATION;
    }

    m_state.clearWeatherTime = 0;
    m_state.raining = true;
    m_state.thundering = false;
    m_state.rainTime = duration;
    m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);

    // 强度会自然渐变到 1
}

void WeatherManager::setThunder(i32 duration) {
    if (duration <= 0) {
        duration = mc::weather::WeatherConstants::DEFAULT_COMMAND_DURATION;
    }

    // 雷暴需要同时启用降雨
    m_state.clearWeatherTime = 0;
    m_state.raining = true;
    m_state.thundering = true;
    m_state.rainTime = duration;
    m_state.thunderTime = duration;

    // 注意：/weather thunder 命令会使 rainTime 和 thunderTime 相同
    // 这样雷暴结束后晴天会开始
}

void WeatherManager::resetWeather() {
    m_state.resetWeather();

    // 设置新的随机计时器
    m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
    m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);

    m_weatherChanged = true;
}

std::pair<bool, BlockPos> WeatherManager::trySpawnLightning() {
    // 参考 MC 1.16.5 ServerWorld.tickEnvironment()
    // 雷暴时每tick有 1/100000 概率生成闪电

    if (!m_state.isThundering() || !m_state.isRaining()) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 概率检查
    if (m_random->nextInt(mc::weather::WeatherConstants::LIGHTNING_CHANCE_DENOMINATOR) != 0) {
        return {false, BlockPos(0, 0, 0)};
    }

    // TODO: 实现完整闪电生成逻辑
    // 1. 随机选择区块
    // 2. 找到可以降雨的位置
    // 3. 调整位置到实体附近
    // 4. 检查是否生成骷髅马陷阱

    // 简化实现：返回一个标志表示需要生成闪电
    // 实际位置由调用者决定
    return {true, BlockPos(0, 0, 0)};
}

void WeatherManager::serialize(std::vector<u8>& data) const {
    // 简单的二进制序列化
    // 格式: [clearWeatherTime(4)] [rainTime(4)] [thunderTime(4)]
    //       [raining(1)] [thundering(1)] [weatherCycleEnabled(1)]
    //       [rainStrength(4)] [thunderStrength(4)]

    auto writeI32 = [&data](i32 value) {
        data.push_back(static_cast<u8>(value & 0xFF));
        data.push_back(static_cast<u8>((value >> 8) & 0xFF));
        data.push_back(static_cast<u8>((value >> 16) & 0xFF));
        data.push_back(static_cast<u8>((value >> 24) & 0xFF));
    };

    auto writeF32 = [&data](f32 value) {
        u32 bits;
        std::memcpy(&bits, &value, sizeof(f32));
        data.push_back(static_cast<u8>(bits & 0xFF));
        data.push_back(static_cast<u8>((bits >> 8) & 0xFF));
        data.push_back(static_cast<u8>((bits >> 16) & 0xFF));
        data.push_back(static_cast<u8>((bits >> 24) & 0xFF));
    };

    writeI32(m_state.clearWeatherTime);
    writeI32(m_state.rainTime);
    writeI32(m_state.thunderTime);
    data.push_back(m_state.raining ? 1 : 0);
    data.push_back(m_state.thundering ? 1 : 0);
    data.push_back(m_state.weatherCycleEnabled ? 1 : 0);
    writeF32(m_state.rainStrength);
    writeF32(m_state.thunderStrength);
}

Result<void> WeatherManager::deserialize(const std::vector<u8>& data, size_t& offset) {
    if (data.size() < offset + 22) { // 3 * 4 + 3 + 2 * 4 = 23 bytes
        return Error(ErrorCode::InvalidData, "Insufficient data for weather state");
    }

    auto readI32 = [&data, &offset]() -> i32 {
        i32 value = static_cast<i32>(
            static_cast<u32>(data[offset]) |
            (static_cast<u32>(data[offset + 1]) << 8) |
            (static_cast<u32>(data[offset + 2]) << 16) |
            (static_cast<u32>(data[offset + 3]) << 24));
        offset += 4;
        return value;
    };

    auto readF32 = [&data, &offset]() -> f32 {
        u32 bits =
            static_cast<u32>(data[offset]) |
            (static_cast<u32>(data[offset + 1]) << 8) |
            (static_cast<u32>(data[offset + 2]) << 16) |
            (static_cast<u32>(data[offset + 3]) << 24);
        offset += 4;
        f32 value;
        std::memcpy(&value, &bits, sizeof(f32));
        return value;
    };

    m_state.clearWeatherTime = readI32();
    m_state.rainTime = readI32();
    m_state.thunderTime = readI32();
    m_state.raining = data[offset++] != 0;
    m_state.thundering = data[offset++] != 0;
    m_state.weatherCycleEnabled = data[offset++] != 0;
    m_state.rainStrength = readF32();
    m_state.thunderStrength = readF32();

    // 初始化前一帧强度
    m_state.prevRainStrength = m_state.rainStrength;
    m_state.prevThunderStrength = m_state.thunderStrength;

    return {};
}

} // namespace mc::server
