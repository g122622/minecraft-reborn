#include "Block.hpp"
#include "BlockRegistry.hpp"
#include "Material.hpp"
#include <sstream>

namespace mr {

// ============================================================================
// VoxelShapes
// ============================================================================

namespace {
    CollisionShape g_emptyShape = CollisionShape::empty();
    CollisionShape g_fullBlockShape = CollisionShape::fullBlock();
}

const CollisionShape& VoxelShapes::empty() {
    return g_emptyShape;
}

const CollisionShape& VoxelShapes::fullCube() {
    return g_fullBlockShape;
}

CollisionShape VoxelShapes::cube(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2) {
    return CollisionShape::box(x1 / 16.0f, y1 / 16.0f, z1 / 16.0f,
                               x2 / 16.0f, y2 / 16.0f, z2 / 16.0f);
}

// ============================================================================
// BlockState
// ============================================================================

BlockState::BlockState(const Block& block,
                       std::unordered_map<const IProperty*, size_t> values,
                       u32 stateId)
    : StateHolder<Block, BlockState>(&block, std::move(values), stateId) {
    cacheProperties();
}

void BlockState::cacheProperties() {
    // 缓存方块属性
    m_isSolid = m_owner->isSolid(*this);
    m_isOpaque = m_owner->isOpaque(*this);
    m_blocksMovement = m_owner->material().blocksMovement();
    m_isLiquid = m_owner->material().isLiquid();
    m_isFlammable = m_owner->material().isFlammable();
    m_lightLevel = m_owner->lightLevel();
    m_hardness = m_owner->hardness();
    m_resistance = m_owner->resistance();
    m_blockId = m_owner->blockId();
}

bool BlockState::isAir() const {
    return m_owner->isAir(*this);
}

const CollisionShape& BlockState::getCollisionShape() const {
    return m_owner->getCollisionShape(*this);
}

const CollisionShape& BlockState::getShape() const {
    return m_owner->getShape(*this);
}

const CollisionShape& BlockState::getOcclusionShape() const {
    return m_owner->getOcclusionShape(*this);
}

const ResourceLocation& BlockState::blockLocation() const {
    return m_owner->blockLocation();
}

String BlockState::toModelKey() const {
    if (m_values.empty()) {
        return "";
    }

    std::ostringstream ss;
    bool first = true;
    for (const auto& [prop, valueIndex] : m_values) {
        if (!first) ss << ',';
        ss << prop->name() << '=' << prop->valueToString(valueIndex);
        first = false;
    }
    return ss.str();
}

String BlockState::ownerName() const {
    return m_owner->toString();
}

// ============================================================================
// BlockProperties
// ============================================================================

BlockProperties::BlockProperties(const Material& material)
    : m_material(material)
    , m_hardness(0.0f)
    , m_resistance(0.0f)
    , m_lightLevel(0)
    , m_hasCollision(material.blocksMovement())
    , m_isSolid(material.isSolid())
    , m_isFlammable(material.isFlammable())
    , m_requiresTool(false)
    , m_isReplaceable(material.isReplaceable()) {
}

BlockProperties& BlockProperties::hardness(f32 value) {
    m_hardness = value;
    return *this;
}

BlockProperties& BlockProperties::resistance(f32 value) {
    m_resistance = value;
    return *this;
}

BlockProperties& BlockProperties::lightLevel(u8 level) {
    m_lightLevel = level > 15 ? 15 : level;
    return *this;
}

BlockProperties& BlockProperties::noCollision() {
    m_hasCollision = false;
    return *this;
}

BlockProperties& BlockProperties::notSolid() {
    m_isSolid = false;
    return *this;
}

BlockProperties& BlockProperties::requiresTool() {
    m_requiresTool = true;
    return *this;
}

BlockProperties& BlockProperties::flammable(bool value) {
    m_isFlammable = value;
    return *this;
}

BlockProperties& BlockProperties::replaceable() {
    m_isReplaceable = true;
    return *this;
}

BlockProperties& BlockProperties::strength(f32 value) {
    m_hardness = value;
    m_resistance = value;
    return *this;
}

// ============================================================================
// Block
// ============================================================================

Block* Block::getBlock(u32 blockId) {
    return BlockRegistry::instance().getBlock(blockId);
}

Block* Block::getBlock(const ResourceLocation& id) {
    return BlockRegistry::instance().getBlock(id);
}

BlockState* Block::getBlockState(u32 stateId) {
    return BlockRegistry::instance().getBlockState(stateId);
}

void Block::forEachBlock(std::function<void(Block&)> callback) {
    BlockRegistry::instance().forEachBlock(std::move(callback));
}

void Block::forEachBlockState(std::function<void(const BlockState&)> callback) {
    BlockRegistry::instance().forEachBlockState(std::move(callback));
}

Block::Block(BlockProperties properties)
    : m_material(properties.material())
    , m_hardness(properties.hardness())
    , m_resistance(properties.resistance())
    , m_lightLevel(properties.lightLevel())
    , m_hasCollision(properties.hasCollision())
    , m_isFlammable(properties.isFlammable()) {
}

void Block::createBlockState(std::unique_ptr<StateContainer<Block, BlockState>> container) {
    m_stateContainer = std::move(container);
    m_defaultState = &m_stateContainer->baseState();
}

void Block::setDefaultState(const BlockState& state) {
    m_defaultState = &state;
}

const CollisionShape& Block::getShape(const BlockState& state) const {
    (void)state;
    return VoxelShapes::fullCube();
}

const CollisionShape& Block::getCollisionShape(const BlockState& state) const {
    if (!m_hasCollision) {
        return VoxelShapes::empty();
    }
    return getShape(state);
}

const CollisionShape& Block::getOcclusionShape(const BlockState& state) const {
    return getShape(state);
}

bool Block::isAir(const BlockState& state) const {
    (void)state;
    // 基类默认返回 false
    // AirBlock 会覆盖此方法返回 true
    return false;
}

bool Block::isSolid(const BlockState& state) const {
    (void)state;
    return m_material.isSolid();
}

bool Block::isOpaque(const BlockState& state) const {
    (void)state;
    return m_material.isOpaque();
}

} // namespace mr
