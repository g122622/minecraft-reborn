#pragma once

#include "../storage/BlockLightStorage.hpp"
#include "LevelBasedGraph.hpp"
#include "../../block/BlockPos.hpp"
#include "../../../util/Direction.hpp"

namespace mc {

// 前向声明
class IWorld;
class CollisionShape;

/**
 * @brief 方块光照引擎
 *
 * 实现方块光照的传播算法。
 * 方块光源（如火把、萤石）发出的光会向相邻方块传播，
 * 每传播一个方块衰减1级，直到衰减为0。
 *
 * 参考: net.minecraft.world.lighting.BlockLightEngine
 */
class BlockLightEngine : public LevelBasedGraph {
public:
    /**
     * @brief 构造函数
     * @param provider 区块光照提供者
     */
    explicit BlockLightEngine(IChunkLightProvider* provider);

    // ========================================================================
    // 光照操作
    // ========================================================================

    /**
     * @brief 检查指定位置的光照
     *
     * 调度该位置及其相邻位置的光照更新。
     *
     * @param pos 方块位置
     */
    void checkLight(const BlockPos& pos);

    /**
     * @brief 方块发光等级增加时调用
     *
     * 当方块被放置且发光等级大于0时调用。
     *
     * @param pos 方块位置
     * @param lightLevel 发光等级
     */
    void onBlockEmissionIncrease(const BlockPos& pos, i32 lightLevel);

    /**
     * @brief 获取指定位置的光照等级
     *
     * @param pos 方块位置
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] u8 getLightFor(const BlockPos& pos) const;

    /**
     * @brief 更新区块段状态
     *
     * @param pos 区块段位置
     * @param isEmpty 是否为空
     */
    void updateSectionStatus(const SectionPos& pos, bool isEmpty);

    /**
     * @brief 设置光照数据
     *
     * @param pos 区块段位置
     * @param array 光照数组
     * @param retain 是否保留
     */
    void setData(const SectionPos& pos, NibbleArray* array, bool retain);

    /**
     * @brief 获取光照数组
     *
     * @param pos 区块段位置
     * @return 光照数组指针
     */
    [[nodiscard]] NibbleArray* getData(const SectionPos& pos);

    /**
     * @brief 检查是否有待处理的工作
     */
    [[nodiscard]] bool hasWork() const;

    /**
     * @brief 处理光照更新
     *
     * @param maxUpdates 最大更新数量
     * @param updateSkyLight 是否更新天空光照（忽略，方块光照引擎不处理）
     * @param updateBlockLight 是否更新方块光照
     * @return 剩余配额
     */
    i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight);

protected:
    // ========================================================================
    // LevelBasedGraph 接口实现
    // ========================================================================

    [[nodiscard]] bool isRoot(i64 pos) const override;
    [[nodiscard]] i32 computeLevel(i64 pos, i64 excludedSource, i32 level) override;
    void notifyNeighbors(i64 pos, i32 level, bool isDecreasing) override;
    [[nodiscard]] i32 getLevel(i64 pos) const override;
    void setLevel(i64 pos, i32 level) override;
    [[nodiscard]] i32 getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) override;

private:
    IChunkLightProvider* m_chunkProvider;
    BlockLightStorage m_storage;
    BlockPos m_scratchPos;

    /**
     * @brief 获取指定位置的发光等级
     */
    [[nodiscard]] i32 getLightValue(i64 worldPos) const;

    /**
     * @brief 获取方块及其透明度
     *
     * @param pos 世界位置编码
     * @param opacityOut 透明度输出（可选）
     * @return 方块状态指针
     */
    [[nodiscard]] const BlockState* getBlockAndOpacity(i64 pos, i32* opacityOut) const;

    /**
     * @brief 获取方块的遮挡形状
     */
    [[nodiscard]] const CollisionShape& getVoxelShape(const BlockState& state, i64 pos, Direction dir) const;

    /**
     * @brief 检查两个方块之间是否有完整的遮挡面
     *
     * 参考: net.minecraft.world.lighting.LightEngine#func_215613_a
     */
    [[nodiscard]] static bool facesHaveOcclusion(
        IWorld* world,
        const BlockState& stateA, const BlockPos& posA,
        const BlockState& stateB, const BlockPos& posB,
        Direction dir, i32 opacity);

    /**
     * @brief 世界位置编码
     */
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z);
    [[nodiscard]] static i64 packPos(const BlockPos& pos);
    [[nodiscard]] static void unpackPos(i64 packed, i32& x, i32& y, i32& z);
    [[nodiscard]] static i64 offsetPos(i64 pos, Direction dir);
    [[nodiscard]] static i64 worldToSectionPos(i64 worldPos);
};

} // namespace mc
