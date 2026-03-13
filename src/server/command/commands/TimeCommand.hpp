#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /time 命令
 *
 * 用法：
 * - /time set <value> - 设置时间
 * - /time add <value> - 增加时间
 * - /time query <day|daytime|gametime> - 查询时间
 *
 * 权限等级：2
 *
 * 参考 MC 的 TimeCommand
 */
class TimeCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 setTime(CommandContext<ServerCommandSource>& context);
    static i32 addTime(CommandContext<ServerCommandSource>& context);
    static i32 queryTime(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
