#pragma once

#include "../../../core/Types.hpp"
#include "../../../math/MathUtils.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/ChunkPos.hpp"
#include <memory>
#include <vector>

namespace mr {

// 前向声明
class BlockState;
class ChunkPrimer;
class Biome;
class WorldGenRegion;
class IChunkGenerator;

/**
 * @brief 方块匹配规则基类
 *
 * 用于矿石生成时判断目标方块是否可被替换。
 * 参考 MC RuleTest
 */
class RuleTest {
public:
    virtual ~RuleTest() = default;

    /**
     * @brief 测试方块状态是否匹配规则
     * @param state 方块状态
     * @param random 随机数生成器
     * @return 是否匹配
     */
    [[nodiscard]] virtual bool test(const BlockState& state, math::Random& random) const = 0;

    /**
     * @brief 获取规则类型名称
     */
    [[nodiscard]] virtual const char* name() const = 0;
};

/**
 * @brief 方块状态提供者
 *
 * 用于提供方块状态，可以是固定的或基于噪声的。
 */
class BlockStateProvider {
public:
    virtual ~BlockStateProvider() = default;

    /**
     * @brief 获取方块状态
     * @param random 随机数生成器
     * @param pos 位置
     * @return 方块状态
     */
    [[nodiscard]] virtual const BlockState* getState(math::Random& random, i32 x, i32 y, i32 z) const = 0;
};

/**
 * @brief 固定方块状态提供者
 */
class SimpleBlockStateProvider : public BlockStateProvider {
public:
    explicit SimpleBlockStateProvider(BlockId blockId);

    [[nodiscard]] const BlockState* getState(math::Random& random, i32 x, i32 y, i32 z) const override;

private:
    BlockId m_blockId;
};

/**
 * @brief 特征配置基类
 *
 * 所有特征配置的基类接口。
 */
struct IFeatureConfig {
    virtual ~IFeatureConfig() = default;
};

/**
 * @brief 矿石特征配置
 *
 * 参考 MC OreFeatureConfig，定义矿石生成的参数。
 */
struct OreFeatureConfig : public IFeatureConfig {
    /// 目标方块规则（哪些方块可被替换为矿石）
    std::unique_ptr<RuleTest> target;

    /// 矿石方块ID
    BlockId state;

    /// 矿脉大小（方块数量）
    i32 size;

    /**
     * @brief 构造矿石配置
     * @param targetRule 目标方块规则
     * @param oreBlock 矿石方块ID
     * @param veinSize 矿脉大小
     */
    OreFeatureConfig(std::unique_ptr<RuleTest> targetRule, BlockId oreBlock, i32 veinSize);

    /**
     * @brief 创建自然石头目标配置（用于主世界矿石）
     */
    static std::unique_ptr<RuleTest> naturalStone();
};

/**
 * @brief 矿石目标类型枚举
 *
 * 定义常见的矿石生成目标。
 */
enum class OreTargetType {
    NaturalStone,   ///< 石头、花岗岩、闪长岩、安山岩
    Netherrack,     ///< 下界岩
    Basalt          ///< 玄武岩
};

/**
 * @brief 创建目标规则
 * @param type 目标类型
 * @return 目标规则
 */
std::unique_ptr<RuleTest> createOreTarget(OreTargetType type);

} // namespace mr
