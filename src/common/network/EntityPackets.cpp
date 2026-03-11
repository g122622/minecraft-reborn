#include "EntityPackets.hpp"
#include "PacketSerializer.hpp"
#include <algorithm>

namespace mr::network {

// ==================== SpawnEntityPacket ====================

Result<std::vector<u8>> SpawnEntityPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeBytes(m_uuid.data(), 16);
    serializer.writeString(m_entityTypeId);
    serializer.writeF32(m_x);
    serializer.writeF32(m_y);
    serializer.writeF32(m_z);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    serializer.writeI16(m_velocityX);
    serializer.writeI16(m_velocityY);
    serializer.writeI16(m_velocityZ);
    return serializer.buffer();
}

Result<void> SpawnEntityPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto result = deserializer.readU32();
    if (!result.success()) {
        return Error(result.error());
    }
    m_entityId = result.value();

    auto uuidResult = deserializer.readBytes(16);
    if (!uuidResult.success()) {
        return Error(ErrorCode::InvalidPacket, "Failed to read UUID");
    }
    const auto& uuidBytes = uuidResult.value();
    std::copy(uuidBytes.begin(), uuidBytes.end(), m_uuid.begin());

    auto typeIdResult = deserializer.readString();
    if (!typeIdResult.success()) {
        return Error(typeIdResult.error());
    }
    m_entityTypeId = typeIdResult.value();

    auto xResult = deserializer.readF32();
    if (!xResult.success()) return Error(xResult.error());
    m_x = xResult.value();

    auto yResult = deserializer.readF32();
    if (!yResult.success()) return Error(yResult.error());
    m_y = yResult.value();

    auto zResult = deserializer.readF32();
    if (!zResult.success()) return Error(zResult.error());
    m_z = zResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    auto vxResult = deserializer.readI16();
    if (!vxResult.success()) return Error(vxResult.error());
    m_velocityX = vxResult.value();

    auto vyResult = deserializer.readI16();
    if (!vyResult.success()) return Error(vyResult.error());
    m_velocityY = vyResult.value();

    auto vzResult = deserializer.readI16();
    if (!vzResult.success()) return Error(vzResult.error());
    m_velocityZ = vzResult.value();

    return {};
}

// ==================== SpawnMobPacket ====================

Result<std::vector<u8>> SpawnMobPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeBytes(m_uuid.data(), 16);
    serializer.writeString(m_entityTypeId);
    serializer.writeF32(m_x);
    serializer.writeF32(m_y);
    serializer.writeF32(m_z);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    serializer.writeF32(m_headYaw);
    serializer.writeI16(m_velocityX);
    serializer.writeI16(m_velocityY);
    serializer.writeI16(m_velocityZ);
    serializer.writeU32(static_cast<u32>(m_metadata.size()));
    serializer.writeBytes(m_metadata.data(), m_metadata.size());
    return serializer.buffer();
}

Result<void> SpawnMobPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto result = deserializer.readU32();
    if (!result.success()) {
        return Error(result.error());
    }
    m_entityId = result.value();

    auto uuidResult = deserializer.readBytes(16);
    if (!uuidResult.success()) {
        return Error(ErrorCode::InvalidPacket, "Failed to read UUID");
    }
    const auto& uuidBytes = uuidResult.value();
    std::copy(uuidBytes.begin(), uuidBytes.end(), m_uuid.begin());

    auto typeIdResult = deserializer.readString();
    if (!typeIdResult.success()) {
        return Error(typeIdResult.error());
    }
    m_entityTypeId = typeIdResult.value();

    auto xResult = deserializer.readF32();
    if (!xResult.success()) return Error(xResult.error());
    m_x = xResult.value();

    auto yResult = deserializer.readF32();
    if (!yResult.success()) return Error(yResult.error());
    m_y = yResult.value();

    auto zResult = deserializer.readF32();
    if (!zResult.success()) return Error(zResult.error());
    m_z = zResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    auto headYawResult = deserializer.readF32();
    if (!headYawResult.success()) return Error(headYawResult.error());
    m_headYaw = headYawResult.value();

    auto vxResult = deserializer.readI16();
    if (!vxResult.success()) return Error(vxResult.error());
    m_velocityX = vxResult.value();

    auto vyResult = deserializer.readI16();
    if (!vyResult.success()) return Error(vyResult.error());
    m_velocityY = vyResult.value();

    auto vzResult = deserializer.readI16();
    if (!vzResult.success()) return Error(vzResult.error());
    m_velocityZ = vzResult.value();

    auto metaLenResult = deserializer.readU32();
    if (!metaLenResult.success()) return Error(metaLenResult.error());
    u32 metaLen = metaLenResult.value();

    auto metaDataResult = deserializer.readBytes(metaLen);
    if (!metaDataResult.success()) {
        return Error(ErrorCode::InvalidPacket, "Failed to read metadata");
    }
    m_metadata = std::move(metaDataResult.value());

    return {};
}

// ==================== EntityMetadataPacket ====================

Result<std::vector<u8>> EntityMetadataPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeU32(static_cast<u32>(m_metadata.size()));
    serializer.writeBytes(m_metadata.data(), m_metadata.size());
    return serializer.buffer();
}

Result<void> EntityMetadataPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto lenResult = deserializer.readU32();
    if (!lenResult.success()) return Error(lenResult.error());
    u32 len = lenResult.value();

    auto metaDataResult = deserializer.readBytes(len);
    if (!metaDataResult.success()) {
        return Error(ErrorCode::InvalidPacket, "Failed to read metadata");
    }
    m_metadata = std::move(metaDataResult.value());
    return {};
}

// ==================== EntityVelocityPacket ====================

Result<std::vector<u8>> EntityVelocityPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeI16(m_velocityX);
    serializer.writeI16(m_velocityY);
    serializer.writeI16(m_velocityZ);
    return serializer.buffer();
}

Result<void> EntityVelocityPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto vxResult = deserializer.readI16();
    if (!vxResult.success()) return Error(vxResult.error());
    m_velocityX = vxResult.value();

    auto vyResult = deserializer.readI16();
    if (!vyResult.success()) return Error(vyResult.error());
    m_velocityY = vyResult.value();

    auto vzResult = deserializer.readI16();
    if (!vzResult.success()) return Error(vzResult.error());
    m_velocityZ = vzResult.value();

    return {};
}

// ==================== EntityTeleportPacket ====================

Result<std::vector<u8>> EntityTeleportPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeF32(m_x);
    serializer.writeF32(m_y);
    serializer.writeF32(m_z);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    serializer.writeBool(m_onGround);
    return serializer.buffer();
}

Result<void> EntityTeleportPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto xResult = deserializer.readF32();
    if (!xResult.success()) return Error(xResult.error());
    m_x = xResult.value();

    auto yResult = deserializer.readF32();
    if (!yResult.success()) return Error(yResult.error());
    m_y = yResult.value();

    auto zResult = deserializer.readF32();
    if (!zResult.success()) return Error(zResult.error());
    m_z = zResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    auto groundResult = deserializer.readBool();
    if (!groundResult.success()) return Error(groundResult.error());
    m_onGround = groundResult.value();

    return {};
}

// ==================== EntityDestroyPacket ====================

Result<std::vector<u8>> EntityDestroyPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(static_cast<u32>(m_entityIds.size()));
    for (u32 id : m_entityIds) {
        serializer.writeU32(id);
    }
    return serializer.buffer();
}

Result<void> EntityDestroyPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto countResult = deserializer.readU32();
    if (!countResult.success()) return Error(countResult.error());
    u32 count = countResult.value();

    m_entityIds.clear();
    m_entityIds.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        auto idResult = deserializer.readU32();
        if (!idResult.success()) return Error(idResult.error());
        m_entityIds.push_back(idResult.value());
    }
    return {};
}

// ==================== EntityAnimationPacket ====================

Result<std::vector<u8>> EntityAnimationPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeU8(static_cast<u8>(m_animation));
    return serializer.buffer();
}

Result<void> EntityAnimationPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto animResult = deserializer.readU8();
    if (!animResult.success()) return Error(animResult.error());
    m_animation = static_cast<Animation>(animResult.value());

    return {};
}

// ==================== EntityMovePacket ====================

Result<std::vector<u8>> EntityMovePacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeI16(m_deltaX);
    serializer.writeI16(m_deltaY);
    serializer.writeI16(m_deltaZ);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    serializer.writeBool(m_onGround);
    return serializer.buffer();
}

Result<void> EntityMovePacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto dxResult = deserializer.readI16();
    if (!dxResult.success()) return Error(dxResult.error());
    m_deltaX = dxResult.value();

    auto dyResult = deserializer.readI16();
    if (!dyResult.success()) return Error(dyResult.error());
    m_deltaY = dyResult.value();

    auto dzResult = deserializer.readI16();
    if (!dzResult.success()) return Error(dzResult.error());
    m_deltaZ = dzResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    auto groundResult = deserializer.readBool();
    if (!groundResult.success()) return Error(groundResult.error());
    m_onGround = groundResult.value();

    return {};
}

// ==================== EntityHeadLookPacket ====================

Result<std::vector<u8>> EntityHeadLookPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeF32(m_headYaw);
    return serializer.buffer();
}

Result<void> EntityHeadLookPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_headYaw = yawResult.value();

    return {};
}

// ==================== EntityStatusPacket ====================

Result<std::vector<u8>> EntityStatusPacket::serialize() const {
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeU8(static_cast<u8>(m_status));
    return serializer.buffer();
}

Result<void> EntityStatusPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto statusResult = deserializer.readU8();
    if (!statusResult.success()) return Error(statusResult.error());
    m_status = static_cast<Status>(statusResult.value());

    return {};
}

} // namespace mr::network
