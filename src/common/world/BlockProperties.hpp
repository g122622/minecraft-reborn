#pragma once

#include "../core/Types.hpp"
#include <map>
#include <string>
#include <functional>

namespace mr {

/**
 * @brief 方块属性键值对
 *
 * 用于表示方块的状态属性，如 axis=y, facing=north 等
 */
class BlockProperties {
public:
    BlockProperties() = default;

    // 从BlockState的data字段解析属性
    explicit BlockProperties(u16 data);

    // 添加属性
    void set(String key, String value) {
        m_properties[std::move(key)] = std::move(value);
    }

    // 获取属性
    [[nodiscard]] const String* get(const String& key) const {
        auto it = m_properties.find(key);
        if (it != m_properties.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // 检查是否有属性
    [[nodiscard]] bool has(const String& key) const {
        return m_properties.count(key) > 0;
    }

    // 获取属性或默认值
    [[nodiscard]] String getOr(const String& key, const String& defaultValue) const {
        auto* value = get(key);
        return value ? *value : defaultValue;
    }

    // 获取所有属性
    [[nodiscard]] const std::map<String, String>& all() const {
        return m_properties;
    }

    // 转换为状态字符串 (用于模型变体查找)
    // 格式: "axis=y,facing=north" 或 "normal"
    [[nodiscard]] String toStateString() const;

    // 编码为BlockState的data字段
    [[nodiscard]] u16 encode() const;

    // 预定义属性
    [[nodiscard]] bool isAxisY() const { return getOr("axis", "y") == "y"; }
    [[nodiscard]] bool isAxisX() const { return getOr("axis", "y") == "x"; }
    [[nodiscard]] bool isAxisZ() const { return getOr("axis", "y") == "z"; }

    [[nodiscard]] String getFacing() const { return getOr("facing", "north"); }

    // 比较
    bool operator==(const BlockProperties& other) const {
        return m_properties == other.m_properties;
    }
    bool operator!=(const BlockProperties& other) const {
        return !(*this == other);
    }

private:
    std::map<String, String> m_properties;
};

/**
 * @brief 方块属性编码器
 *
 * 将方块属性编码/解码到BlockState的data字段
 */
class BlockPropertyEncoder {
public:
    // 属性ID定义 (用于编码到data字段)
    // data字段是16位，可以存储多个属性
    enum PropertyId : u16 {
        AXIS = 0,       // 2 bits: y=0, x=1, z=2, none=3
        FACING = 2,     // 3 bits: north=0, south=1, west=2, east=3, up=4, down=5
        HALF = 5,       // 1 bit: bottom=0, top=1
        TYPE = 6,       // 2 bits
        POWERED = 8,    // 1 bit
        OPEN = 9,       // 1 bit
        LIT = 10,       // 1 bit
        WATERLOGGED = 11, // 1 bit
        SNOWY = 12,     // 1 bit
    };

    // 编码属性到data
    static u16 encodeAxis(const String& axis);
    static u16 encodeFacing(const String& facing);
    static u16 encodeHalf(const String& half);

    // 解码属性从data
    static String decodeAxis(u16 data);
    static String decodeFacing(u16 data);
    static String decodeHalf(u16 data);

    // 设置属性位
    static u16 setBits(u16 data, u16 offset, u16 mask, u16 value);
    static u16 getBits(u16 data, u16 offset, u16 mask);
};

/**
 * @brief 方块属性工厂
 *
 * 创建常见方块的属性
 */
namespace BlockProps {

// 原木属性
BlockProperties log(u16 data);
BlockProperties log(const String& axis);

// 楼梯属性
BlockProperties stairs(const String& facing, const String& half);

// 门属性
BlockProperties door(const String& facing, const String& half, bool open, bool powered);

// 草方块属性
BlockProperties grassBlock(bool snowy);

// 水/岩浆属性
BlockProperties fluid(u16 level);

} // namespace BlockProps

} // namespace mr
