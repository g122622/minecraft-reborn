#pragma once

#include "EntityType.hpp"
#include "EntityRegistry.hpp"
#include "animal/PigEntity.hpp"
#include "animal/CowEntity.hpp"
#include "animal/SheepEntity.hpp"
#include "animal/ChickenEntity.hpp"
#include "ItemEntity.hpp"
#include <spdlog/spdlog.h>
#include <mutex>

namespace mc {
namespace entity {

/**
 * @brief 原版实体初始化器
 *
 * 注册所有原版实体类型到实体注册表。
 * 必须在服务器启动时或客户端初始化时调用。
 *
 * 参考 MC 1.16.5 EntityType 注册
 */
class VanillaEntities {
public:
    /**
     * @brief 注册所有原版实体类型
     *
     * 包括动物、怪物和其他实体类型。
     * 此方法线程安全，可以安全地多次调用，后续调用将被忽略。
     */
    static void registerAll() {
        std::call_once(s_onceFlag, []() {
            doRegisterAll();
        });
    }

    /**
     * @brief 获取实体类型的本地化名称
     * @param typeId 实体类型ID
     * @return 本地化名称键（如 entity.minecraft.pig）
     */
    static String getLocalizedNameKey(EntityTypeId typeId) {
        const auto* type = EntityRegistry::instance().getType(typeId);
        if (!type) {
            return "entity.minecraft.unknown";
        }

        // 将 minecraft:pig 转换为 entity.minecraft.pig
        const String& name = type->name();
        if (name.find(':') != String::npos) {
            return "entity." + name;
        }
        return "entity.minecraft." + name;
    }

private:
    static inline std::once_flag s_onceFlag;

    static void doRegisterAll() {
        auto& registry = EntityRegistry::instance();

        // ========== 动物 ==========
        // 猪
        registry.registerType(
            EntityTypes::PIG,
            EntityType::Builder(&PigEntity::create, EntityClassification::Creature)
                .size(0.9f, 0.9f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build()
        );

        // 牛
        registry.registerType(
            EntityTypes::COW,
            EntityType::Builder(&CowEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.4f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build()
        );

        // 羊
        registry.registerType(
            EntityTypes::SHEEP,
            EntityType::Builder(&SheepEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.3f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build()
        );

        // 鸡
        registry.registerType(
            EntityTypes::CHICKEN,
            EntityType::Builder(&ChickenEntity::create, EntityClassification::Creature)
                .size(0.4f, 0.7f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build()
        );

        // ========== 物品 ==========
        registry.registerType(
            EntityTypes::ITEM,
            EntityType::Builder(&ItemEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(20)
                .canSummon(true)
                .build()
        );

        // ========== 玩家 ==========
        // 玩家实体类型由 Player 类自行管理

        spdlog::info("Registered {} entity types", registry.size());
    }
};

} // namespace entity
} // namespace mc
