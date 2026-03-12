#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /clear 命令
 *
 * 用法：
 * - /clear - 清空自己的背包
 * - /clear <player> - 清空指定玩家的背包
 * - /clear <player> <item> - 清空指定玩家的指定物品
 * - /clear <player> <item> <maxCount> - 清空指定玩家的指定物品，最多清除数量
 *
 * 权限等级：2
 *
 * 参考 MC 的 ClearCommand
 */
class ClearCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 clearSelf(CommandContext<ServerCommandSource>& context);
    static i32 clearPlayer(CommandContext<ServerCommandSource>& context);
    static i32 clearPlayerItem(CommandContext<ServerCommandSource>& context);
    static i32 clearPlayerItemCount(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
