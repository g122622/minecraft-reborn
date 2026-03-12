#pragma once

#include "ClientEntity.hpp"
#include "../../../common/core/Types.hpp"
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace mc::client {

/**
 * @brief 客户端实体管理器
 *
 * 管理客户端所有实体的创建、更新、销毁。
 * 提供实体查询和迭代功能。
 *
 * 参考 MC 1.16.5 World 客户端实体管理
 */
class ClientEntityManager {
public:
    ClientEntityManager() = default;
    ~ClientEntityManager() = default;

    // 禁止拷贝
    ClientEntityManager(const ClientEntityManager&) = delete;
    ClientEntityManager& operator=(const ClientEntityManager&) = delete;

    // ========== 实体管理 ==========

    /**
     * @brief 创建实体
     * @param id 实体ID
     * @param typeId 实体类型标识符
     * @return 创建的实体指针，如果ID已存在则返回nullptr
     */
    [[nodiscard]] ClientEntity* spawnEntity(EntityId id, const String& typeId);

    /**
     * @brief 移除实体
     * @param id 实体ID
     * @return 是否成功移除
     */
    bool removeEntity(EntityId id);

    /**
     * @brief 获取实体
     * @param id 实体ID
     * @return 实体指针，如果不存在则返回nullptr
     */
    [[nodiscard]] ClientEntity* getEntity(EntityId id);
    [[nodiscard]] const ClientEntity* getEntity(EntityId id) const;

    /**
     * @brief 检查实体是否存在
     */
    [[nodiscard]] bool hasEntity(EntityId id) const;

    /**
     * @brief 移除所有实体
     */
    void clear();

    /**
     * @brief 移除所有已标记为移除的实体
     */
    void removeDeadEntities();

    // ========== 实体查询 ==========

    /**
     * @brief 获取实体数量
     */
    [[nodiscard]] size_t entityCount() const { return m_entities.size(); }

    /**
     * @brief 遍历所有实体
     * @param func 回调函数
     */
    void forEachEntity(std::function<void(ClientEntity&)> func);

    /**
     * @brief 遍历所有实体（const版本）
     * @param func 回调函数
     */
    void forEachEntity(std::function<void(const ClientEntity&)> func) const;

    /**
     * @brief 获取指定类型的所有实体
     * @param typeId 实体类型标识符
     * @return 实体ID列表
     */
    [[nodiscard]] std::vector<EntityId> getEntitiesByType(const String& typeId) const;

    /**
     * @brief 获取指定范围内的实体
     * @param x 中心X
     * @param y 中心Y
     * @param z 中心Z
     * @param radius 半径
     * @return 范围内的实体ID列表
     */
    [[nodiscard]] std::vector<EntityId> getEntitiesInRange(f32 x, f32 y, f32 z, f32 radius) const;

    // ========== 更新 ==========

    /**
     * @brief 更新所有实体（每tick调用）
     */
    void tick();

    /**
     * @brief 更新所有实体的动画状态
     * 在渲染前调用，用于插值计算
     * @param partialTick 部分 tick (0.0-1.0)
     */
    void updateAnimations(f32 partialTick);

private:
    // 实体存储
    std::unordered_map<EntityId, std::unique_ptr<ClientEntity>> m_entities;

    // 待移除的实体列表
    std::vector<EntityId> m_entitiesToRemove;
};

} // namespace mc::client
