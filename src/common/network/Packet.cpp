#include "Packet.hpp"
#include "PacketSerializer.hpp"

namespace mr::network {

// ============================================================================
// Packet 基类实现
// ============================================================================

Packet::Packet(PacketType type)
    : m_type(type)
{
}

// ============================================================================
// HeartbeatPacket 实现
// ============================================================================

Result<std::vector<u8>> HeartbeatPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(static_cast<u32>(PACKET_HEADER_SIZE + sizeof(u64))); // size
    serializer.writeU16(static_cast<u16>(m_type)); // type
    serializer.writeU16(m_flags); // flags
    serializer.writeU16(0); // reserved
    serializer.writeU16(0); // padding (确保头部为12字节)
    serializer.writeU64(m_timestamp); // timestamp
    return serializer.buffer();
}

Result<void> HeartbeatPacket::deserialize(const u8* data, size_t size) {
    if (size < PACKET_HEADER_SIZE + sizeof(u64)) {
        return Error(ErrorCode::InvalidArgument, "Packet too small for heartbeat");
    }

    PacketDeserializer deserializer(data, size);

    // 读取头部
    (void)deserializer.readU32(); // size
    (void)deserializer.readU16(); // type (可以忽略，因为我们知道包类型)
    auto flagsResult = deserializer.readU16(); // flags
    if (flagsResult.success()) {
        m_flags = flagsResult.value();
    }
    (void)deserializer.readU16(); // reserved
    (void)deserializer.readU16(); // padding

    auto timestampResult = deserializer.readU64();
    if (timestampResult.failed()) {
        return timestampResult.error();
    }
    m_timestamp = timestampResult.value();

    return Result<void>::ok();
}

// ============================================================================
// DisconnectPacket 实现
// ============================================================================

Result<std::vector<u8>> DisconnectPacket::serialize() const {
    PacketSerializer serializer;

    // 先写入内容以计算大小
    size_t contentSize = 2 + m_reason.size(); // 字符串长度(2字节) + 内容
    serializer.writeU32(static_cast<u32>(PACKET_HEADER_SIZE + contentSize)); // size
    serializer.writeU16(static_cast<u16>(m_type)); // type
    serializer.writeU16(m_flags); // flags
    serializer.writeU16(0); // reserved
    serializer.writeU16(0); // padding

    // 写入断开原因
    serializer.writeString(m_reason);

    return serializer.buffer();
}

Result<void> DisconnectPacket::deserialize(const u8* data, size_t size) {
    if (size < PACKET_HEADER_SIZE) {
        return Error(ErrorCode::InvalidArgument, "Packet too small for disconnect");
    }

    PacketDeserializer deserializer(data, size);

    // 读取头部
    (void)deserializer.readU32(); // size
    (void)deserializer.readU16(); // type
    auto flagsResult = deserializer.readU16(); // flags
    if (flagsResult.success()) {
        m_flags = flagsResult.value();
    }
    (void)deserializer.readU16(); // reserved
    (void)deserializer.readU16(); // padding

    // 读取断开原因
    auto reasonResult = deserializer.readString();
    if (reasonResult.failed()) {
        return reasonResult.error();
    }
    m_reason = reasonResult.value();

    return Result<void>::ok();
}

} // namespace mr::network
