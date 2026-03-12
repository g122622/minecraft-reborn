#pragma once

#include "server/application/MinecraftServer.hpp"
#include "server/core/ServerCore.hpp"
#include "server/core/TimeManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/player/ServerPlayer.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

/**
 * @brief 统一的服务端命令桥接类
 *
 * 为 IntegratedServer 和 ServerApplication 提供统一的 MinecraftServer 实现。
 * 通过 ServerCore 和可选的 ServerWorld 提供命令执行所需的上下文。
 *
 * 使用示例：
 * @code
 * CoreCommandBridge bridge(m_world, m_serverCore.get());
 * command::ServerCommandSource source(&bridge, ...);
 * registry.execute("/time set day", source);
 * @endcode
 */
class CoreCommandBridge final : public MinecraftServer {
public:
    /**
     * @brief 构造命令桥接
     * @param world 服务端世界（可为 nullptr，用于内置服务器）
     * @param core 服务端核心（必须有效）
     */
    CoreCommandBridge(ServerWorld* world, ServerCore* core)
        : m_world(world)
        , m_core(core)
    {
    }

    // ========== MinecraftServer 接口实现 ==========

    [[nodiscard]] ServerWorld* getWorld() override { return m_world; }

    [[nodiscard]] i64 getSeed() const override {
        return m_world ? static_cast<i64>(m_world->config().seed) : 0;
    }

    [[nodiscard]] i64 getTicks() const override {
        return m_core ? static_cast<i64>(m_core->currentTick()) : 0;
    }

    [[nodiscard]] i64 getDay() const override {
        return m_core ? m_core->gameTime().dayCount() : 0;
    }

    [[nodiscard]] i64 getDayTime() const override {
        return m_core ? m_core->gameTime().dayTime() : 0;
    }

    [[nodiscard]] i64 getGameTime() const override {
        return m_core ? m_core->gameTime().gameTime() : 0;
    }

    [[nodiscard]] std::vector<::mc::ServerPlayer*> getPlayers() override {
        // TODO: 从 PlayerManager 获取玩家列表
        return {};
    }

    [[nodiscard]] ::mc::ServerPlayer* getPlayer(const String& /*name*/) override {
        // TODO: 从 PlayerManager 查找玩家
        return nullptr;
    }

    void broadcast(const String& message) override {
        spdlog::info("[Broadcast] {}", message);
    }

    bool setDayTime(i64 time) override {
        if (!m_core) return false;
        m_core->timeManager().setDayTime(time);
        return true;
    }

    bool addDayTime(i64 ticks) override {
        if (!m_core) return false;
        m_core->timeManager().addDayTime(ticks);
        return true;
    }

    bool teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) override {
        if (!m_world) return false;
        m_world->teleportPlayer(playerId, x, y, z, yaw, pitch);
        return true;
    }

    bool setPlayerGameMode(PlayerId playerId, GameMode mode) override {
        return m_world ? m_world->setPlayerGameMode(playerId, mode) : false;
    }

    [[nodiscard]] command::CommandRegistry& getCommandRegistry() override {
        return command::CommandRegistry::getGlobal();
    }

    bool isCommandAllowed(const command::ICommandSource& /*source*/, const String& /*command*/) override {
        return true;
    }

private:
    ServerWorld* m_world;
    ServerCore* m_core;
};

} // namespace mc::server
