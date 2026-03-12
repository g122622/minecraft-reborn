#include "ClearCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/player/ServerPlayer.hpp"
#include <sstream>

namespace mc {
namespace command {

void ClearCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    clearNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });

    // /clear - 清空自己
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearSelf(ctx);
    });

    // /clear <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::players()
    );
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearPlayer(ctx);
    });

    // /clear <player> <item>
    auto itemArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>(
        "item",
        ItemArgumentType::item()
    );
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearPlayerItem(ctx);
    });

    // /clear <player> <item> <maxCount>
    auto maxCountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "maxCount",
        IntegerArgumentType::integer(0)
    );
    maxCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearPlayerItemCount(ctx);
    });

    itemArg->addChild(maxCountArg);
    playerArg->addChild(itemArg);
    clearNode->addChild(playerArg);

    dispatcher.registerCommand(clearNode);
}

i32 ClearCommand::clearSelf(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 必须是玩家
    if (!source.isPlayer()) {
        source.sendMessage("You must be a player to use this command");
        return 0;
    }

    ServerPlayer& player = source.assertPlayer();

    // TODO: 清空背包
    // i32 clearedCount = player.getInventory().clear();

    std::ostringstream ss;
    ss << "Cleared the inventory of " << player.username() << ", removing 0 items";
    source.sendMessage(ss.str());

    return 1;
}

i32 ClearCommand::clearPlayer(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 获取目标玩家
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // TODO: 解析选择器获取玩家列表
    // std::vector<ServerPlayer*> players = resolvePlayers(selector, source);

    std::ostringstream ss;
    ss << "Cleared the inventory of target player(s), removing 0 items";
    source.sendMessage(ss.str());

    return 1;
}

i32 ClearCommand::clearPlayerItem(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 获取目标玩家
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取物品
    ItemInput itemInput = context.getArgument<ItemInput>("item");

    if (!itemInput.isValid()) {
        source.sendMessage("Invalid item");
        return 0;
    }

    const Item* item = itemInput.getItem();
    if (!item) {
        source.sendMessage("Unknown item");
        return 0;
    }

    // TODO: 清除指定物品
    // i32 clearedCount = player.getInventory().clear(item->getId(), INT_MAX);

    std::ostringstream ss;
    ss << "Cleared " << item->getName() << " from target player(s), removing 0 items";
    source.sendMessage(ss.str());

    return 1;
}

i32 ClearCommand::clearPlayerItemCount(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 获取目标玩家
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取物品
    ItemInput itemInput = context.getArgument<ItemInput>("item");

    // 获取最大数量
    i32 maxCount = context.getArgument<i32>("maxCount");

    if (!itemInput.isValid()) {
        source.sendMessage("Invalid item");
        return 0;
    }

    const Item* item = itemInput.getItem();
    if (!item) {
        source.sendMessage("Unknown item");
        return 0;
    }

    // TODO: 清除指定数量的物品
    // i32 clearedCount = player.getInventory().clear(item->getId(), maxCount);

    std::ostringstream ss;
    ss << "Cleared " << item->getName() << " (max " << maxCount
       << ") from target player(s), removing 0 items";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
