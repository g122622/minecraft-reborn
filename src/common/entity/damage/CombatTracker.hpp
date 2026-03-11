/**
 * @file CombatTracker.hpp
 * @brief 战斗追踪器 - 记录实体的战斗历史
 *
 * 参考 MC 1.16.5 CombatTracker
 */

#pragma once

#include "CombatEntry.hpp"
#include "../../core/Types.hpp"
#include <vector>
#include <memory>

namespace mr {

// 前向声明
class LivingEntity;
class Entity;

/**
 * @brief 战斗追踪器
 *
 * 记录实体受到的所有伤害事件，用于生成死亡消息和统计战斗数据。
 * 每个LivingEntity都有一个CombatTracker实例。
 */
class CombatTracker {
public:
    /**
     * @brief 构造函数
     * @param owner 拥有此追踪器的生物
     */
    explicit CombatTracker(LivingEntity* owner);

    /**
     * @brief 记录伤害事件
     * @param source 伤害来源
     * @param damage 伤害值
     * @param timestamp 当前时间（tick）
     */
    void recordDamage(std::unique_ptr<DamageSource> source, f32 damage, u32 timestamp);

    /**
     * @brief 重置追踪器
     *
     * 清除所有记录的战斗数据，通常在重生时调用。
     */
    void reset();

    /**
     * @brief 获取最近的伤害来源
     * @return 最近的战斗条目，没有则返回nullptr
     */
    [[nodiscard]] const CombatEntry* getLastEntry() const;

    /**
     * @brief 获取最佳伤害来源（造成最多伤害的来源）
     * @return 最佳战斗条目，没有则返回nullptr
     */
    [[nodiscard]] const CombatEntry* getBestEntry() const;

    /**
     * @brief 获取最近的伤害者实体
     * @return 最近的造成伤害的实体，没有则返回nullptr
     */
    [[nodiscard]] Entity* getLastAttacker() const;

    /**
     * @brief 获取最佳伤害者实体（造成最多伤害的实体）
     * @return 造成最多伤害的实体，没有则返回nullptr
     */
    [[nodiscard]] Entity* getBestAttacker() const;

    /**
     * @brief 生成死亡消息
     * @return 死亡消息字符串
     */
    [[nodiscard]] String getDeathMessage() const;

    /**
     * @brief 检查是否有战斗记录
     */
    [[nodiscard]] bool hasCombat() const { return !m_entries.empty(); }

    /**
     * @brief 获取总承受伤害
     */
    [[nodiscard]] f32 getTotalDamage() const { return m_totalDamage; }

    /**
     * @brief 获取战斗时长（从第一条记录到最后一条）
     * @return 战斗时长（tick）
     */
    [[nodiscard]] u32 getCombatDuration() const;

    /**
     * @brief 获取战斗条目数量
     */
    [[nodiscard]] size_t getEntryCount() const { return m_entries.size(); }

    /**
     * @brief 检查是否在战斗中
     *
     * 如果最近一段时间内受到伤害，则认为在战斗中。
     * @param currentTime 当前时间（tick）
     * @param threshold 阈值时间（tick），默认100（5秒）
     */
    [[nodiscard]] bool isInCombat(u32 currentTime, u32 threshold = 100) const;

    // 静态常量
    static constexpr u32 COMBAT_TIMEOUT = 100;  // 战斗超时时间（5秒）

private:
    /**
     * @brief 清理过期的战斗条目
     * @param currentTime 当前时间
     */
    void cleanupOldEntries(u32 currentTime);

    /**
     * @brief 更新最佳伤害记录
     */
    void updateBestEntry();

    LivingEntity* m_owner;                  // 拥有者
    std::vector<CombatEntry> m_entries;      // 战斗记录
    f32 m_totalDamage = 0.0f;               // 总承受伤害
    size_t m_bestEntryIndex = 0;            // 最佳伤害记录索引
};

} // namespace mr
