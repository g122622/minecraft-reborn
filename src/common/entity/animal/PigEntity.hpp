#pragma once

#include "AnimalEntity.hpp"

namespace mr {

/**
 * @brief 猪实体
 *
 * 最基础的被动动物，可被骑乘（使用鞍）。
 *
 * 参考 MC 1.16.5 PigEntity
 */
class PigEntity : public AnimalEntity {
public:
    PigEntity(LegacyEntityType type, EntityId id);
    ~PigEntity() override = default;

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 骑乘 ==========

    /**
     * @brief 是否装备了鞍
     */
    [[nodiscard]] bool hasSaddle() const { return m_hasSaddle; }

    /**
     * @brief 设置鞍状态
     */
    void setSaddle(bool saddle) { m_hasSaddle = saddle; }

    // TODO: 骑乘逻辑

private:
    bool m_hasSaddle = false;
};

} // namespace mr
