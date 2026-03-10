#include "TeleportCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/entity/Player.hpp"
#include <sstream>

namespace mr {
namespace command {

void TeleportCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mr::command;

    auto tpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tp");
    tpNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });

    // /tp <target> - 传送到目标实体
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target",
        EntityArgumentType::entity()
    );
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return teleportToEntity(ctx);
    });

    // /tp <x> <y> <z> - 传送到坐标
    auto xPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>(
        "x",
        FloatArgumentType::floatArg()
    );
    auto yPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>(
        "y",
        FloatArgumentType::floatArg()
    );
    auto zPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>(
        "z",
        FloatArgumentType::floatArg()
    );
    zPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return teleportToPosition(ctx);
    });
    yPosArg->addChild(zPosArg);
    xPosArg->addChild(yPosArg);

    tpNode->addChild(targetArg);
    tpNode->addChild(xPosArg);

    dispatcher.registerCommand(tpNode);
}

i32 TeleportCommand::teleportToEntity(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 必须是实体
    if (!source.isPlayer()) {
        source.sendMessage("You must be an entity to use this command");
        return 0;
    }

    ServerPlayer& player = source.assertPlayer();

    // 获取目标
    EntitySelector selector = context.getArgument<EntitySelector>("target");

    // TODO: 解析选择器获取目标实体
    // Entity* target = resolveSelector(selector, source);

    std::ostringstream ss;
    ss << "Teleported " << player.getName() << " to target";
    source.sendMessage(ss.str());

    return 1;
}

i32 TeleportCommand::teleportToPosition(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 必须是实体
    if (!source.isPlayer()) {
        source.sendMessage("You must be an entity to use this command");
        return 0;
    }

    ServerPlayer& player = source.assertPlayer();

    // 获取坐标
    f64 x = context.getArgument<f64>("x");
    f64 y = context.getArgument<f64>("y");
    f64 z = context.getArgument<f64>("z");

    // TODO: 实际传送逻辑
    // player.teleport(x, y, z);

    std::ostringstream ss;
    ss << "Teleported " << player.getName() << " to "
       << x << ", " << y << ", " << z;
    source.sendMessage(ss.str());

    return 1;
}

i32 TeleportCommand::teleportTargetToEntity(CommandContext<ServerCommandSource>& context) {
    // TODO: 实现
    auto& source = context.getSource();
    source.sendMessage("Teleport target to entity - not implemented");
    return 0;
}

i32 TeleportCommand::teleportTargetToPosition(CommandContext<ServerCommandSource>& context) {
    // TODO: 实现
    auto& source = context.getSource();
    source.sendMessage("Teleport target to position - not implemented");
    return 0;
}

} // namespace command
} // namespace mr
