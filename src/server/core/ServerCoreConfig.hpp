#pragma once

#include "common/core/Types.hpp"

namespace mc::server {

/// Tick 持续时间（毫秒）- 20 TPS 对应 50ms
constexpr u64 TICK_DURATION_MS = 50;

/// 心跳检查间隔（tick 数）- 每 300 tick (15秒) 检查一次
constexpr u64 KEEPALIVE_CHECK_INTERVAL_TICKS = 300;

/// 清理断开连接玩家的间隔（tick 数）- 每 20 tick (1秒) 清理一次
constexpr u64 CLEANUP_INTERVAL_TICKS = 20;

/**
 * @brief 服务端核心配置
 *
 * 包含服务器运行时的各种配置参数，如视距、心跳间隔、游戏模式等。
 *
 * 使用示例：
 * @code
 * ServerCoreConfig config;
 * config.viewDistance = 12;
 * config.maxPlayers = 50;
 * config.seed = 12345;
 * @endcode
 */
struct ServerCoreConfig {
    /// 视距（区块数）
    i32 viewDistance = 10;

    /// 心跳间隔（毫秒）
    i32 keepAliveInterval = 15000;

    /// 心跳超时（毫秒）
    i32 keepAliveTimeout = 30000;

    /// 默认游戏模式
    GameMode defaultGameMode = GameMode::Survival;

    /// 世界种子
    u64 seed = 12345;

    /// 最大玩家数
    i32 maxPlayers = 20;

    /// 服务器 TPS（Ticks Per Second）
    i32 tickRate = 20;
};

} // namespace mc::server
