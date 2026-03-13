#pragma once

#include "Path.hpp"
#include "PathPoint.hpp"
#include "PathHeap.hpp"
#include "NodeProcessor.hpp"
#include "Region.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 寻路器
 *
 * 实现 A* 算法寻找从起点到终点的最优路径。
 *
 * 参考 MC 1.16.5 PathFinder
 */
class PathFinder {
public:
    /**
     * @brief 构造函数
     * @param processor 节点处理器
     */
    explicit PathFinder(std::unique_ptr<NodeProcessor> processor)
        : m_nodeProcessor(std::move(processor))
    {
    }

    ~PathFinder() = default;

    // ========== 配置 ==========

    /**
     * @brief 设置世界区域
     */
    void setRegion(const Region* region) {
        if (m_nodeProcessor) {
            m_nodeProcessor->setRegion(region);
        }
    }

    /**
     * @brief 设置实体尺寸
     */
    void setEntitySize(f32 width, f32 height) {
        if (m_nodeProcessor) {
            m_nodeProcessor->setEntitySize(width, height);
        }
    }

    /**
     * @brief 设置最大搜索距离
     */
    void setMaxSearchDistance(i32 distance) { m_maxSearchDistance = distance; }

    /**
     * @brief 设置最大搜索节点数
     */
    void setMaxNodes(i32 maxNodes) { m_maxNodes = maxNodes; }

    /**
     * @brief 设置搜索范围（用于性能优化）
     */
    void setSearchRange(i32 range) { m_searchRange = range; }

    // ========== 寻路 ==========

    /**
     * @brief 寻找到坐标的路径
     * @param startX 起点X
     * @param startY 起点Y
     * @param startZ 起点Z
     * @param targetX 终点X
     * @param targetY 终点Y
     * @param targetZ 终点Z
     * @return 找到的路径，如果找不到返回空路径
     */
    [[nodiscard]] Path findPath(i32 startX, i32 startY, i32 startZ,
                                 i32 targetX, i32 targetY, i32 targetZ) {
        return findPath(startX, startY, startZ, targetX, targetY, targetZ, m_maxSearchDistance);
    }

    /**
     * @brief 寻找到坐标的路径（带最大距离）
     * @param startX 起点X
     * @param startY 起点Y
     * @param startZ 起点Z
     * @param targetX 终点X
     * @param targetY 终点Y
     * @param targetZ 终点Z
     * @param maxDistance 最大搜索距离
     * @return 找到的路径，如果找不到返回空路径
     */
    [[nodiscard]] Path findPath(i32 startX, i32 startY, i32 startZ,
                                 i32 targetX, i32 targetY, i32 targetZ,
                                 i32 maxDistance);

    /**
     * @brief 寻找到目标范围的路径
     * @param startX 起点X
     * @param startY 起点Y
     * @param startZ 起点Z
     * @param targetX 目标中心X
     * @param targetY 目标中心Y
     * @param targetZ 目标中心Z
     * @param range 目标范围（到达范围内任意点即成功）
     * @return 找到的路径
     */
    [[nodiscard]] Path findPathToRange(i32 startX, i32 startY, i32 startZ,
                                        i32 targetX, i32 targetY, i32 targetZ,
                                        i32 range);

    // ========== 调试 ==========

    /**
     * @brief 获取节点处理器
     */
    [[nodiscard]] NodeProcessor* getNodeProcessor() { return m_nodeProcessor.get(); }

    /**
     * @brief 获取最后搜索的节点数
     */
    [[nodiscard]] i32 getLastSearchedNodes() const { return m_lastSearchedNodes; }

private:
    std::unique_ptr<NodeProcessor> m_nodeProcessor;
    PathHeap m_openSet;
    i32 m_maxSearchDistance = 100;
    i32 m_maxNodes = 2000;
    i32 m_searchRange = 32;
    i32 m_lastSearchedNodes = 0;

    // ========== 内部方法 ==========

    /**
     * @brief 启发式函数：估算从当前点到目标的代价
     */
    [[nodiscard]] static f32 heuristic(i32 x, i32 y, i32 z,
                                         i32 targetX, i32 targetY, i32 targetZ) {
        // 使用曼哈顿距离作为启发式
        i32 dx = std::abs(x - targetX);
        i32 dy = std::abs(y - targetY);
        i32 dz = std::abs(z - targetZ);
        return static_cast<f32>(dx + dy + dz);
    }

    /**
     * @brief 计算两点间的移动代价
     */
    [[nodiscard]] static f32 getMovementCost(const PathPoint& from, const PathPoint& to) {
        // 水平移动代价为1，对角线为1.414，垂直移动为1
        i32 dx = std::abs(to.x() - from.x());
        i32 dy = std::abs(to.y() - from.y());
        i32 dz = std::abs(to.z() - from.z());

        f32 horizontalDist = static_cast<f32>(dx + dz);
        f32 verticalDist = static_cast<f32>(dy);

        // 对角线移动（水平方向同时移动）
        bool isDiagonal = (dx > 0 && dz > 0);

        f32 baseCost = isDiagonal ? horizontalDist * 0.707f : horizontalDist;
        return baseCost + verticalDist;
    }

    /**
     * @brief 检查是否到达目标
     */
    [[nodiscard]] static bool isTargetReached(const PathPoint& point,
                                               i32 targetX, i32 targetY, i32 targetZ,
                                               i32 tolerance = 0) {
        return std::abs(point.x() - targetX) <= tolerance &&
               std::abs(point.y() - targetY) <= tolerance &&
               std::abs(point.z() - targetZ) <= tolerance;
    }

    /**
     * @brief 检查节点是否在搜索范围内
     */
    [[nodiscard]] bool isInSearchRange(i32 x, i32 y, i32 z,
                                        i32 startX, i32 startY, i32 startZ,
                                        i32 targetX, i32 targetY, i32 targetZ) const {
        // 检查节点是否在起点和终点之间的范围内
        i32 minX = std::min(startX, targetX) - m_searchRange;
        i32 maxX = std::max(startX, targetX) + m_searchRange;
        i32 minY = std::min(startY, targetY) - m_searchRange / 2;
        i32 maxY = std::max(startY, targetY) + m_searchRange / 2;
        i32 minZ = std::min(startZ, targetZ) - m_searchRange;
        i32 maxZ = std::max(startZ, targetZ) + m_searchRange;

        return x >= minX && x <= maxX &&
               y >= minY && y <= maxY &&
               z >= minZ && z <= maxZ;
    }
};

} // namespace mc::entity::ai::pathfinding
