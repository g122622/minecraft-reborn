#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /give 命令
 *
 * 用法：
 * - /give <player> <item> [count] - 给予玩家物品
 *
 * 权限等级：2
 *
 * 参考 MC 的 GiveCommand
 */
class GiveCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 giveItem(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
