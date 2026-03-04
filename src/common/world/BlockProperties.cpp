#include "BlockProperties.hpp"
#include <algorithm>

namespace mr {

// ============================================================================
// BlockProperties 实现
// ============================================================================

BlockProperties::BlockProperties(u16 data) {
    // 从data解码属性
    String axis = BlockPropertyEncoder::decodeAxis(data);
    if (!axis.empty() && axis != "none") {
        set("axis", axis);
    }

    String facing = BlockPropertyEncoder::decodeFacing(data);
    if (!facing.empty()) {
        set("facing", facing);
    }

    String half = BlockPropertyEncoder::decodeHalf(data);
    if (!half.empty()) {
        set("half", half);
    }
}

String BlockProperties::toStateString() const {
    if (m_properties.empty()) {
        return "normal";
    }

    String result;
    bool first = true;

    // 按键排序以保证一致性
    std::vector<std::pair<String, String>> sorted(m_properties.begin(), m_properties.end());
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& [key, value] : sorted) {
        if (!first) {
            result += ",";
        }
        result += key + "=" + value;
        first = false;
    }

    return result;
}

u16 BlockProperties::encode() const {
    u16 data = 0;

    // 编码axis
    if (has("axis")) {
        data = BlockPropertyEncoder::setBits(data, 0, 0x3, BlockPropertyEncoder::encodeAxis(*get("axis")));
    }

    // 编码facing
    if (has("facing")) {
        data = BlockPropertyEncoder::setBits(data, 2, 0x7, BlockPropertyEncoder::encodeFacing(*get("facing")));
    }

    // 编码half
    if (has("half")) {
        data = BlockPropertyEncoder::setBits(data, 5, 0x1, BlockPropertyEncoder::encodeHalf(*get("half")));
    }

    // 其他属性...

    return data;
}

// ============================================================================
// BlockPropertyEncoder 实现
// ============================================================================

u16 BlockPropertyEncoder::encodeAxis(const String& axis) {
    if (axis == "y") return 0;
    if (axis == "x") return 1;
    if (axis == "z") return 2;
    return 3; // none
}

u16 BlockPropertyEncoder::encodeFacing(const String& facing) {
    if (facing == "north") return 0;
    if (facing == "south") return 1;
    if (facing == "west") return 2;
    if (facing == "east") return 3;
    if (facing == "up") return 4;
    if (facing == "down") return 5;
    return 0;
}

u16 BlockPropertyEncoder::encodeHalf(const String& half) {
    if (half == "top") return 1;
    return 0;
}

String BlockPropertyEncoder::decodeAxis(u16 data) {
    u16 value = getBits(data, 0, 0x3);
    switch (value) {
        case 0: return "y";
        case 1: return "x";
        case 2: return "z";
        case 3: return "none";
        default: return "";
    }
}

String BlockPropertyEncoder::decodeFacing(u16 data) {
    u16 value = getBits(data, 2, 0x7);
    switch (value) {
        case 0: return "north";
        case 1: return "south";
        case 2: return "west";
        case 3: return "east";
        case 4: return "up";
        case 5: return "down";
        default: return "";
    }
}

String BlockPropertyEncoder::decodeHalf(u16 data) {
    u16 value = getBits(data, 5, 0x1);
    return value == 1 ? "top" : "bottom";
}

u16 BlockPropertyEncoder::setBits(u16 data, u16 offset, u16 mask, u16 value) {
    return (data & ~(mask << offset)) | ((value & mask) << offset);
}

u16 BlockPropertyEncoder::getBits(u16 data, u16 offset, u16 mask) {
    return (data >> offset) & mask;
}

// ============================================================================
// BlockProps 工厂函数
// ============================================================================

namespace BlockProps {

BlockProperties log(u16 data) {
    BlockProperties props;
    String axis = BlockPropertyEncoder::decodeAxis(data);
    if (!axis.empty() && axis != "none") {
        props.set("axis", axis);
    }
    return props;
}

BlockProperties log(const String& axis) {
    BlockProperties props;
    props.set("axis", axis);
    return props;
}

BlockProperties stairs(const String& facing, const String& half) {
    BlockProperties props;
    props.set("facing", facing);
    props.set("half", half);
    return props;
}

BlockProperties door(const String& facing, const String& half, bool open, bool powered) {
    BlockProperties props;
    props.set("facing", facing);
    props.set("half", half);
    if (open) props.set("open", "true");
    if (powered) props.set("powered", "true");
    return props;
}

BlockProperties grassBlock(bool snowy) {
    BlockProperties props;
    if (snowy) props.set("snowy", "true");
    return props;
}

BlockProperties fluid(u16 level) {
    BlockProperties props;
    props.set("level", std::to_string(level));
    return props;
}

} // namespace BlockProps

} // namespace mr
