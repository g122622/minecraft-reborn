#pragma once

#include "../../core/Types.hpp"
#include "MathUtils.hpp"

#include <cmath>
#include <functional>

namespace mc {

/**
 * @brief 2D向量类
 *
 * 用于表示平面位置、方向、UV坐标等2D量。
 * 移动输入（forward, strafe）等场景使用。
 */
class Vector2 {
public:
    f32 x, y;

    // 构造函数
    Vector2() noexcept
        : x(0.0f)
        , y(0.0f)
    {
    }

    Vector2(f32 x, f32 y) noexcept
        : x(x)
        , y(y)
    {
    }

    explicit Vector2(f32 value) noexcept
        : x(value)
        , y(value)
    {
    }

    // 静态常量
    static const Vector2 ZERO;
    static const Vector2 ONE;
    static const Vector2 UP;
    static const Vector2 DOWN;
    static const Vector2 LEFT;
    static const Vector2 RIGHT;

    // 算术运算
    [[nodiscard]] Vector2 operator+(const Vector2& other) const noexcept
    {
        return {x + other.x, y + other.y};
    }

    [[nodiscard]] Vector2 operator-(const Vector2& other) const noexcept
    {
        return {x - other.x, y - other.y};
    }

    [[nodiscard]] Vector2 operator*(f32 scalar) const noexcept
    {
        return {x * scalar, y * scalar};
    }

    [[nodiscard]] Vector2 operator*(const Vector2& other) const noexcept
    {
        return {x * other.x, y * other.y};
    }

    [[nodiscard]] Vector2 operator/(f32 scalar) const noexcept
    {
        return {x / scalar, y / scalar};
    }

    [[nodiscard]] Vector2 operator/(const Vector2& other) const noexcept
    {
        return {x / other.x, y / other.y};
    }

    Vector2& operator+=(const Vector2& other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2& operator*=(f32 scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vector2& operator/=(f32 scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    [[nodiscard]] Vector2 operator-() const noexcept
    {
        return {-x, -y};
    }

    // 比较运算
    [[nodiscard]] bool operator==(const Vector2& other) const noexcept
    {
        return math::approxEqual(x, other.x) &&
               math::approxEqual(y, other.y);
    }

    [[nodiscard]] bool operator!=(const Vector2& other) const noexcept
    {
        return !(*this == other);
    }

    // 向量运算
    [[nodiscard]] f32 length() const noexcept
    {
        return std::sqrt(x * x + y * y);
    }

    [[nodiscard]] f32 lengthSquared() const noexcept
    {
        return x * x + y * y;
    }

    [[nodiscard]] Vector2 normalized() const noexcept
    {
        const f32 len = length();
        if (len > math::EPSILON) {
            const f32 invLen = 1.0f / len;
            return {x * invLen, y * invLen};
        }
        return ZERO;
    }

    void normalize() noexcept
    {
        *this = normalized();
    }

    [[nodiscard]] f32 dot(const Vector2& other) const noexcept
    {
        return x * other.x + y * other.y;
    }

    /**
     * @brief 2D叉积（返回标量）
     *
     * 结果为 this.x * other.y - this.y * other.x
     * 正值表示 other 在 this 的逆时针方向
     */
    [[nodiscard]] f32 cross(const Vector2& other) const noexcept
    {
        return x * other.y - y * other.x;
    }

    [[nodiscard]] f32 distance(const Vector2& other) const noexcept
    {
        return (*this - other).length();
    }

    [[nodiscard]] f32 distanceSquared(const Vector2& other) const noexcept
    {
        return (*this - other).lengthSquared();
    }

    [[nodiscard]] Vector2 lerp(const Vector2& target, f32 t) const noexcept
    {
        return {
            math::lerp(x, target.x, t),
            math::lerp(y, target.y, t)
        };
    }

    /**
     * @brief 旋转向量
     * @param angle 弧度角
     * @return 旋转后的向量
     */
    [[nodiscard]] Vector2 rotated(f32 angle) const noexcept
    {
        const f32 c = std::cos(angle);
        const f32 s = std::sin(angle);
        return {x * c - y * s, x * s + y * c};
    }

    /**
     * @brief 获取垂直向量（逆时针旋转90度）
     */
    [[nodiscard]] Vector2 perpendicular() const noexcept
    {
        return {-y, x};
    }

    /**
     * @brief 获取向量角度（弧度）
     */
    [[nodiscard]] f32 angle() const noexcept
    {
        return std::atan2(y, x);
    }

    /**
     * @brief 从角度创建单位向量
     * @param angle 弧度角
     * @return 单位向量
     */
    [[nodiscard]] static Vector2 fromAngle(f32 angle) noexcept
    {
        return {std::cos(angle), std::sin(angle)};
    }
};

// 标量 * 向量
[[nodiscard]] inline Vector2 operator*(f32 scalar, const Vector2& vec) noexcept
{
    return vec * scalar;
}

// 静态常量定义
inline const Vector2 Vector2::ZERO{0.0f, 0.0f};
inline const Vector2 Vector2::ONE{1.0f, 1.0f};
inline const Vector2 Vector2::UP{0.0f, 1.0f};
inline const Vector2 Vector2::DOWN{0.0f, -1.0f};
inline const Vector2 Vector2::LEFT{-1.0f, 0.0f};
inline const Vector2 Vector2::RIGHT{1.0f, 0.0f};

} // namespace mc

// 哈希函数支持
namespace std {
template<>
struct hash<mc::Vector2> {
    size_t operator()(const mc::Vector2& v) const noexcept
    {
        size_t h1 = std::hash<float>{}(v.x);
        size_t h2 = std::hash<float>{}(v.y);
        return h1 ^ (h2 << 1);
    }
};
} // namespace std
