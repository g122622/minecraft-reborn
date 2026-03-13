#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /list 命令
 *
 * 用法：
 * - /list - 列出当前服务器上的所有玩家
 *
 * 权限等级：0（所有玩家可用）
 *
 * 参考 MC 的 ListCommand
 */
class ListCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 listPlayers(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
