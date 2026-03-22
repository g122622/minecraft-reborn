#include "GameModeManager.hpp"
#include "PlayerManager.hpp"
#include "ConnectionManager.hpp"
#include "ServerPlayerData.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::core {

GameModeManager::GameModeManager(PlayerManager& playerManager, ConnectionManager& connectionManager)
    : m_playerManager(playerManager)
    , m_connectionManager(connectionManager)
{
}

// ============================================================================
// 游戏模式管理
// ============================================================================

bool GameModeManager::setGameMode(PlayerId playerId, GameMode mode) {
    // 获取玩家数据
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        spdlog::warn("GameModeManager: Player {} not found", playerId);
        return false;
    }

    GameMode oldMode = player->gameMode;

    // 如果模式相同，不需要更新
    if (oldMode == mode) {
        return true;
    }

    // 更新玩家游戏模式
    player->gameMode = mode;

    // 根据游戏模式更新玩家能力
    // 注意：飞行状态等能力会在客户端收到 PlayerAbilitiesPacket 后更新

    // 发送网络包
    if (!sendGameModeChangePacket(playerId, mode)) {
        spdlog::warn("GameModeManager: Failed to send game mode change packet to player {}", playerId);
    }

    if (!sendAbilitiesPacket(playerId, mode)) {
        spdlog::warn("GameModeManager: Failed to send abilities packet to player {}", playerId);
    }

    // 调用回调
    if (m_onGameModeChange) {
        m_onGameModeChange(playerId, oldMode, mode);
    }

    spdlog::info("GameModeManager: Player {} changed game mode from {} to {}",
                 playerId, static_cast<int>(oldMode), static_cast<int>(mode));

    return true;
}

bool GameModeManager::setGameModeLocal(PlayerId playerId, GameMode mode) {
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }

    player->gameMode = mode;
    return true;
}

GameMode GameModeManager::getGameMode(PlayerId playerId) const {
    const auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return GameMode::NotSet;
    }
    return player->gameMode;
}

// ============================================================================
// 能力同步
// ============================================================================

bool GameModeManager::syncAbilities(PlayerId playerId) {
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }

    return sendAbilitiesPacket(playerId, player->gameMode);
}

u8 GameModeManager::getAbilitiesForGameMode(GameMode mode) {
    u8 flags = 0;

    switch (mode) {
        case GameMode::Creative:
            // 创造模式：无敌、可飞行、创造模式标志
            flags |= static_cast<u8>(network::PlayerAbilityFlags::Invulnerable);
            flags |= static_cast<u8>(network::PlayerAbilityFlags::CanFly);
            flags |= static_cast<u8>(network::PlayerAbilityFlags::CreativeMode);
            break;

        case GameMode::Spectator:
            // 旁观者模式：无敌、可飞行、正在飞行
            flags |= static_cast<u8>(network::PlayerAbilityFlags::Invulnerable);
            flags |= static_cast<u8>(network::PlayerAbilityFlags::CanFly);
            flags |= static_cast<u8>(network::PlayerAbilityFlags::Flying);
            break;

        case GameMode::Survival:
        case GameMode::Adventure:
        default:
            // 生存和冒险模式：无特殊能力
            break;
    }

    return flags;
}

// ============================================================================
// 私有方法
// ============================================================================

bool GameModeManager::sendGameModeChangePacket(PlayerId playerId, GameMode mode) {
    network::GameStateChangePacket packet = network::GameStateChangePacket::gameModeChange(mode);

    auto result = packet.serialize();
    if (!result.success()) {
        spdlog::error("GameModeManager: Failed to serialize GameStateChangePacket: {}", result.error().message());
        return false;
    }

    return m_connectionManager.sendPacketToPlayer(
        playerId,
        network::PacketType::GameStateChange,
        result.value()
    );
}

bool GameModeManager::sendAbilitiesPacket(PlayerId playerId, GameMode mode) {
    network::PlayerAbilitiesPacket packet = network::PlayerAbilitiesPacket::fromGameMode(mode);

    auto result = packet.serialize();
    if (!result.success()) {
        spdlog::error("GameModeManager: Failed to serialize PlayerAbilitiesPacket: {}", result.error().message());
        return false;
    }

    return m_connectionManager.sendPacketToPlayer(
        playerId,
        network::PacketType::PlayerAbilities,
        result.value()
    );
}

} // namespace mc::server::core
