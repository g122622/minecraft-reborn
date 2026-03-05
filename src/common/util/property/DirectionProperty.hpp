#pragma once

#include "EnumProperty.hpp"
#include <functional>

namespace mr {

/**
 * @brief 方向属性
 *
 * 专门用于表示方向的枚举属性，提供便捷的方向特定功能。
 *
 * 参考: net.minecraft.state.DirectionProperty
 *
 * 用法示例:
 * @code
 * // 创建包含所有方向的方向属性
 * auto facing = DirectionProperty::create("facing");
 *
 * // 创建只包含水平方向的方向属性
 * auto horizontalFacing = DirectionProperty::createHorizontal("facing");
 *
 * // 创建特定方向集合的属性
 * auto facingExceptUp = DirectionProperty::create("facing", [](Direction d) {
 *     return d != Direction::Up;
 * });
 * @endcode
 */
class DirectionProperty : public EnumProperty<Direction> {
public:
    /**
     * @brief 创建包含所有方向的方向属性
     * @param name 属性名称
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<DirectionProperty> create(const String& name) {
        return create(name, Directions::all());
    }

    /**
     * @brief 创建包含所有方向的方向属性
     * @param name 属性名称
     * @param values 方向列表
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<DirectionProperty> create(const String& name, const std::vector<Direction>& values) {
        return std::unique_ptr<DirectionProperty>(new DirectionProperty(name, values));
    }

    /**
     * @brief 创建包含特定方向的方向属性
     * @param name 属性名称
     * @param values 方向列表（可变参数）
     * @return 属性实例
     */
    template<typename... Dirs>
    [[nodiscard]] static std::unique_ptr<DirectionProperty> create(const String& name, Dirs... values) {
        return create(name, std::vector<Direction>{values...});
    }

    /**
     * @brief 创建只包含水平方向的方向属性 (NORTH, EAST, SOUTH, WEST)
     * @param name 属性名称
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<DirectionProperty> createHorizontal(const String& name) {
        return create(name, Directions::horizontal());
    }

    /**
     * @brief 使用过滤器创建方向属性
     * @param name 属性名称
     * @param filter 过滤函数
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<DirectionProperty> create(const String& name, std::function<bool(Direction)> filter) {
        std::vector<Direction> values;
        for (Direction dir : Directions::all()) {
            if (filter(dir)) {
                values.push_back(dir);
            }
        }
        return create(name, values);
    }

    /**
     * @brief 获取类型名称
     */
    [[nodiscard]] const char* typeName() const override {
        return "DirectionProperty";
    }

private:
    DirectionProperty(const String& name, const std::vector<Direction>& values)
        : EnumProperty<Direction>(name, values) {
    }
};

} // namespace mr
