#include "BlockCollision.hpp"
#include <mutex>

namespace mr {

BlockCollisionRegistry& BlockCollisionRegistry::instance() {
    static BlockCollisionRegistry instance;
    return instance;
}

BlockCollisionRegistry::BlockCollisionRegistry()
    : m_emptyShape(CollisionShape::empty())
    , m_fullBlockShape(CollisionShape::fullBlock())
{
    // 初始化所有形状为空
    m_shapes.fill(m_emptyShape);
}

void BlockCollisionRegistry::initialize() {
    if (m_initialized) return;

    registerVanillaBlocks();
    m_initialized = true;
}

void BlockCollisionRegistry::registerVanillaBlocks() {
    // ===== 空碰撞方块 =====
    registerEmpty(BlockId::Air);
    registerEmpty(BlockId::Water);
    registerEmpty(BlockId::Lava);
    // 草、花、农作物等以后添加

    // ===== 完整方块 =====
    registerFullBlock(BlockId::Stone);
    registerFullBlock(BlockId::GrassBlock);
    registerFullBlock(BlockId::Dirt);
    registerFullBlock(BlockId::Cobblestone);
    registerFullBlock(BlockId::OakPlanks);
    registerFullBlock(BlockId::Bedrock);
    registerFullBlock(BlockId::Sand);
    registerFullBlock(BlockId::Gravel);
    registerFullBlock(BlockId::GoldOre);
    registerFullBlock(BlockId::IronOre);
    registerFullBlock(BlockId::CoalOre);
    registerFullBlock(BlockId::DiamondOre);
    registerFullBlock(BlockId::DiamondBlock);
    registerFullBlock(BlockId::OakLog);
    // OakLeaves 通常是部分透明但仍有碰撞
    registerFullBlock(BlockId::OakLeaves);
    registerFullBlock(BlockId::Snow);
    registerFullBlock(BlockId::Ice);
    registerFullBlock(BlockId::Netherrack);
    registerFullBlock(BlockId::Glowstone);
    registerFullBlock(BlockId::EndStone);
    registerFullBlock(BlockId::Obsidian);

    // TODO: 后续添加部分碰撞方块（台阶、楼梯、门、栅栏等）
    // 这些需要使用形状工厂
}

void BlockCollisionRegistry::registerShape(BlockId blockId, const CollisionShape& shape) {
    size_t index = static_cast<size_t>(blockId);
    if (index < m_shapes.size()) {
        m_shapes[index] = shape;
    }
}

void BlockCollisionRegistry::registerFullBlock(BlockId blockId) {
    registerShape(blockId, m_fullBlockShape);
}

void BlockCollisionRegistry::registerEmpty(BlockId blockId) {
    registerShape(blockId, m_emptyShape);
}

void BlockCollisionRegistry::registerShapeFactory(BlockId blockId,
                                                    std::function<CollisionShape(BlockState)> factory) {
    m_shapeFactories[blockId] = std::move(factory);
}

CollisionShape BlockCollisionRegistry::getShape(BlockState state) const {
    // 首先检查是否有形状工厂
    auto it = m_shapeFactories.find(state.id());
    if (it != m_shapeFactories.end()) {
        return it->second(state);
    }

    // 使用预注册的形状
    return getShapeById(state.id());
}

CollisionShape BlockCollisionRegistry::getShapeById(BlockId blockId) const {
    size_t index = static_cast<size_t>(blockId);
    if (index < m_shapes.size()) {
        return m_shapes[index];
    }
    return m_emptyShape;
}

bool BlockCollisionRegistry::hasCollision(BlockState state) const {
    return !getShape(state).isEmpty();
}

} // namespace mr
