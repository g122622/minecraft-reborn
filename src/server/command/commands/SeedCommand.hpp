#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mr {
namespace command {

/**
 * @brief /seed 命令
 *
 * 用法：
 * - /seed - 显示世界种子
 *
 * 权限等级：2
 *
 * 参考 MC 的 SeedCommand
 */
class SeedCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 showSeed(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mr
