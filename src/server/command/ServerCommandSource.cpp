#include "ServerCommandSource.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include <spdlog/spdlog.h>

namespace mr {
namespace command {

ServerCommandSource::ServerCommandSource(
    MinecraftServer* server,
    ServerPlayer* player,
    server::ServerWorld* world,
    const Vector3d& position,
    const Vector2f& rotation,
    i32 permissionLevel
)
    : m_server(server)
    , m_player(player)
    , m_world(world)
    , m_position(position)
    , m_rotation(rotation)
    , m_permissionLevel(permissionLevel)
    , m_feedbackDisabled(false)
{
    // 设置显示名称
    if (player) {
        m_name = player->username();
    } else {
        m_name = "Console";
    }
}

void ServerCommandSource::sendMessage(
    const String& message,
    const std::optional<Uuid>& /*senderUuid*/
) {
    // 发送消息给命令源
    if (m_player) {
        // 发送给玩家
        m_player->sendSystemMessage(message);
    } else {
        // 发送给控制台
        spdlog::info("{}", message);
    }
}

bool ServerCommandSource::shouldReceiveFeedback() const {
    return !m_feedbackDisabled;
}

bool ServerCommandSource::shouldReceiveErrors() const {
    return true;
}

bool ServerCommandSource::allowLogging() const {
    return true;
}

ServerPlayer& ServerCommandSource::assertPlayer() const {
    if (!m_player) {
        throw CommandException(
            CommandErrorType::PermissionDenied,
            "commands.requires.player"
        );
    }
    return *m_player;
}

ServerCommandSource ServerCommandSource::withPlayer(ServerPlayer* player) const {
    ServerCommandSource source(*this);
    source.m_player = player;
    if (player) {
        source.m_name = player->username();
    }
    return source;
}

ServerCommandSource ServerCommandSource::withPosition(const Vector3d& pos) const {
    ServerCommandSource source(*this);
    source.m_position = pos;
    return source;
}

ServerCommandSource ServerCommandSource::withRotation(const Vector2f& rot) const {
    ServerCommandSource source(*this);
    source.m_rotation = rot;
    return source;
}

ServerCommandSource ServerCommandSource::withWorld(server::ServerWorld* world) const {
    ServerCommandSource source(*this);
    source.m_world = world;
    return source;
}

ServerCommandSource ServerCommandSource::withFeedbackDisabled() const {
    ServerCommandSource source(*this);
    source.m_feedbackDisabled = true;
    return source;
}

ServerCommandSource ServerCommandSource::withPermissionLevel(i32 level) const {
    ServerCommandSource source(*this);
    source.m_permissionLevel = level;
    return source;
}

ServerCommandSource ServerCommandSource::forConsole(MinecraftServer* server) {
    return ServerCommandSource(
        server,
        nullptr,  // 无玩家
        nullptr,  // 无世界
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        4  // 控制台拥有最高权限
    );
}

} // namespace command
} // namespace mr
