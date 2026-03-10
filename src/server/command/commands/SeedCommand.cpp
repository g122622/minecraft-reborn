#include "SeedCommand.hpp"
#include "common/command/CommandContext.hpp"
#include <sstream>

namespace mr {
namespace command {

void SeedCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mr::command;

    auto seedNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("seed");
    seedNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    seedNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return showSeed(ctx);
    });

    dispatcher.registerCommand(seedNode);
}

i32 SeedCommand::showSeed(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();

    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    // TODO: 从服务器获取世界种子
    // u64 seed = server->getWorldSeed();

    // 暂时使用占位值
    u64 seed = 12345678901234ULL;

    std::ostringstream ss;
    ss << "Seed: [" << seed << "]";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mr
