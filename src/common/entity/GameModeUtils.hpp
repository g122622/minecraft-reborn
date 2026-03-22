#pragma once

#include "common/core/Types.hpp"

namespace mc {

// Forward declaration
struct PlayerAbilities;

namespace entity {

/**
 * @brief 游戏模式工具函数
 *
 * 提供游戏模式相关的能力计算工具。
 * 参考 MC 1.16.5 PlayerList#setGameMode
 */
namespace GameModeUtils {

/**
 * @brief 根据游戏模式计算玩家能力
 *
 * 返回的 PlayerAbilities 结构包含正确的能力标志。
 *
 * @param mode 游戏模式
 * @return PlayerAbilities 结构
 */
[[nodiscard]] PlayerAbilities getAbilitiesForGameMode(GameMode mode);

/**
 * @brief 检查游戏模式是否允许飞行
 *
 * 创造模式和旁观者模式允许飞行。
 *
 * @param mode 游戏模式
 * @return 如果允许飞行返回 true
 */
[[nodiscard]] bool canFly(GameMode mode);

/**
 * @brief 检查游戏模式是否无敌
 *
 * 创造模式和旁观者模式无敌。
 *
 * @param mode 游戏模式
 * @return 如果无敌返回 true
 */
[[nodiscard]] bool isInvulnerable(GameMode mode);

/**
 * @brief 检查游戏模式是否允许编辑方块
 *
 * 冒险模式和旁观者模式不允许编辑。
 *
 * @param mode 游戏模式
 * @return 如果允许编辑返回 true
 */
[[nodiscard]] bool canEdit(GameMode mode);

/**
 * @brief 检查游戏模式是否为创造模式
 *
 * @param mode 游戏模式
 * @return 如果是创造模式返回 true
 */
[[nodiscard]] bool isCreative(GameMode mode);

/**
 * @brief 检查游戏模式是否为生存模式
 *
 * @param mode 游戏模式
 * @return 如果是生存模式返回 true
 */
[[nodiscard]] bool isSurvival(GameMode mode);

/**
 * @brief 检查游戏模式是否为冒险模式
 *
 * @param mode 游戏模式
 * @return 如果是冒险模式返回 true
 */
[[nodiscard]] bool isAdventure(GameMode mode);

/**
 * @brief 检查游戏模式是否为旁观者模式
 *
 * @param mode 游戏模式
 * @return 如果是旁观者模式返回 true
 */
[[nodiscard]] bool isSpectator(GameMode mode);

} // namespace GameModeUtils
} // namespace entity
} // namespace mc
