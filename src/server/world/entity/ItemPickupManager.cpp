#include "ItemPickupManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "common/entity/ItemEntity.hpp"
#include "common/entity/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/ItemStack.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace mc::server {

// ============================================================================
// tick
// ============================================================================

void ItemPickupManager::tick(ServerWorld& world) {
    // 处理物品合并
    processItemMerging(world);

    // 遍历所有玩家，检查拾取
    auto players = world.getEntitiesInAABB(
        AxisAlignedBB(-100000, -100, -100000, 100000, 100000, 100000),
        nullptr  // 不过滤任何实体
    );

    for (Entity* entity : players) {
        if (!entity || !entity->isAlive()) {
            continue;
        }

        // 检查是否是玩家
        if (entity->legacyType() != LegacyEntityType::Player) {
            continue;
        }

        checkPlayerPickup(world, *entity);
    }
}

// ============================================================================
// checkPlayerPickup
// ============================================================================

void ItemPickupManager::checkPlayerPickup(ServerWorld& world, Entity& player) {
    // 计算拾取范围
    f32 range = calculatePickupRange(player);
    Vector3 playerPos = player.position();

    // 查找附近的物品实体
    AxisAlignedBB searchBox(
        playerPos.x - range,
        playerPos.y - range,
        playerPos.z - range,
        playerPos.x + range,
        playerPos.y + player.height() + range,
        playerPos.z + range
    );

    auto nearbyEntities = world.getEntitiesInAABB(searchBox, &player);

    for (Entity* entity : nearbyEntities) {
        if (!entity || !entity->isAlive()) {
            continue;
        }

        // 只处理物品实体
        if (entity->legacyType() != LegacyEntityType::Item) {
            continue;
        }

        ItemEntity* itemEntity = static_cast<ItemEntity*>(entity);

        // 检查是否可以拾取
        if (!canPickup(player, *itemEntity)) {
            continue;
        }

        // 尝试拾取
        if (tryPickupItem(world, player, *itemEntity)) {
            // 物品被完全拾取，标记移除
            itemEntity->remove();
        }
    }
}

// ============================================================================
// tryPickupItem
// ============================================================================

bool ItemPickupManager::tryPickupItem(
    ServerWorld& world,
    Entity& player,
    ItemEntity& itemEntity)
{
    // 获取物品堆
    ItemStack& stack = const_cast<ItemStack&>(itemEntity.getItemStack());
    if (stack.isEmpty()) {
        return true;  // 空物品，直接移除
    }

    // 获取玩家背包（通过 Player 类型）
    if (player.legacyType() != LegacyEntityType::Player) {
        return false;
    }

    Player* playerEntity = static_cast<Player*>(&player);
    PlayerInventory& inventory = playerEntity->inventory();

    // 尝试添加到背包
    i32 originalCount = stack.getCount();
    i32 added = inventory.add(stack);

    if (added > 0) {
        // 成功添加部分或全部物品
        // stack 已被 add() 方法修改（剩余数量）

        // 发送背包更新
        sendInventoryUpdate(world, *playerEntity);

        // 发送拾取音效（TODO）
        // 发送拾取统计（TODO）

        if (stack.isEmpty()) {
            // 完全拾取，发送实体销毁包
            sendEntityDestroy(world, itemEntity.id(), player.id());
            return true;
        } else {
            // 部分拾取，物品数量减少
            // ItemEntity 的 stack 已更新，不需要额外操作
            return false;
        }
    }

    // 背包满，无法拾取
    return false;
}

// ============================================================================
// processItemMerging
// ============================================================================

void ItemPickupManager::processItemMerging(ServerWorld& world) {
    // 获取所有物品实体
    auto entities = world.getEntitiesInAABB(
        AxisAlignedBB(-100000, -100, -100000, 100000, 100000, 100000),
        nullptr
    );

    std::vector<ItemEntity*> itemEntities;
    for (Entity* entity : entities) {
        if (entity && entity->isAlive() && entity->legacyType() == LegacyEntityType::Item) {
            itemEntities.push_back(static_cast<ItemEntity*>(entity));
        }
    }

    // 检查合并
    for (size_t i = 0; i < itemEntities.size(); ++i) {
        ItemEntity* item1 = itemEntities[i];
        if (!item1 || !item1->isAlive()) {
            continue;
        }

        for (size_t j = i + 1; j < itemEntities.size(); ++j) {
            ItemEntity* item2 = itemEntities[j];
            if (!item2 || !item2->isAlive()) {
                continue;
            }

            // 检查距离
            Vector3 pos1 = item1->position();
            Vector3 pos2 = item2->position();
            f32 distSq = (pos1 - pos2).lengthSquared();

            if (distSq <= MERGE_RANGE * MERGE_RANGE) {
                // 尝试合并
                if (item1->tryMergeWith(*item2)) {
                    // 合并成功，item2 可能被标记移除
                    spdlog::debug("ItemEntity {} merged into {}", item2->id(), item1->id());
                }
            }
        }
    }
}

// ============================================================================
// calculatePickupRange
// ============================================================================

f32 ItemPickupManager::calculatePickupRange(const Entity& player) const {
    f32 range = PICKUP_RANGE;

    // 创造模式不改变范围
    // if (player.isCreative()) { } // 暂时跳过

    // 潜行时范围缩小
    if (player.legacyType() == LegacyEntityType::Player) {
        const Player* playerEntity = static_cast<const Player*>(&player);
        if (playerEntity->isSneaking()) {
            range = PICKUP_RANGE_SNEAKING;
        }
    }

    return range;
}

// ============================================================================
// canPickup
// ============================================================================

bool ItemPickupManager::canPickup(const Entity& player, const ItemEntity& itemEntity) const {
    // 检查是否可拾取
    if (!itemEntity.canBePickedUp()) {
        return false;
    }

    // 检查所有者限制
    // 物品刚被丢弃时，丢弃者不能立即拾取
    if (!itemEntity.ownerUuid().empty()) {
        // 检查玩家 UUID 是否匹配
        if (player.legacyType() == LegacyEntityType::Player) {
            const Player* playerEntity = static_cast<const Player*>(&player);
            // 如果玩家 UUID 匹配，需要检查拾取延迟
            // 这里简化处理：如果设置了所有者且延迟未过期，则不能拾取
            // 实际 MC 实现中，所有者在 10 ticks 后才能拾取
            if (itemEntity.getAge() < DEFAULT_THROWER_PICKUP_DELAY) {
                // TODO: 检查 UUID 匹配
                // 暂时跳过 UUID 检查
                (void)playerEntity;
            }
        }
    }

    return true;
}

// ============================================================================
// sendInventoryUpdate
// ============================================================================

void ItemPickupManager::sendInventoryUpdate(ServerWorld& world, Player& player) {
    // 获取玩家ID和连接
    PlayerId playerId = player.playerId();
    ServerPlayerData* playerData = world.getPlayer(playerId);
    if (!playerData || !playerData->hasConnection()) {
        return;
    }

    // 获取玩家背包
    PlayerInventory& inventory = player.inventory();

    // 创建背包内容包
    // 使用 ContainerContentPacket 发送完整背包
    std::vector<ItemStack> items;
    items.reserve(inventory::PLAYER_INVENTORY_SIZE);

    for (i32 i = 0; i < inventory::PLAYER_INVENTORY_SIZE; ++i) {
        items.push_back(inventory.getItem(i));
    }

    ContainerContentPacket contentPacket(inventory::PLAYER_CONTAINER_ID, std::move(items));

    // 序列化数据包
    network::PacketSerializer payload;
    contentPacket.serialize(payload);

    // 创建完整数据包（包含头部）
    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + payload.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::ContainerContent));
    fullPacket.writeU16(0);  // flags
    fullPacket.writeU16(0);  // reserved
    fullPacket.writeU16(0);  // padding
    fullPacket.writeBytes(payload.buffer());

    // 发送给玩家
    playerData->send(fullPacket.buffer().data(), fullPacket.buffer().size());
}

// ============================================================================
// sendEntityDestroy
// ============================================================================

void ItemPickupManager::sendEntityDestroy(
    ServerWorld& world,
    EntityId entityId,
    EntityId collectorId)
{
    // 发送实体销毁包给所有追踪此实体的玩家
    // 这里简化处理，使用广播
    (void)collectorId;

    // 创建实体销毁包
    // SPacketDestroyEntities: VarInt count, Array[VarInt] entityIds
    network::PacketSerializer ser;
    ser.writeVarInt(static_cast<i32>(network::PacketType::EntityDestroy));
    ser.writeVarInt(1);  // count = 1
    ser.writeVarInt(static_cast<i32>(entityId));

    // 创建完整数据包（包含长度前缀）
    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::EntityDestroy));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    // 广播给所有玩家
    world.broadcastPacket(fullPacket.buffer());
}

} // namespace mc::server
