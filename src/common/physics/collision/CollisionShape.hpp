#pragma once

#include "../../util/AxisAlignedBB.hpp"
#include "../../util/Direction.hpp"
#include "../../core/Types.hpp"
#include <vector>

namespace mr {

// Axis 枚举已移至 Direction.hpp

/**
 * @brief 碰撞形状
 *
 * 简化版的VoxelShape，支持空、完整方块、简单盒三种类型。
 * 碰撞箱使用方块本地坐标（0-1范围）。
 *
 * 注意：
 * - 空形状表示没有碰撞（如空气、水、岩浆）
 * - 完整方块是最常见的形状，有优化路径
 * - 简单盒支持自定义碰撞箱（如台阶、楼梯等）
 */
class CollisionShape {
public:
    /**
     * @brief 形状类型
     */
    enum class Type : u8 {
        Empty,      // 无碰撞
        FullBlock,  // 完整方块 (0,0,0) -> (1,1,1)
        SimpleBox   // 简单盒（可能有多个碰撞箱）
    };

    // 默认构造：空形状
    CollisionShape() noexcept : m_type(Type::Empty) {}

    /**
     * @brief 创建空形状（无碰撞）
     */
    [[nodiscard]] static CollisionShape empty() noexcept {
        return CollisionShape();
    }

    /**
     * @brief 创建完整方块形状
     */
    [[nodiscard]] static CollisionShape fullBlock() noexcept {
        CollisionShape shape;
        shape.m_type = Type::FullBlock;
        shape.m_boxes.emplace_back(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return shape;
    }

    /**
     * @brief 创建简单盒形状
     * @param minX, minY, minZ 最小坐标（方块本地坐标，0-1范围）
     * @param maxX, maxY, maxZ 最大坐标（方块本地坐标，0-1范围）
     */
    [[nodiscard]] static CollisionShape box(f32 minX, f32 minY, f32 minZ,
                                             f32 maxX, f32 maxY, f32 maxZ) noexcept {
        CollisionShape shape;
        shape.m_type = Type::SimpleBox;
        shape.m_boxes.emplace_back(minX, minY, minZ, maxX, maxY, maxZ);
        return shape;
    }

    /**
     * @brief 添加额外的碰撞盒
     * 用于复杂形状（如楼梯）
     */
    CollisionShape& addBox(f32 minX, f32 minY, f32 minZ,
                            f32 maxX, f32 maxY, f32 maxZ) {
        m_boxes.emplace_back(minX, minY, minZ, maxX, maxY, maxZ);
        if (m_boxes.size() > 1) {
            m_type = Type::SimpleBox;
        }
        return *this;
    }

    // 查询
    [[nodiscard]] bool isEmpty() const noexcept { return m_type == Type::Empty; }
    [[nodiscard]] bool isFullBlock() const noexcept { return m_type == Type::FullBlock; }
    [[nodiscard]] Type type() const noexcept { return m_type; }
    [[nodiscard]] const std::vector<AxisAlignedBB>& boxes() const noexcept { return m_boxes; }
    [[nodiscard]] size_t boxCount() const noexcept { return m_boxes.size(); }

    /**
     * @brief 获取世界坐标碰撞箱
     * @param blockX, blockY, blockZ 方块在世界中的位置
     * @return 世界坐标下的碰撞箱列表
     */
    [[nodiscard]] std::vector<AxisAlignedBB> getWorldBoxes(i32 blockX, i32 blockY, i32 blockZ) const {
        std::vector<AxisAlignedBB> worldBoxes;
        worldBoxes.reserve(m_boxes.size());

        f32 bx = static_cast<f32>(blockX);
        f32 by = static_cast<f32>(blockY);
        f32 bz = static_cast<f32>(blockZ);

        for (const auto& localBox : m_boxes) {
            worldBoxes.emplace_back(
                bx + localBox.minX, by + localBox.minY, bz + localBox.minZ,
                bx + localBox.maxX, by + localBox.maxY, bz + localBox.maxZ
            );
        }
        return worldBoxes;
    }

    /**
     * @brief 检测与实体碰撞箱是否相交
     * @param entityBox 实体的世界坐标碰撞箱
     * @param blockX, blockY, blockZ 方块位置
     * @return 是否相交
     */
    [[nodiscard]] bool intersects(const AxisAlignedBB& entityBox,
                                   i32 blockX, i32 blockY, i32 blockZ) const {
        if (isEmpty()) return false;

        f32 bx = static_cast<f32>(blockX);
        f32 by = static_cast<f32>(blockY);
        f32 bz = static_cast<f32>(blockZ);

        for (const auto& localBox : m_boxes) {
            AxisAlignedBB worldBox(
                bx + localBox.minX, by + localBox.minY, bz + localBox.minZ,
                bx + localBox.maxX, by + localBox.maxY, bz + localBox.maxZ
            );
            if (entityBox.intersects(worldBox)) {
                return true;
            }
        }
        return false;
    }

private:
    Type m_type = Type::Empty;
    std::vector<AxisAlignedBB> m_boxes;  // 方块本地坐标 (0-1范围)
};

} // namespace mr
