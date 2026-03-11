#pragma once

#include "AnimalEntity.hpp"

namespace mr {

/**
 * @brief 牛实体
 *
 * 可被挤奶的被动动物，用小麦繁殖。
 *
 * 参考 MC 1.16.5 CowEntity
 */
class CowEntity : public AnimalEntity {
public:
    CowEntity(LegacyEntityType type, EntityId id);
    ~CowEntity() override = default;

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // TODO: 挤奶逻辑
};

} // namespace mr
