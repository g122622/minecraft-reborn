#include "PlayerAbilitiesPacket.hpp"
#include "PacketSerializer.hpp"
#include "../../entity/Player.hpp"

namespace mc::network {

// ============================================================================
// 常量定义
// ============================================================================

namespace {
    // MC 1.16.5 默认速度值
    constexpr f32 DEFAULT_FLY_SPEED = 0.05f;
    constexpr f32 DEFAULT_WALK_SPEED = 0.1f;
}

// ============================================================================
// 构造函数
// ============================================================================

PlayerAbilitiesPacket::PlayerAbilitiesPacket()
    : Packet(PacketType::PlayerAbilities)
    , m_flags(0)
    , m_flySpeed(DEFAULT_FLY_SPEED)
    , m_walkSpeed(DEFAULT_WALK_SPEED)
{
}

PlayerAbilitiesPacket::PlayerAbilitiesPacket(const PlayerAbilities& abilities)
    : Packet(PacketType::PlayerAbilities)
    , m_flags(0)
    , m_flySpeed(abilities.flySpeed)
    , m_walkSpeed(abilities.walkSpeed)
{
    setInvulnerable(abilities.invulnerable);
    setFlying(abilities.flying);
    setCanFly(abilities.canFly);
    setCreativeMode(abilities.creativeMode);
}

// ============================================================================
// 工厂方法
// ============================================================================

PlayerAbilitiesPacket PlayerAbilitiesPacket::fromPlayer(const Player& player) {
    // 从玩家获取能力（假设 Player 类有 abilities() 方法）
    // 如果没有，则从游戏模式推断
    PlayerAbilitiesPacket packet;

    GameMode mode = player.gameMode();

    // 根据游戏模式设置能力
    switch (mode) {
        case GameMode::Creative:
            packet.setInvulnerable(true);
            packet.setCanFly(true);
            packet.setCreativeMode(true);
            break;

        case GameMode::Spectator:
            packet.setInvulnerable(true);
            packet.setCanFly(true);
            packet.setFlying(true);
            packet.setCreativeMode(false);
            break;

        case GameMode::Survival:
        case GameMode::Adventure:
        default:
            packet.setInvulnerable(false);
            packet.setCanFly(false);
            packet.setFlying(false);
            packet.setCreativeMode(false);
            break;
    }

    return packet;
}

PlayerAbilitiesPacket PlayerAbilitiesPacket::fromGameMode(GameMode mode) {
    PlayerAbilitiesPacket packet;

    switch (mode) {
        case GameMode::Creative:
            packet.setInvulnerable(true);
            packet.setCanFly(true);
            packet.setCreativeMode(true);
            break;

        case GameMode::Spectator:
            packet.setInvulnerable(true);
            packet.setCanFly(true);
            packet.setFlying(true);
            packet.setCreativeMode(false);
            break;

        case GameMode::Survival:
        case GameMode::Adventure:
        default:
            packet.setInvulnerable(false);
            packet.setCanFly(false);
            packet.setFlying(false);
            packet.setCreativeMode(false);
            break;
    }

    return packet;
}

// ============================================================================
// 序列化
// ============================================================================

Result<std::vector<u8>> PlayerAbilitiesPacket::serialize() const {
    PacketSerializer serializer(expectedSize());

    serializer.writeU8(m_flags);
    serializer.writeF32(m_flySpeed);
    serializer.writeF32(m_walkSpeed);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> PlayerAbilitiesPacket::deserialize(const u8* data, size_t size) {
    if (size < expectedSize()) {
        return Error(ErrorCode::InvalidData, "PlayerAbilitiesPacket: insufficient data");
    }

    PacketDeserializer deserializer(data, size);

    auto flagsResult = deserializer.readU8();
    if (!flagsResult.success()) {
        return flagsResult.error();
    }
    m_flags = flagsResult.value();

    auto flySpeedResult = deserializer.readF32();
    if (!flySpeedResult.success()) {
        return flySpeedResult.error();
    }
    m_flySpeed = flySpeedResult.value();

    auto walkSpeedResult = deserializer.readF32();
    if (!walkSpeedResult.success()) {
        return walkSpeedResult.error();
    }
    m_walkSpeed = walkSpeedResult.value();

    return {};
}

size_t PlayerAbilitiesPacket::expectedSize() const {
    // 1 byte flags + 4 bytes flySpeed + 4 bytes walkSpeed
    return 9;
}

} // namespace mc::network
