/**
 * @file CombatEntry.cpp
 * @brief 战斗条目实现
 */

#include "CombatEntry.hpp"

namespace mr {

CombatEntry::CombatEntry(std::unique_ptr<DamageSource> source, f32 damage, u32 timestamp)
    : m_source(std::move(source))
    , m_damage(damage)
    , m_timestamp(timestamp)
{
}

bool CombatEntry::isLivingSource() const {
    return m_source && m_source->isEntitySource();
}

bool CombatEntry::isPlayerSource() const {
    return m_source && m_source->isPlayerSource();
}

} // namespace mr
