#include "ServerPlayer.hpp"
#include <spdlog/spdlog.h>

namespace mr {

ServerPlayer::ServerPlayer(EntityId id, const String& name)
    : Player(id, name)
{
}

void ServerPlayer::sendChatMessage(const String& message) {
    // TODO: 实现发送聊天消息给玩家
    spdlog::info("[Chat -> {}] {}", username(), message);
}

void ServerPlayer::sendSystemMessage(const String& message) {
    // TODO: 实现发送系统消息给玩家
    spdlog::info("[System -> {}] {}", username(), message);
}

} // namespace mr
