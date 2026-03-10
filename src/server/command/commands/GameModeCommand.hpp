#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mr {
namespace command {

/**
 * @brief /gamemode 命令
 *
 * 用法：
 * - /gamemode <mode> - 设置自己的游戏模式
 * - /gamemode <mode> <target> - 设置指定玩家的游戏模式
 *
 * 权限等级：2
 *
 * 参考 MC 的 GameModeCommand
 */
class GameModeCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 设置游戏模式（自己）
     */
    static i32 setGameModeSelf(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置游戏模式（指定玩家）
     */
    static i32 setGameModeOthers(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 实际设置游戏模式的逻辑
     * @param source 命令源
     * @param player 目标玩家
     * @param mode 游戏模式
     * @return 结果码
     */
    static i32 setGameMode(
        ServerCommandSource& source,
        class ServerPlayer& player,
        GameMode mode
    );

    /**
     * @brief 获取游戏模式的显示名称
     */
    static const char* getGameModeName(GameMode mode);
};

} // namespace command
} // namespace mr
