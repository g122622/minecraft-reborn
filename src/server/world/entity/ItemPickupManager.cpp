#include "ItemPickupManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "common/entity/ItemEntity.hpp"
#include "common/entity/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/ItemStack.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/EntityPackets.hpp"
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

    // 检查是否是玩家
    if (player.legacyType() != LegacyEntityType::Player) {
        return false;
    }

    Player* playerEntity = static_cast<Player*>(&player);

    // 使用 ItemEntity::onPlayerPickup 处理拾取逻辑
    // 这确保所有者 UUID 检查等逻辑在一处实现
    bool fullyPickedUp = itemEntity.onPlayerPickup(*playerEntity);

    if (fullyPickedUp || itemEntity.getItemStack().isEmpty()) {
        // 完全拾取，发送背包更新和实体销毁包
        sendInventoryUpdate(world, *playerEntity);
        sendEntityDestroy(world, itemEntity.id(), player.id());
        return fullyPickedUp;
    }

    // 部分拾取，发送背包更新
    sendInventoryUpdate(world, *playerEntity);
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

    // 所有者限制检查由 ItemEntity::onPlayerPickup 处理
    // 这里只检查基本的可拾取状态

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
    // 首先发送 CollectItemPacket 触发拾取动画
    // 这会让客户端播放物品飞向玩家的动画
    network::CollectItemPacket collectPacket;
    collectPacket.setCollectedEntityId(static_cast<u32>(entityId));
    collectPacket.setCollectorEntityId(static_cast<u32>(collectorId));
    collectPacket.setPickupItemCount(1);  // 默认拾取数量

    auto collectResult = collectPacket.serialize();
    if (collectResult.success()) {
        network::PacketSerializer fullPacket;
        fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + collectResult.value().size()));
        fullPacket.writeU16(static_cast<u16>(network::PacketType::CollectItem));
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeBytes(collectResult.value());

        // 广播给所有玩家
        world.broadcastPacket(fullPacket.buffer());
    }

    // 然后发送实体销毁包
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
