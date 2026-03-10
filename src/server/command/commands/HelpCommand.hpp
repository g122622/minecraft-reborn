#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mr {
namespace command {

/**
 * @brief /help 命令
 *
 * 用法：
 * - /help - 显示命令帮助
 * - /help <command> - 显示指定命令的帮助
 *
 * 权限等级：0（所有玩家可用）
 *
 * 参考 MC 的 HelpCommand
 */
class HelpCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 showHelp(CommandContext<ServerCommandSource>& context);
    static i32 showCommandHelp(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mr
