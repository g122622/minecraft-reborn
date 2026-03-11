/**
 * @file CombatEntry.hpp
 * @brief 战斗条目 - 记录单次伤害事件
 *
 * 参考 MC 1.16.5 CombatEntry
 */

#pragma once

#include "DamageSource.hpp"
#include "../../core/Types.hpp"
#include <memory>

namespace mr {

/**
 * @brief 战斗条目
 *
 * 记录一次伤害事件的详细信息，用于生成死亡消息。
 */
class CombatEntry {
public:
    /**
     * @brief 构造函数
     * @param source 伤害来源（会被复制）
     * @param damage 伤害值
     * @param timestamp 发生时间（tick）
     */
    CombatEntry(std::unique_ptr<DamageSource> source, f32 damage, u32 timestamp);

    /**
     * @brief 获取伤害来源
     */
    [[nodiscard]] const DamageSource* source() const { return m_source.get(); }

    /**
     * @brief 获取伤害值
     */
    [[nodiscard]] f32 damage() const { return m_damage; }

    /**
     * @brief 获取发生时间
     */
    [[nodiscard]] u32 timestamp() const { return m_timestamp; }

    /**
     * @brief 是否来自生物
     */
    [[nodiscard]] bool isLivingSource() const;

    /**
     * @brief 是否来自玩家
     */
    [[nodiscard]] bool isPlayerSource() const;

private:
    std::unique_ptr<DamageSource> m_source;
    f32 m_damage;
    u32 m_timestamp;
};

} // namespace mr
