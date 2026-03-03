#pragma once

#include "../core/Types.hpp"
#include "../core/Result.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <type_traits>

namespace mr::network {

// 网络字节序转换
class NetworkEndian {
public:
    static u16 hostToNetwork16(u16 value);
    static u32 hostToNetwork32(u32 value);
    static u64 hostToNetwork64(u64 value);
    static u16 networkToHost16(u16 value);
    static u32 networkToHost32(u32 value);
    static u64 networkToHost64(u64 value);

    template<typename T>
    static T hostToNetwork(T value) {
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");
        if constexpr (sizeof(T) == 2) {
            return static_cast<T>(hostToNetwork16(static_cast<u16>(value)));
        } else if constexpr (sizeof(T) == 4) {
            return static_cast<T>(hostToNetwork32(static_cast<u32>(value)));
        } else if constexpr (sizeof(T) == 8) {
            return static_cast<T>(hostToNetwork64(static_cast<u64>(value)));
        } else {
            return value; // 1字节无需转换
        }
    }

    template<typename T>
    static T networkToHost(T value) {
        return hostToNetwork(value); // 转换是对称的
    }
};

// 数据包序列化器
class PacketSerializer {
public:
    PacketSerializer();
    explicit PacketSerializer(size_t initialCapacity);

    // 写入操作
    void writeU8(u8 value);
    void writeU16(u16 value);
    void writeU32(u32 value);
    void writeU64(u64 value);
    void writeI8(i8 value);
    void writeI16(i16 value);
    void writeI32(i32 value);
    void writeI64(i64 value);
    void writeF32(f32 value);
    void writeF64(f64 value);
    void writeBool(bool value);
    void writeString(const String& value);
    void writeStringView(StringView value);
    void writeBytes(const u8* data, size_t size);
    void writeBytes(const std::vector<u8>& data);

    // 读取操作
    [[nodiscard]] Result<u8> readU8();
    [[nodiscard]] Result<u16> readU16();
    [[nodiscard]] Result<u32> readU32();
    [[nodiscard]] Result<u64> readU64();
    [[nodiscard]] Result<i8> readI8();
    [[nodiscard]] Result<i16> readI16();
    [[nodiscard]] Result<i32> readI32();
    [[nodiscard]] Result<i64> readI64();
    [[nodiscard]] Result<f32> readF32();
    [[nodiscard]] Result<f64> readF64();
    [[nodiscard]] Result<bool> readBool();
    [[nodiscard]] Result<String> readString();
    [[nodiscard]] Result<std::vector<u8>> readBytes(size_t size);

    // 缓冲区管理
    const std::vector<u8>& buffer() const { return m_buffer; }
    std::vector<u8>& buffer() { return m_buffer; }
    const u8* data() const { return m_buffer.data(); }
    u8* data() { return m_buffer.data(); }
    size_t size() const { return m_buffer.size(); }
    size_t remaining() const { return m_buffer.size() - m_readPos; }
    void clear();
    void resetRead();
    void resize(size_t size);
    void reserve(size_t capacity);

private:
    std::vector<u8> m_buffer;
    size_t m_readPos = 0;
};

// 数据包反序列化器 (读取现有数据)
class PacketDeserializer {
public:
    explicit PacketDeserializer(const u8* data, size_t size);
    explicit PacketDeserializer(const std::vector<u8>& data);

    // 读取操作
    [[nodiscard]] Result<u8> readU8();
    [[nodiscard]] Result<u16> readU16();
    [[nodiscard]] Result<u32> readU32();
    [[nodiscard]] Result<u64> readU64();
    [[nodiscard]] Result<i8> readI8();
    [[nodiscard]] Result<i16> readI16();
    [[nodiscard]] Result<i32> readI32();
    [[nodiscard]] Result<i64> readI64();
    [[nodiscard]] Result<f32> readF32();
    [[nodiscard]] Result<f64> readF64();
    [[nodiscard]] Result<bool> readBool();
    [[nodiscard]] Result<String> readString();
    [[nodiscard]] Result<std::vector<u8>> readBytes(size_t size);

    // 状态查询
    size_t size() const { return m_size; }
    size_t remaining() const { return m_size - m_readPos; }
    bool hasRemaining(size_t bytes) const { return remaining() >= bytes; }
    void reset() { m_readPos = 0; }

private:
    const u8* m_data;
    size_t m_size;
    size_t m_readPos = 0;
};

} // namespace mr::network
