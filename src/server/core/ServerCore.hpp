#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/world/time/GameTime.hpp"
#include "server/core/ServerCoreConfig.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <memory>
#include <functional>

namespace mc {
class AbstractContainerMenu;
}

namespace mc::server {

// 前向声明
class ServerWorld;

namespace core {
// 前向声明管理器
class PlayerManager;
class ConnectionManager;
class TimeManager;
class TeleportManager;
class KeepAliveManager;
class PositionTracker;
class PacketHandler;
}

/**
 * @brief 服务端核心（门面类）
 *
 * 协调所有管理器，提供统一的接口给 IntegratedServer 和 ServerApplication 使用。
 * 通过组合各个专门的管理器实现关注点分离。
 *
 * 使用示例：
 * @code
 * ServerCoreConfig config;
 * config.viewDistance = 12;
 * config.maxPlayers = 50;
 *
 * ServerCore core(config);
 * core.setWorld(&world);
 *
 * // 添加玩家
 * auto* player = core.addPlayer(1, "Steve", connection);
 *
 * // 主循环
 * while (running) {
 *     core.tick();
 *     // ...
 * }
 * @endcode
 */
class ServerCore {
public:
    /**
     * @brief 构造服务端核心
     */
    ServerCore();

    /**
     * @brief 构造服务端核心（带配置）
     * @param config 配置
     */
    explicit ServerCore(const ServerCoreConfig& config);

    /**
     * @brief 析构函数
     */
    ~ServerCore();

    // 禁止拷贝
    ServerCore(const ServerCore&) = delete;
    ServerCore& operator=(const ServerCore&) = delete;

    // ========== 配置 ==========

    /**
     * @brief 设置配置
     */
    void setConfig(const ServerCoreConfig& config);

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const ServerCoreConfig& config() const { return m_config; }

    // ========== 世界关联 ==========

    /**
     * @brief 设置关联的服务端世界
     * @param world 服务端世界指针（不获取所有权）
     */
    void setWorld(ServerWorld* world) { m_world = world; }

    /**
     * @brief 获取世界
     */
    [[nodiscard]] ServerWorld* world() { return m_world; }
    [[nodiscard]] const ServerWorld* world() const { return m_world; }

    // ========== 管理器访问 ==========

    /**
     * @brief 获取玩家管理器
     */
    [[nodiscard]] core::PlayerManager& playerManager();
    [[nodiscard]] const core::PlayerManager& playerManager() const;

    /**
     * @brief 获取连接管理器
     */
    [[nodiscard]] core::ConnectionManager& connectionManager();
    [[nodiscard]] const core::ConnectionManager& connectionManager() const;

    /**
     * @brief 获取时间管理器
     */
    [[nodiscard]] core::TimeManager& timeManager();
    [[nodiscard]] const core::TimeManager& timeManager() const;

    /**
     * @brief 获取传送管理器
     */
    [[nodiscard]] core::TeleportManager& teleportManager();
    [[nodiscard]] const core::TeleportManager& teleportManager() const;

    /**
     * @brief 获取心跳管理器
     */
    [[nodiscard]] core::KeepAliveManager& keepAliveManager();
    [[nodiscard]] const core::KeepAliveManager& keepAliveManager() const;

    /**
     * @brief 获取位置追踪器
     */
    [[nodiscard]] core::PositionTracker& positionTracker();
    [[nodiscard]] const core::PositionTracker& positionTracker() const;

    /**
     * @brief 获取数据包处理器
     */
    [[nodiscard]] core::PacketHandler& packetHandler();
    [[nodiscard]] const core::PacketHandler& packetHandler() const;

    /**
     * @brief 获取天气管理器
     */
    [[nodiscard]] WeatherManager& weatherManager();
    [[nodiscard]] const WeatherManager& weatherManager() const;

    // ========== 便捷方法（委托给管理器） ==========

    // --- 玩家管理 ---

    /**
     * @brief 添加玩家
     * @param playerId 玩家ID
     * @param username 用户名
     * @param connection 连接接口
     * @return 玩家数据指针
     */
    ServerPlayerData* addPlayer(PlayerId playerId, const String& username, network::ConnectionPtr connection);

    /**
     * @brief 移除玩家
     */
    void removePlayer(PlayerId playerId);

    /**
     * @brief 获取玩家数据
     */
    [[nodiscard]] ServerPlayerData* getPlayer(PlayerId playerId);
    [[nodiscard]] const ServerPlayerData* getPlayer(PlayerId playerId) const;

    /**
     * @brief 检查玩家是否存在
     */
    [[nodiscard]] bool hasPlayer(PlayerId playerId) const;

    /**
     * @brief 获取玩家数量
     */
    [[nodiscard]] size_t playerCount() const;

    /**
     * @brief 遍历所有玩家
     */
    template<typename Func>
    void forEachPlayer(Func&& func);

    template<typename Func>
    void forEachPlayer(Func&& func) const;

    /**
     * @brief 获取下一个玩家ID
     */
    [[nodiscard]] PlayerId nextPlayerId();

    // --- 连接管理 ---

    /**
     * @brief 断开玩家连接
     */
    void disconnectPlayer(PlayerId playerId, const String& reason = "");

    /**
     * @brief 清理已断开连接的玩家
     */
    void cleanupDisconnectedPlayers();

    /**
     * @brief 向所有玩家广播数据
     */
    void broadcast(const u8* data, size_t size);

    /**
     * @brief 向除指定玩家外的所有玩家广播数据
     */
    void broadcastExcept(PlayerId excludePlayerId, const u8* data, size_t size);

    /**
     * @brief 向指定玩家发送数据包
     */
    bool sendPacketToPlayer(PlayerId playerId, network::PacketType type, const std::vector<u8>& payload);

    /**
     * @brief 广播数据包给所有玩家
     */
    void broadcastPacket(network::PacketType type, const std::vector<u8>& payload);

    // --- 时间管理 ---

    /**
     * @brief 获取游戏时间
     */
    [[nodiscard]] time::GameTime& gameTime();
    [[nodiscard]] const time::GameTime& gameTime() const;

    /**
     * @brief 更新时间（每 tick 调用）
     */
    void tickTime();

    /**
     * @brief 获取当前 tick
     */
    [[nodiscard]] u64 currentTick() const;

    // --- 区块同步 ---

    /**
     * @brief 获取区块同步管理器
     */
    [[nodiscard]] network::ChunkSyncManager& chunkSyncManager();
    [[nodiscard]] const network::ChunkSyncManager& chunkSyncManager() const;

    // --- 传送管理 ---

    /**
     * @brief 传送玩家
     * @return 传送ID
     */
    u32 teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw = 0.0f, f32 pitch = 0.0f);

    /**
     * @brief 确认传送
     */
    bool confirmTeleport(PlayerId playerId, u32 teleportId);

    // --- 位置更新 ---

    /**
     * @brief 更新玩家位置
     */
    void updatePlayerPosition(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, bool onGround);

    // --- 心跳管理 ---

    /**
     * @brief 更新心跳时间戳
     */
    void updateKeepAlive(PlayerId playerId, u64 timestamp);

    /**
     * @brief 检查是否需要发送心跳
     */
    [[nodiscard]] bool needsKeepAlive(PlayerId playerId, u64 currentTick) const;

    /**
     * @brief 记录心跳发送时间
     */
    void recordKeepAliveSent(PlayerId playerId, u64 timestamp);

    // --- 游戏模式管理 ---

    /**
     * @brief 设置玩家游戏模式
     * @param playerId 玩家ID
     * @param mode 游戏模式
     * @return true 如果设置成功
     */
    bool setPlayerGameMode(PlayerId playerId, GameMode mode);

    /**
     * @brief 获取玩家游戏模式
     * @param playerId 玩家ID
     * @return 游戏模式，如果玩家不存在返回 NotSet
     */
    [[nodiscard]] GameMode getPlayerGameMode(PlayerId playerId) const;

    // ========== 主循环 ==========

    /**
     * @brief 执行一个 tick
     *
     * 包括：更新时间、清理断开连接的玩家、处理心跳超时等
     */
    void tick();

private:
    ServerCoreConfig m_config;
    ServerWorld* m_world = nullptr;

    // 管理器（使用 unique_ptr 避免头文件依赖）
    std::unique_ptr<core::PlayerManager> m_playerManager;
    std::unique_ptr<core::ConnectionManager> m_connectionManager;
    std::unique_ptr<core::TimeManager> m_timeManager;
    std::unique_ptr<core::TeleportManager> m_teleportManager;
    std::unique_ptr<core::KeepAliveManager> m_keepAliveManager;
    std::unique_ptr<core::PositionTracker> m_positionTracker;
    std::unique_ptr<core::PacketHandler> m_packetHandler;

    // 天气管理器（直接对象）
    WeatherManager m_weatherManager;

    // Tick 计数器（用于间隔执行清理和心跳检查）
    u64 m_currentCleanupTick = 0;
    u64 m_currentKeepAliveCheckTick = 0;
};

// ============================================================================
// 模板实现
// ============================================================================

template<typename Func>
void ServerCore::forEachPlayer(Func&& func) {
    m_playerManager->forEachPlayer(std::forward<Func>(func));
}

template<typename Func>
void ServerCore::forEachPlayer(Func&& func) const {
    m_playerManager->forEachPlayer(std::forward<Func>(func));
}

} // namespace mc::server
