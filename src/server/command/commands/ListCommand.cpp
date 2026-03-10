#include "ListCommand.hpp"
#include "common/command/CommandContext.hpp"
#include <sstream>

namespace mr {
namespace command {

void ListCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mr::command;

    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(0);  // 所有人可用
    });
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return listPlayers(ctx);
    });

    dispatcher.registerCommand(listNode);
}

i32 ListCommand::listPlayers(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();

    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    // TODO: 从服务器获取玩家列表
    // auto players = server->getPlayerList();

    // 暂时返回占位信息
    std::ostringstream ss;
    ss << "There are 0 of a max of 20 players online";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mr
