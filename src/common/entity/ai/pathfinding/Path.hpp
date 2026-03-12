#pragma once

#include "../../../core/Types.hpp"
#include "PathPoint.hpp"
#include <vector>
#include <optional>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 路径对象
 *
 * 表示从起点到终点的完整路径，包含所有路径点。
 *
 * 参考 MC 1.16.5 Path
 */
class Path {
public:
    Path() = default;

    /**
     * @brief 构造函数
     * @param points 路径点数组
     */
    explicit Path(std::vector<PathPoint>&& points)
        : m_points(std::move(points))
    {
    }

    // ========== 路径信息 ==========

    /**
     * @brief 获取路径长度
     */
    [[nodiscard]] size_t length() const { return m_points.size(); }

    /**
     * @brief 检查路径是否为空
     */
    [[nodiscard]] bool empty() const { return m_points.empty(); }

    /**
     * @brief 获取路径终点
     * @return 终点路径点，如果路径为空返回nullptr
     */
    [[nodiscard]] const PathPoint* getEnd() const {
        return m_points.empty() ? nullptr : &m_points.back();
    }

    /**
     * @brief 获取路径起点
     * @return 起点路径点，如果路径为空返回nullptr
     */
    [[nodiscard]] const PathPoint* getStart() const {
        return m_points.empty() ? nullptr : &m_points.front();
    }

    // ========== 路径点访问 ==========

    /**
     * @brief 获取指定索引的路径点
     * @param index 索引
     * @return 路径点指针，如果索引无效返回nullptr
     */
    [[nodiscard]] const PathPoint* getPoint(size_t index) const {
        if (index >= m_points.size()) {
            return nullptr;
        }
        return &m_points[index];
    }

    /**
     * @brief 获取所有路径点
     */
    [[nodiscard]] const std::vector<PathPoint>& getPoints() const {
        return m_points;
    }

    // ========== 路径导航 ==========

    /**
     * @brief 获取当前目标路径点
     * @return 当前目标路径点，如果已到达终点返回nullptr
     */
    [[nodiscard]] const PathPoint* getCurrentTarget() const {
        if (m_points.empty() || m_currentIndex >= static_cast<i32>(m_points.size())) {
            return nullptr;
        }
        return &m_points[static_cast<size_t>(m_currentIndex)];
    }

    /**
     * @brief 获取当前路径点索引
     */
    [[nodiscard]] i32 getCurrentIndex() const { return m_currentIndex; }

    /**
     * @brief 前进到下一个路径点
     * @return 是否还有下一个路径点
     */
    bool advance() {
        ++m_currentIndex;
        return m_currentIndex < static_cast<i32>(m_points.size());
    }

    /**
     * @brief 检查是否已到达终点
     */
    [[nodiscard]] bool isFinished() const {
        return m_points.empty() || m_currentIndex >= static_cast<i32>(m_points.size());
    }

    /**
     * @brief 重置路径进度
     */
    void reset() {
        m_currentIndex = 0;
    }

    /**
     * @brief 设置当前索引
     */
    void setCurrentIndex(i32 index) {
        m_currentIndex = std::max(0, std::min(index, static_cast<i32>(m_points.size()) - 1));
    }

    // ========== 路径操作 ==========

    /**
     * @brief 添加路径点（用于路径构建）
     */
    void addPoint(const PathPoint& point) {
        m_points.push_back(point);
    }

    /**
     * @brief 添加路径点（移动语义）
     */
    void addPoint(PathPoint&& point) {
        m_points.push_back(std::move(point));
    }

    /**
     * @brief 从终点反向构建路径
     * @param end 终点路径点（通过parent链回溯）
     * @return 构建的路径
     */
    static Path buildFromEnd(const PathPoint* end) {
        std::vector<PathPoint> points;

        // 回溯父节点
        const PathPoint* current = end;
        while (current != nullptr) {
            points.push_back(current->clone());
            current = current->parent();
        }

        // 反转路径（起点在前）
        std::reverse(points.begin(), points.end());

        return Path(std::move(points));
    }

    /**
     * @brief 裁剪路径起点
     * @param count 要移除的起点数量
     */
    void trimStart(size_t count) {
        if (count >= m_points.size()) {
            m_points.clear();
            m_currentIndex = 0;
        } else {
            m_points.erase(m_points.begin(), m_points.begin() + static_cast<i32>(count));
            m_currentIndex = std::max(0, m_currentIndex - static_cast<i32>(count));
        }
    }

    // ========== 状态检查 ==========

    /**
     * @brief 检查路径是否到达了目标位置
     * @param x 目标X坐标
     * @param y 目标Y坐标
     * @param z 目标Z坐标
     * @param tolerance 位置容差
     */
    [[nodiscard]] bool reachesTarget(i32 x, i32 y, i32 z, f32 tolerance = 1.0f) const {
        if (m_points.empty()) {
            return false;
        }

        const PathPoint& end = m_points.back();
        f32 dx = static_cast<f32>(end.x() - x);
        f32 dy = static_cast<f32>(end.y() - y);
        f32 dz = static_cast<f32>(end.z() - z);

        return (dx * dx + dy * dy + dz * dz) <= tolerance * tolerance;
    }

private:
    std::vector<PathPoint> m_points;
    i32 m_currentIndex = 0;
};

} // namespace mc::entity::ai::pathfinding
