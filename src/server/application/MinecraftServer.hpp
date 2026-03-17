#pragma once

#include "common/core/Types.hpp"
#include "common/command/CommandSource.hpp"
#include "common/network/ProtocolPackets.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

class ServerPlayer;
namespace server {
class ServerWorld;
}

namespace command {
class CommandRegistry;
}

/**
 * @brief Minecraft 服务器主类
 *
 * 管理服务器的所有核心组件，包括：
 * - 世界管理
 * - 玩家管理
 * - 网络处理
 * - 命令系统
 * - tick 循环
 */
class MinecraftServer {
public:
    virtual ~MinecraftServer() = default;

    /**
     * @brief 获取服务器世界
     */
    [[nodiscard]] virtual server::ServerWorld* getWorld() = 0;

    /**
     * @brief 获取服务器种子
     */
    [[nodiscard]] virtual i64 getSeed() const = 0;

    /**
     * @brief 获取服务器 ticks
     */
    [[nodiscard]] virtual i64 getTicks() const = 0;

    /**
     * @brief 获取当前天数
     */
    [[nodiscard]] virtual i64 getDay() const = 0;

    /**
     * @brief 获取当前一天内时间
     */
    [[nodiscard]] virtual i64 getDayTime() const = 0;

    /**
     * @brief 获取游戏总时间
     */
    [[nodiscard]] virtual i64 getGameTime() const = 0;

    /**
     * @brief 获取所有在线玩家
     */
    [[nodiscard]] virtual std::vector<ServerPlayer*> getPlayers() = 0;

    /**
     * @brief 根据名称获取玩家
     */
    [[nodiscard]] virtual ServerPlayer* getPlayer(const String& name) = 0;

    /**
     * @brief 获取在线玩家数量
     */
    [[nodiscard]] virtual size_t playerCount() const = 0;

    /**
     * @brief 发送消息给所有玩家
     */
    virtual void broadcast(const String& message) = 0;

    /**
     * @brief 设置一天内时间
     */
    virtual bool setDayTime(i64 time) = 0;

    /**
     * @brief 增加一天内时间
     */
    virtual bool addDayTime(i64 ticks) = 0;

    /**
     * @brief 设置天气为晴天
     * @param duration 持续时间（ticks），0 表示使用默认值
     * @return 是否成功
     */
    virtual bool setWeatherClear(i32 duration = 0) = 0;

    /**
     * @brief 设置天气为降雨
     * @param duration 持续时间（ticks），0 表示使用默认值
     * @return 是否成功
     */
    virtual bool setWeatherRain(i32 duration = 0) = 0;

    /**
     * @brief 设置天气为雷暴
     * @param duration 持续时间（ticks），0 表示使用默认值
     * @return 是否成功
     */
    virtual bool setWeatherThunder(i32 duration = 0) = 0;

    /**
     * @brief 获取当前天气类型
     */
    [[nodiscard]] virtual i32 getWeatherType() const = 0;

    /**
     * @brief 获取降雨强度
     */
    [[nodiscard]] virtual f32 getRainStrength() const = 0;

    /**
     * @brief 获取雷暴强度
     */
    [[nodiscard]] virtual f32 getThunderStrength() const = 0;

    /**
     * @brief 传送玩家
     */
    virtual bool teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) = 0;

    /**
     * @brief 设置玩家游戏模式
     */
    virtual bool setPlayerGameMode(PlayerId playerId, GameMode mode) = 0;

    /**
     * @brief 获取命令注册表
     */
    [[nodiscard]] virtual command::CommandRegistry& getCommandRegistry() = 0;

    /**
     * @brief 检查命令源是否有权限运行命令
     */
    virtual bool isCommandAllowed(const command::ICommandSource& source, const String& command) = 0;
};

} // namespace mc
