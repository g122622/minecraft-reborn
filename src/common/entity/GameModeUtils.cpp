#include "GameModeUtils.hpp"
#include "Player.hpp"

namespace mc {
namespace entity {

PlayerAbilities GameModeUtils::getAbilitiesForGameMode(GameMode mode) {
    PlayerAbilities abilities;

    switch (mode) {
        case GameMode::Survival:
            // 生存模式：默认能力（无特殊标志）
            abilities.creativeMode = false;
            abilities.canFly = false;
            abilities.flying = false;
            abilities.invulnerable = false;
            abilities.allowEdit = true;
            abilities.flySpeed = 0.05f;
            abilities.walkSpeed = 0.1f;
            break;

        case GameMode::Creative:
            // 创造模式：无敌、可飞行、创造模式
            abilities.creativeMode = true;
            abilities.canFly = true;
            abilities.flying = false;  // 飞行状态由玩家控制
            abilities.invulnerable = true;
            abilities.allowEdit = true;
            abilities.flySpeed = 0.05f;
            abilities.walkSpeed = 0.1f;
            break;

        case GameMode::Adventure:
            // 冒险模式：无特殊能力，不能编辑方块
            abilities.creativeMode = false;
            abilities.canFly = false;
            abilities.flying = false;
            abilities.invulnerable = false;
            abilities.allowEdit = false;  // 不能编辑方块
            abilities.flySpeed = 0.05f;
            abilities.walkSpeed = 0.1f;
            break;

        case GameMode::Spectator:
            // 旁观者模式：无敌、可飞行、正在飞行
            abilities.creativeMode = false;
            abilities.canFly = true;
            abilities.flying = true;  // 旁观者默认飞行
            abilities.invulnerable = true;
            abilities.allowEdit = false;  // 不能编辑方块
            abilities.flySpeed = 0.05f;
            abilities.walkSpeed = 0.1f;
            break;

        default:
            // 未知模式：使用生存模式设置
            abilities.creativeMode = false;
            abilities.canFly = false;
            abilities.flying = false;
            abilities.invulnerable = false;
            abilities.allowEdit = true;
            abilities.flySpeed = 0.05f;
            abilities.walkSpeed = 0.1f;
            break;
    }

    return abilities;
}

bool GameModeUtils::canFly(GameMode mode) {
    return mode == GameMode::Creative || mode == GameMode::Spectator;
}

bool GameModeUtils::isInvulnerable(GameMode mode) {
    return mode == GameMode::Creative || mode == GameMode::Spectator;
}

bool GameModeUtils::canEdit(GameMode mode) {
    return mode != GameMode::Adventure && mode != GameMode::Spectator;
}

bool GameModeUtils::isCreative(GameMode mode) {
    return mode == GameMode::Creative;
}

bool GameModeUtils::isSurvival(GameMode mode) {
    return mode == GameMode::Survival;
}

bool GameModeUtils::isAdventure(GameMode mode) {
    return mode == GameMode::Adventure;
}

bool GameModeUtils::isSpectator(GameMode mode) {
    return mode == GameMode::Spectator;
}

} // namespace entity
} // namespace mc
