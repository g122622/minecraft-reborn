#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /weather 命令
 *
 * 用法：
 * - /weather clear [duration] - 设置晴天
 * - /weather rain [duration] - 设置降雨
 * - /weather thunder [duration] - 设置雷暴
 * - /weather query - 查询当前天气
 *
 * 权限等级：2
 *
 * duration 单位为 ticks (20 ticks = 1秒)
 * 如果不指定 duration，默认为 6000 ticks (5分钟)
 *
 * 参考 MC 1.16.5 WeatherCommand
 */
class WeatherCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 setClear(CommandContext<ServerCommandSource>& context);
    static i32 setRain(CommandContext<ServerCommandSource>& context);
    static i32 setThunder(CommandContext<ServerCommandSource>& context);
    static i32 query(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
