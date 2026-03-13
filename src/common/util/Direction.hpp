#pragma once

#include "../core/Types.hpp"
#include <string>
#include <array>
#include <unordered_map>

namespace mc {

/**
 * @brief 方向枚举
 *
 * 表示六个基本方向，参考 net.minecraft.util.Direction
 *
 * 索引顺序: DOWN=0, UP=1, NORTH=2, SOUTH=3, WEST=4, EAST=5
 * 水平顺序: SOUTH=0, WEST=1, NORTH=2, EAST=3
 */
enum class Direction : u8 {
    Down = 0,   // Y- (下)
    Up = 1,     // Y+ (上)
    North = 2,  // Z- (北)
    South = 3,  // Z+ (南)
    West = 4,   // X- (西)
    East = 5,   // X+ (东)
    None = 255  // 无方向/无效
};

/**
 * @brief 坐标轴枚举
 */
enum class Axis : u8 {
    X = 0,
    Y = 1,
    Z = 2
};

/**
 * @brief 轴方向枚举
 */
enum class AxisDirection : u8 {
    Positive = 0,
    Negative = 1
};

/**
 * @brief Direction工具函数
 */
namespace Directions {
    constexpr size_t COUNT = 6;
    constexpr size_t HORIZONTAL_COUNT = 4;

    /**
     * @brief 获取所有方向
     */
    inline std::array<Direction, 6> all() {
        return {
            Direction::Down, Direction::Up,
            Direction::North, Direction::South,
            Direction::West, Direction::East
        };
    }

    /**
     * @brief 获取所有水平方向 (NORTH, EAST, SOUTH, WEST)
     */
    inline std::array<Direction, 4> horizontal() {
        return {
            Direction::North, Direction::East,
            Direction::South, Direction::West
        };
    }

    /**
     * @brief 获取相反方向
     */
    inline Direction opposite(Direction dir) {
        const Direction opposites[] = {
            Direction::Up,    // Down -> Up
            Direction::Down,  // Up -> Down
            Direction::South, // North -> South
            Direction::North, // South -> North
            Direction::East,  // West -> East
            Direction::West   // East -> West
        };
        return opposites[static_cast<size_t>(dir)];
    }

    /**
     * @brief 获取方向的X偏移
     */
    inline i32 xOffset(Direction dir) {
        const i32 offsets[] = {0, 0, 0, 0, -1, 1};
        return offsets[static_cast<size_t>(dir)];
    }

    /**
     * @brief 获取方向的Y偏移
     */
    inline i32 yOffset(Direction dir) {
        const i32 offsets[] = {-1, 1, 0, 0, 0, 0};
        return offsets[static_cast<size_t>(dir)];
    }

    /**
     * @brief 获取方向的Z偏移
     */
    inline i32 zOffset(Direction dir) {
        const i32 offsets[] = {0, 0, -1, 1, 0, 0};
        return offsets[static_cast<size_t>(dir)];
    }

    /**
     * @brief 获取方向的坐标轴
     */
    inline Axis getAxis(Direction dir) {
        const Axis axes[] = {
            Axis::Y, Axis::Y,  // Down, Up
            Axis::Z, Axis::Z,  // North, South
            Axis::X, Axis::X   // West, East
        };
        return axes[static_cast<size_t>(dir)];
    }

    /**
     * @brief 获取方向的轴方向
     */
    inline AxisDirection getAxisDirection(Direction dir) {
        const AxisDirection dirs[] = {
            AxisDirection::Negative, AxisDirection::Positive,  // Down, Up
            AxisDirection::Negative, AxisDirection::Positive,  // North, South
            AxisDirection::Negative, AxisDirection::Positive   // West, East
        };
        return dirs[static_cast<size_t>(dir)];
    }

    /**
     * @brief 判断方向是否水平
     */
    inline bool isHorizontal(Direction dir) {
        return dir == Direction::North || dir == Direction::South ||
               dir == Direction::West || dir == Direction::East;
    }

    /**
     * @brief 判断方向是否垂直
     */
    inline bool isVertical(Direction dir) {
        return dir == Direction::Up || dir == Direction::Down;
    }

    /**
     * @brief 顺时针旋转 (仅水平方向)
     */
    inline Direction rotateY(Direction dir) {
        if (dir == Direction::North) return Direction::East;
        if (dir == Direction::East) return Direction::South;
        if (dir == Direction::South) return Direction::West;
        if (dir == Direction::West) return Direction::North;
        return dir;
    }

    /**
     * @brief 逆时针旋转 (仅水平方向)
     */
    inline Direction rotateYCCW(Direction dir) {
        if (dir == Direction::North) return Direction::West;
        if (dir == Direction::West) return Direction::South;
        if (dir == Direction::South) return Direction::East;
        if (dir == Direction::East) return Direction::North;
        return dir;
    }

    /**
     * @brief 从名称获取方向
     */
    inline Optional<Direction> fromName(StringView name) {
        static const std::unordered_map<String, Direction> nameMap = {
            {"down", Direction::Down},
            {"up", Direction::Up},
            {"north", Direction::North},
            {"south", Direction::South},
            {"west", Direction::West},
            {"east", Direction::East}
        };
        auto it = nameMap.find(String(name));
        return it != nameMap.end() ? Optional<Direction>(it->second) : std::nullopt;
    }

    /**
     * @brief 获取方向名称
     */
    inline String toString(Direction dir) {
        const char* names[] = {"down", "up", "north", "south", "west", "east"};
        return names[static_cast<size_t>(dir)];
    }

    /**
     * @brief 获取方向索引 (0-5)
     */
    inline size_t index(Direction dir) {
        return static_cast<size_t>(dir);
    }

    /**
     * @brief 从索引获取方向
     */
    inline Direction fromIndex(size_t idx) {
        return static_cast<Direction>(idx % 6);
    }

    /**
     * @brief 将Direction转换为BlockFace
     */
    inline BlockFace toBlockFace(Direction dir) {
        switch (dir) {
            case Direction::Down:  return BlockFace::Bottom;
            case Direction::Up:    return BlockFace::Top;
            case Direction::North: return BlockFace::North;
            case Direction::South: return BlockFace::South;
            case Direction::West:  return BlockFace::West;
            case Direction::East:  return BlockFace::East;
            default:               return BlockFace::Bottom;
        }
    }

    /**
     * @brief 从轴向和轴方向获取方向
     */
    inline Direction fromAxisAndDirection(Axis axis, AxisDirection axisDir) {
        if (axis == Axis::X) {
            return axisDir == AxisDirection::Positive ? Direction::East : Direction::West;
        } else if (axis == Axis::Y) {
            return axisDir == AxisDirection::Positive ? Direction::Up : Direction::Down;
        } else {
            return axisDir == AxisDirection::Positive ? Direction::South : Direction::North;
        }
    }

    /**
     * @brief 从向量获取方向
     */
    inline Direction fromVector(f32 x, f32 y, f32 z) {
        f32 maxDot = -1.0f;
        Direction result = Direction::North;

        const Direction dirs[] = {
            Direction::Down, Direction::Up,
            Direction::North, Direction::South,
            Direction::West, Direction::East
        };
        const i32 xOffs[] = {0, 0, 0, 0, -1, 1};
        const i32 yOffs[] = {-1, 1, 0, 0, 0, 0};
        const i32 zOffs[] = {0, 0, -1, 1, 0, 0};

        for (size_t i = 0; i < 6; ++i) {
            f32 dot = x * static_cast<f32>(xOffs[i]) +
                     y * static_cast<f32>(yOffs[i]) +
                     z * static_cast<f32>(zOffs[i]);
            if (dot > maxDot) {
                maxDot = dot;
                result = dirs[i];
            }
        }
        return result;
    }
}

/**
 * @brief Axis工具函数
 */
namespace Axes {
    constexpr size_t COUNT = 3;

    inline std::array<Axis, 3> all() {
        return {Axis::X, Axis::Y, Axis::Z};
    }

    inline Optional<Axis> fromName(StringView name) {
        if (name == "x") return Axis::X;
        if (name == "y") return Axis::Y;
        if (name == "z") return Axis::Z;
        return std::nullopt;
    }

    inline String toString(Axis axis) {
        const char* names[] = {"x", "y", "z"};
        return names[static_cast<size_t>(axis)];
    }

    inline bool isHorizontal(Axis axis) {
        return axis == Axis::X || axis == Axis::Z;
    }

    inline bool isVertical(Axis axis) {
        return axis == Axis::Y;
    }
}

} // namespace mc
