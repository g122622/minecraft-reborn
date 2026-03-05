#pragma once

#include "../core/Types.hpp"
#include "MathUtils.hpp"

#include <cmath>
#include <functional>

namespace mr {

/**
 * @brief 3D向量类
 *
 * 用于表示位置、方向、速度等3D量
 */
class Vector3 {
public:
    f32 x, y, z;

    // 构造函数
    Vector3() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
    {
    }

    Vector3(f32 x, f32 y, f32 z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {
    }

    explicit Vector3(f32 value) noexcept
        : x(value)
        , y(value)
        , z(value)
    {
    }

    // 静态常量
    static const Vector3 ZERO;
    static const Vector3 ONE;
    static const Vector3 UP;
    static const Vector3 DOWN;
    static const Vector3 LEFT;
    static const Vector3 RIGHT;
    static const Vector3 FORWARD;
    static const Vector3 BACK;

    // 算术运算
    [[nodiscard]] Vector3 operator+(const Vector3& other) const noexcept
    {
        return {x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]] Vector3 operator-(const Vector3& other) const noexcept
    {
        return {x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] Vector3 operator*(f32 scalar) const noexcept
    {
        return {x * scalar, y * scalar, z * scalar};
    }

    [[nodiscard]] Vector3 operator*(const Vector3& other) const noexcept
    {
        return {x * other.x, y * other.y, z * other.z};
    }

    [[nodiscard]] Vector3 operator/(f32 scalar) const noexcept
    {
        return {x / scalar, y / scalar, z / scalar};
    }

    [[nodiscard]] Vector3 operator/(const Vector3& other) const noexcept
    {
        return {x / other.x, y / other.y, z / other.z};
    }

    Vector3& operator+=(const Vector3& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3& operator*=(f32 scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vector3& operator/=(f32 scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    [[nodiscard]] Vector3 operator-() const noexcept
    {
        return {-x, -y, -z};
    }

    // 比较运算
    [[nodiscard]] bool operator==(const Vector3& other) const noexcept
    {
        return math::approxEqual(x, other.x) &&
               math::approxEqual(y, other.y) &&
               math::approxEqual(z, other.z);
    }

    [[nodiscard]] bool operator!=(const Vector3& other) const noexcept
    {
        return !(*this == other);
    }

    // 向量运算
    [[nodiscard]] f32 length() const noexcept
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] f32 lengthSquared() const noexcept
    {
        return x * x + y * y + z * z;
    }

    [[nodiscard]] f32 lengthHorizontal() const noexcept
    {
        return std::sqrt(x * x + z * z);
    }

    [[nodiscard]] Vector3 normalized() const noexcept
    {
        const f32 len = length();
        if (len > math::EPSILON) {
            const f32 invLen = 1.0f / len;
            return {x * invLen, y * invLen, z * invLen};
        }
        return ZERO;
    }

    void normalize() noexcept
    {
        *this = normalized();
    }

    [[nodiscard]] f32 dot(const Vector3& other) const noexcept
    {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] Vector3 cross(const Vector3& other) const noexcept
    {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    [[nodiscard]] f32 distance(const Vector3& other) const noexcept
    {
        return (*this - other).length();
    }

    [[nodiscard]] f32 distanceSquared(const Vector3& other) const noexcept
    {
        return (*this - other).lengthSquared();
    }

    [[nodiscard]] f32 distanceHorizontal(const Vector3& other) const noexcept
    {
        const f32 dx = x - other.x;
        const f32 dz = z - other.z;
        return std::sqrt(dx * dx + dz * dz);
    }

    [[nodiscard]] Vector3 lerp(const Vector3& target, f32 t) const noexcept
    {
        return {
            math::lerp(x, target.x, t),
            math::lerp(y, target.y, t),
            math::lerp(z, target.z, t)
        };
    }

    // 角度计算
    [[nodiscard]] f32 pitch() const noexcept
    {
        return -std::asin(y / length());
    }

    [[nodiscard]] f32 yaw() const noexcept
    {
        return std::atan2(z, x);
    }

    // 从角度创建方向向量
    [[nodiscard]] static Vector3 fromAngles(f32 pitch, f32 yaw) noexcept
    {
        const f32 cosPitch = std::cos(pitch);
        return {
            cosPitch * std::cos(yaw),
            -std::sin(pitch),
            cosPitch * std::sin(yaw)
        };
    }

    // 坐标转换
    [[nodiscard]] BlockCoord blockX() const noexcept { return static_cast<BlockCoord>(std::floor(x)); }
    [[nodiscard]] BlockCoord blockY() const noexcept { return static_cast<BlockCoord>(std::floor(y)); }
    [[nodiscard]] BlockCoord blockZ() const noexcept { return static_cast<BlockCoord>(std::floor(z)); }

    // 取整
    [[nodiscard]] Vector3 floored() const noexcept
    {
        return {static_cast<f32>(std::floor(x)),
                static_cast<f32>(std::floor(y)),
                static_cast<f32>(std::floor(z))};
    }

    [[nodiscard]] Vector3 ceiled() const noexcept
    {
        return {static_cast<f32>(std::ceil(x)),
                static_cast<f32>(std::ceil(y)),
                static_cast<f32>(std::ceil(z))};
    }
};

// 标量 * 向量
[[nodiscard]] inline Vector3 operator*(f32 scalar, const Vector3& vec) noexcept
{
    return vec * scalar;
}

// 静态常量定义
inline const Vector3 Vector3::ZERO{0.0f, 0.0f, 0.0f};
inline const Vector3 Vector3::ONE{1.0f, 1.0f, 1.0f};
inline const Vector3 Vector3::UP{0.0f, 1.0f, 0.0f};
inline const Vector3 Vector3::DOWN{0.0f, -1.0f, 0.0f};
inline const Vector3 Vector3::LEFT{-1.0f, 0.0f, 0.0f};
inline const Vector3 Vector3::RIGHT{1.0f, 0.0f, 0.0f};
inline const Vector3 Vector3::FORWARD{0.0f, 0.0f, 1.0f};
inline const Vector3 Vector3::BACK{0.0f, 0.0f, -1.0f};

// 类型别名
using Position = Vector3;
using Velocity = Vector3;

} // namespace mr

// 哈希函数支持
namespace std {
template<>
struct hash<mr::Vector3> {
    size_t operator()(const mr::Vector3& v) const noexcept
    {
        size_t h1 = std::hash<float>{}(v.x);
        size_t h2 = std::hash<float>{}(v.y);
        size_t h3 = std::hash<float>{}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std
