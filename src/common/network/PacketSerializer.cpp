#include "PacketSerializer.hpp"

#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <arpa/inet.h>
#endif

namespace mr::network {

// ============================================================================
// NetworkEndian 实现
// ============================================================================

u16 NetworkEndian::hostToNetwork16(u16 value) {
    return static_cast<u16>(htons(value));
}

u32 NetworkEndian::hostToNetwork32(u32 value) {
    return static_cast<u32>(htonl(value));
}

u64 NetworkEndian::hostToNetwork64(u64 value) {
    // 假设小端序系统 (x86/x64)
    // 将小端序转换为大端序 (网络字节序)
    return ((value & 0xFF00000000000000ULL) >> 56) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x000000FF00000000ULL) >> 8) |
           ((value & 0x00000000FF000000ULL) << 8) |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x00000000000000FFULL) << 56);
}

u16 NetworkEndian::networkToHost16(u16 value) {
    return static_cast<u16>(ntohs(value));
}

u32 NetworkEndian::networkToHost32(u32 value) {
    return static_cast<u32>(ntohl(value));
}

u64 NetworkEndian::networkToHost64(u64 value) {
    // 与hostToNetwork64相同 (转换是对称的)
    return hostToNetwork64(value);
}

// ============================================================================
// PacketSerializer 实现
// ============================================================================

PacketSerializer::PacketSerializer()
    : m_buffer()
{
    m_buffer.reserve(256);
}

PacketSerializer::PacketSerializer(size_t initialCapacity)
    : m_buffer()
{
    m_buffer.reserve(initialCapacity);
}

void PacketSerializer::writeU8(u8 value) {
    m_buffer.push_back(value);
}

void PacketSerializer::writeU16(u16 value) {
    u16 netValue = NetworkEndian::hostToNetwork16(value);
    const u8* bytes = reinterpret_cast<const u8*>(&netValue);
    m_buffer.insert(m_buffer.end(), bytes, bytes + 2);
}

void PacketSerializer::writeU32(u32 value) {
    u32 netValue = NetworkEndian::hostToNetwork32(value);
    const u8* bytes = reinterpret_cast<const u8*>(&netValue);
    m_buffer.insert(m_buffer.end(), bytes, bytes + 4);
}

void PacketSerializer::writeU64(u64 value) {
    u64 netValue = NetworkEndian::hostToNetwork64(value);
    const u8* bytes = reinterpret_cast<const u8*>(&netValue);
    m_buffer.insert(m_buffer.end(), bytes, bytes + 8);
}

void PacketSerializer::writeI8(i8 value) {
    writeU8(static_cast<u8>(value));
}

void PacketSerializer::writeI16(i16 value) {
    writeU16(static_cast<u16>(value));
}

void PacketSerializer::writeI32(i32 value) {
    writeU32(static_cast<u32>(value));
}

void PacketSerializer::writeI64(i64 value) {
    writeU64(static_cast<u64>(value));
}

void PacketSerializer::writeF32(f32 value) {
    u32 intValue;
    std::memcpy(&intValue, &value, sizeof(f32));
    writeU32(intValue);
}

void PacketSerializer::writeF64(f64 value) {
    u64 intValue;
    std::memcpy(&intValue, &value, sizeof(f64));
    writeU64(intValue);
}

void PacketSerializer::writeBool(bool value) {
    writeU8(value ? 1 : 0);
}

void PacketSerializer::writeString(const String& value) {
    // 字符串格式: 长度(u16) + 数据
    if (value.size() > 65535) {
        // 截断过长的字符串
        writeU16(65535);
        writeBytes(reinterpret_cast<const u8*>(value.data()), 65535);
    } else {
        writeU16(static_cast<u16>(value.size()));
        writeBytes(reinterpret_cast<const u8*>(value.data()), value.size());
    }
}

void PacketSerializer::writeStringView(StringView value) {
    writeString(String(value));
}

void PacketSerializer::writeBytes(const u8* data, size_t size) {
    m_buffer.insert(m_buffer.end(), data, data + size);
}

void PacketSerializer::writeBytes(const std::vector<u8>& data) {
    m_buffer.insert(m_buffer.end(), data.begin(), data.end());
}

Result<u8> PacketSerializer::readU8() {
    if (m_readPos + 1 > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u8");
    }
    return m_buffer[m_readPos++];
}

Result<u16> PacketSerializer::readU16() {
    if (m_readPos + 2 > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u16");
    }
    u16 netValue;
    std::memcpy(&netValue, m_buffer.data() + m_readPos, 2);
    m_readPos += 2;
    return NetworkEndian::networkToHost16(netValue);
}

Result<u32> PacketSerializer::readU32() {
    if (m_readPos + 4 > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u32");
    }
    u32 netValue;
    std::memcpy(&netValue, m_buffer.data() + m_readPos, 4);
    m_readPos += 4;
    return NetworkEndian::networkToHost32(netValue);
}

Result<u64> PacketSerializer::readU64() {
    if (m_readPos + 8 > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u64");
    }
    u64 netValue;
    std::memcpy(&netValue, m_buffer.data() + m_readPos, 8);
    m_readPos += 8;
    return NetworkEndian::networkToHost64(netValue);
}

Result<i8> PacketSerializer::readI8() {
    auto result = readU8();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i8>(result.value());
}

Result<i16> PacketSerializer::readI16() {
    auto result = readU16();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i16>(result.value());
}

Result<i32> PacketSerializer::readI32() {
    auto result = readU32();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i32>(result.value());
}

Result<i64> PacketSerializer::readI64() {
    auto result = readU64();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i64>(result.value());
}

Result<f32> PacketSerializer::readF32() {
    auto result = readU32();
    if (result.failed()) {
        return result.error();
    }
    u32 intValue = result.value();
    f32 value;
    std::memcpy(&value, &intValue, sizeof(f32));
    return value;
}

Result<f64> PacketSerializer::readF64() {
    auto result = readU64();
    if (result.failed()) {
        return result.error();
    }
    u64 intValue = result.value();
    f64 value;
    std::memcpy(&value, &intValue, sizeof(f64));
    return value;
}

Result<bool> PacketSerializer::readBool() {
    auto result = readU8();
    if (result.failed()) {
        return result.error();
    }
    return result.value() != 0;
}

Result<String> PacketSerializer::readString() {
    auto lengthResult = readU16();
    if (lengthResult.failed()) {
        return lengthResult.error();
    }
    u16 length = lengthResult.value();

    if (m_readPos + length > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read string");
    }

    String str(reinterpret_cast<const char*>(m_buffer.data() + m_readPos), length);
    m_readPos += length;
    return str;
}

Result<std::vector<u8>> PacketSerializer::readBytes(size_t size) {
    if (m_readPos + size > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read bytes");
    }
    std::vector<u8> data(m_buffer.begin() + m_readPos, m_buffer.begin() + m_readPos + size);
    m_readPos += size;
    return data;
}

void PacketSerializer::clear() {
    m_buffer.clear();
    m_readPos = 0;
}

void PacketSerializer::resetRead() {
    m_readPos = 0;
}

void PacketSerializer::resize(size_t size) {
    m_buffer.resize(size);
}

void PacketSerializer::reserve(size_t capacity) {
    m_buffer.reserve(capacity);
}

// ============================================================================
// PacketDeserializer 实现
// ============================================================================

PacketDeserializer::PacketDeserializer(const u8* data, size_t size)
    : m_data(data)
    , m_size(size)
    , m_readPos(0)
{
}

PacketDeserializer::PacketDeserializer(const std::vector<u8>& data)
    : m_data(data.data())
    , m_size(data.size())
    , m_readPos(0)
{
}

Result<u8> PacketDeserializer::readU8() {
    if (m_readPos + 1 > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u8");
    }
    return m_data[m_readPos++];
}

Result<u16> PacketDeserializer::readU16() {
    if (m_readPos + 2 > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u16");
    }
    u16 netValue;
    std::memcpy(&netValue, m_data + m_readPos, 2);
    m_readPos += 2;
    return NetworkEndian::networkToHost16(netValue);
}

Result<u32> PacketDeserializer::readU32() {
    if (m_readPos + 4 > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u32");
    }
    u32 netValue;
    std::memcpy(&netValue, m_data + m_readPos, 4);
    m_readPos += 4;
    return NetworkEndian::networkToHost32(netValue);
}

Result<u64> PacketDeserializer::readU64() {
    if (m_readPos + 8 > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u64");
    }
    u64 netValue;
    std::memcpy(&netValue, m_data + m_readPos, 8);
    m_readPos += 8;
    return NetworkEndian::networkToHost64(netValue);
}

Result<i8> PacketDeserializer::readI8() {
    auto result = readU8();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i8>(result.value());
}

Result<i16> PacketDeserializer::readI16() {
    auto result = readU16();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i16>(result.value());
}

Result<i32> PacketDeserializer::readI32() {
    auto result = readU32();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i32>(result.value());
}

Result<i64> PacketDeserializer::readI64() {
    auto result = readU64();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i64>(result.value());
}

Result<f32> PacketDeserializer::readF32() {
    auto result = readU32();
    if (result.failed()) {
        return result.error();
    }
    u32 intValue = result.value();
    f32 value;
    std::memcpy(&value, &intValue, sizeof(f32));
    return value;
}

Result<f64> PacketDeserializer::readF64() {
    auto result = readU64();
    if (result.failed()) {
        return result.error();
    }
    u64 intValue = result.value();
    f64 value;
    std::memcpy(&value, &intValue, sizeof(f64));
    return value;
}

Result<bool> PacketDeserializer::readBool() {
    auto result = readU8();
    if (result.failed()) {
        return result.error();
    }
    return result.value() != 0;
}

Result<String> PacketDeserializer::readString() {
    auto lengthResult = readU16();
    if (lengthResult.failed()) {
        return lengthResult.error();
    }
    u16 length = lengthResult.value();

    if (m_readPos + length > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read string");
    }

    String str(reinterpret_cast<const char*>(m_data + m_readPos), length);
    m_readPos += length;
    return str;
}

Result<std::vector<u8>> PacketDeserializer::readBytes(size_t size) {
    if (m_readPos + size > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read bytes");
    }
    std::vector<u8> data(m_data + m_readPos, m_data + m_readPos + size);
    m_readPos += size;
    return data;
}

} // namespace mr::network
