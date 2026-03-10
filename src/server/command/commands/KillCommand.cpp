#include "KillCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/player/ServerPlayer.hpp"
#include <sstream>

namespace mr {
namespace command {

void KillCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mr::command;

    // /kill - 杀死自己
    auto killNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("kill");
    killNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    killNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return killSelf(ctx);
    });

    // /kill <target> - 杀死指定实体
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target",
        EntityArgumentType::entities()
    );
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return killTarget(ctx);
    });
    killNode->addChild(targetArg);

    dispatcher.registerCommand(killNode);
}

i32 KillCommand::killSelf(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 必须是实体
    if (!source.isPlayer()) {
        source.sendMessage("You must be an entity to use this command");
        return 0;
    }

    ServerPlayer& player = source.assertPlayer();

    // TODO: 杀死玩家
    // player.kill();

    std::ostringstream ss;
    ss << "Killed " << player.username();
    source.sendMessage(ss.str());

    return 1;
}

i32 KillCommand::killTarget(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 获取目标选择器
    EntitySelector selector = context.getArgument<EntitySelector>("target");

    // TODO: 解析选择器获取实体列表
    // std::vector<Entity*> entities = resolveSelector(selector, source);

    source.sendMessage("Killed target entity");
    return 1;
}

} // namespace command
} // namespace mr
