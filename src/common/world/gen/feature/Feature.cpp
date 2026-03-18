#include "Feature.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"

namespace mc {

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
 * @brief 方块状态规则测试
 *
 * 匹配特定方块状态。
 */
class BlockStateRuleTest : public RuleTest {
public:
    explicit BlockStateRuleTest(const BlockState* state) : m_state(state) {}

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "block_state"; }

private:
    const BlockState* m_state;
};

// ============================================================================
// SimpleBlockStateProvider 实现
// ============================================================================

SimpleBlockStateProvider::SimpleBlockStateProvider(const BlockState* state)
    : m_state(state) {}

const BlockState* SimpleBlockStateProvider::getState(math::Random& random, i32 x, i32 y, i32 z) const {
    (void)random;
    (void)x; (void)y; (void)z;
    return m_state;
}

// ============================================================================
// OreFeatureConfig 实现
// ============================================================================

OreFeatureConfig::OreFeatureConfig(std::unique_ptr<RuleTest> targetRule, const BlockState* oreState, i32 veinSize)
    : target(std::move(targetRule))
    , state(oreState)
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
            if (VanillaBlocks::NETHERRACK) {
                return std::make_unique<BlockStateRuleTest>(&VanillaBlocks::NETHERRACK->defaultState());
            }
            return std::make_unique<StoneRuleTest>();
        case OreTargetType::Basalt:
            if (VanillaBlocks::BASALT) {
                return std::make_unique<BlockStateRuleTest>(&VanillaBlocks::BASALT->defaultState());
            }
            return std::make_unique<StoneRuleTest>();
        default:
            return std::make_unique<StoneRuleTest>();
    }
}

// ============================================================================
// StoneRuleTest 实现
// ============================================================================

bool StoneRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)random;
    // 匹配石头、花岗岩、闪长岩、安山岩
    return state.is(VanillaBlocks::STONE) ||
           state.is(VanillaBlocks::GRANITE) ||
           state.is(VanillaBlocks::DIORITE) ||
           state.is(VanillaBlocks::ANDESITE);
}

// ============================================================================
// BlockStateRuleTest 实现
// ============================================================================

bool BlockStateRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)random;
    if (!m_state) return false;
    return state.blockId() == m_state->blockId();
}

} // namespace mc
