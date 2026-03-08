#include "Feature.hpp"
#include "../../block/BlockRegistry.hpp"

namespace mr {

// ============================================================================
// RuleTest 实现
// ============================================================================

/**
 * @brief 石头规则测试
 *
 * 匹配石头、花岗岩、闪长岩、安山岩。
 */
class StoneRuleTest : public RuleTest {
public:
    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "stone"; }
};

/**
 * @brief 方块ID规则测试
 */
class BlockIdRuleTest : public RuleTest {
public:
    explicit BlockIdRuleTest(BlockId blockId) : m_blockId(blockId) {}

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "block_id"; }

private:
    BlockId m_blockId;
};

// ============================================================================
// SimpleBlockStateProvider 实现
// ============================================================================

SimpleBlockStateProvider::SimpleBlockStateProvider(BlockId blockId)
    : m_blockId(blockId) {}

const BlockState* SimpleBlockStateProvider::getState(math::Random& random, i32 x, i32 y, i32 z) const {
    (void)random;
    (void)x; (void)y; (void)z;
    return BlockRegistry::instance().get(m_blockId);
}

// ============================================================================
// OreFeatureConfig 实现
// ============================================================================

OreFeatureConfig::OreFeatureConfig(std::unique_ptr<RuleTest> targetRule, BlockId oreBlock, i32 veinSize)
    : target(std::move(targetRule))
    , state(oreBlock)
    , size(veinSize) {}

std::unique_ptr<RuleTest> OreFeatureConfig::naturalStone() {
    return std::make_unique<StoneRuleTest>();
}

// ============================================================================
// createOreTarget 实现
// ============================================================================

std::unique_ptr<RuleTest> createOreTarget(OreTargetType type) {
    switch (type) {
        case OreTargetType::NaturalStone:
            return std::make_unique<StoneRuleTest>();
        case OreTargetType::Netherrack:
            return std::make_unique<BlockIdRuleTest>(BlockId::Netherrack);
        case OreTargetType::Basalt:
            return std::make_unique<BlockIdRuleTest>(BlockId::Basalt);
        default:
            return std::make_unique<StoneRuleTest>();
    }
}

// ============================================================================
// StoneRuleTest 实现
// ============================================================================

bool StoneRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)random;
    u32 blockId = state.blockId();
    // 匹配石头、花岗岩、闪长岩、安山岩
    return blockId == static_cast<u32>(BlockId::Stone) ||
           blockId == static_cast<u32>(BlockId::Granite) ||
           blockId == static_cast<u32>(BlockId::Diorite) ||
           blockId == static_cast<u32>(BlockId::Andesite);
}

// ============================================================================
// BlockIdRuleTest 实现
// ============================================================================

bool BlockIdRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)random;
    return state.blockId() == static_cast<u32>(m_blockId);
}

} // namespace mr
