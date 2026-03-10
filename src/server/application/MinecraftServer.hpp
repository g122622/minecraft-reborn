#pragma once

#include "common/core/Types.hpp"
#include "common/command/CommandSource.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mr {

class ServerWorld;
class ServerPlayer;

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
    [[nodiscard]] virtual ServerWorld* getWorld() = 0;

    /**
     * @brief 获取服务器种子
     */
    [[nodiscard]] virtual i64 getSeed() const = 0;

    /**
     * @brief 获取服务器 ticks
     */
    [[nodiscard]] virtual i64 getTicks() const = 0;

    /**
     * @brief 获取所有在线玩家
     */
    [[nodiscard]] virtual std::vector<ServerPlayer*> getPlayers() = 0;

    /**
     * @brief 根据名称获取玩家
     */
    [[nodiscard]] virtual ServerPlayer* getPlayer(const String& name) = 0;

    /**
     * @brief 发送消息给所有玩家
     */
    virtual void broadcast(const String& message) = 0;

    /**
     * @brief 获取命令注册表
     */
    [[nodiscard]] virtual command::CommandRegistry& getCommandRegistry() = 0;

    /**
     * @brief 检查命令源是否有权限运行命令
     */
    virtual bool isCommandAllowed(const command::ICommandSource& source, const String& command) = 0;
};

} // namespace mr
