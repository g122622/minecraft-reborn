#include "FlowingFluid.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"
#include "../block/BlockPos.hpp"
#include "../block/Material.hpp"
#include "../../math/Vector3.hpp"
#include "../../util/Direction.hpp"

namespace mc {
namespace fluid {

// ============================================================================
// FlowingFluid 实现
// ============================================================================

FluidState FlowingFluid::getFlowingState(i32 level, bool falling) {
    // level范围是1-8，但源头是level=8
    level = std::clamp(level, 1, 8);
    const FluidState& state = getFlowing().defaultState();
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto& fallingProp = FluidProperties::FALLING();
    return state.with(levelProp, level).with(fallingProp, falling);
}

FluidState FlowingFluid::getStillState(bool falling) {
    // 源头level=8，但isSource=true
    const FluidState& state = getStill().defaultState();
    auto& fallingProp = FluidProperties::FALLING();
    if (falling) {
        return state.with(fallingProp, true);
    }
    return state;
}

void FlowingFluid::tick(IWorld& world, const BlockPos& pos, FluidState& state) {
    // 非源头才需要计算正确状态
    if (!isSource(state)) {
        const BlockState* currentBlock = world.getBlockState(pos.x, pos.y, pos.z);
        FluidState correctState = calculateCorrectFlowingState(world, pos, currentBlock);

        if (correctState.isEmpty()) {
            // 应该消失
            // TODO: 设置为空气方块
            return;
        } else if (correctState != state) {
            // 状态改变
            state = correctState;
            // TODO: 更新世界中的方块

            // 通知邻居更新
            // world.notifyNeighborsOfStateChange(pos, getBlockState(state)->block());
        }
    }

    // 执行流动
    flowAround(world, pos, state);

    // 调度下一次tick
    // 通过IWorld的scheduleFluidTick方法调度
    world.scheduleFluidTick(pos, *this, getTickDelay());
}

Vector3 FlowingFluid::getFlow(IBlockReader& world, const BlockPos& pos,
                               const FluidState& state) const {
    f32 dx = 0.0f;
    f32 dz = 0.0f;

    // 检查四个水平方向
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        // TODO: 从world获取流体状态
        // FluidState neighborFluid = world.getFluidState(neighborPos);

        // if (isSameOrEmpty(neighborFluid)) {
        //     f32 neighborHeight = neighborFluid.getHeight();
        //     f32 f = 0.0f;
        //
        //     if (neighborHeight == 0.0f) {
        //         // 相邻位置没有流体，检查下方
        //         // TODO: 实现下方检查
        //     } else {
        //         f = state.getHeight() - neighborHeight;
        //     }
        //
        //     if (f != 0.0f) {
        //         dx += static_cast<f32>(dir.getXOffset()) * f;
        //         dz += static_cast<f32>(dir.getZOffset()) * f;
        //     }
        // }
    }

    // 下落流体有额外的向下力
    if (state.isFalling()) {
        // TODO: 添加下落流体的额外计算
    }

    // 归一化
    f32 len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0f) {
        dx /= len;
        dz /= len;
    }

    return Vector3(dx, 0.0f, dz);
}

CollisionShape FlowingFluid::getShape(const FluidState& state, IBlockReader& world,
                                       const BlockPos& pos) const {
    // TODO: 实现实际的碰撞形状
    // 形状高度取决于流体高度
    return CollisionShape::empty();
}

void FlowingFluid::flowAround(IWorld& world, const BlockPos& pos, const FluidState& state) {
    if (state.isEmpty()) {
        return;
    }

    const BlockState* currentBlock = world.getBlockState(pos.x, pos.y, pos.z);

    // 1. 优先向下流动
    BlockPos below = pos.down();
    const BlockState* belowBlock = world.getBlockState(below.x, below.y, below.z);
    // TODO: 获取下方流体状态

    // 检查是否可以向下流动
    // if (canFlow(world, pos, currentBlock, Direction::Down, below, belowBlock, belowFluid, *this)) {
    //     flowInto(world, below, belowBlock, Direction::Down, getFlowingState(8, true));
    //     return;  // 向下流动后不再水平流动
    // }

    // 2. 计算水平流动
    if (!isSource(state) && state.getLevel() >= getSpreadDistance(world)) {
        return;  // 已到达最大流动距离
    }

    // 3. 向各方向流动
    i32 newLevel = isSource(state) ? getSpreadDistance(world) : state.getLevel() + getLevelDecrease(world);

    if (newLevel > 0 && newLevel <= 8) {
        auto flowDirections = getFlowDirections(world, pos, currentBlock);

        for (const auto& [dir, fluidState] : flowDirections) {
            BlockPos targetPos = pos.offset(Directions::toBlockFace(dir));
            const BlockState* targetBlock = world.getBlockState(targetPos.x, targetPos.y, targetPos.z);

            // if (canFlow(world, pos, currentBlock, dir, targetPos, targetBlock, targetFluid, *this)) {
            //     flowInto(world, targetPos, targetBlock, dir, fluidState);
            // }
        }
    }
}

void FlowingFluid::flowInto(IWorld& world, const BlockPos& pos, const BlockState* blockState,
                             Direction dir, const FluidState& state) {
    if (blockState != nullptr && !blockState->isAir()) {
        beforeReplacingBlock(world, pos, blockState);
    }

    // 设置流体方块
    // world.setBlock(pos.x, pos.y, pos.z, state.getBlockState());

    // 调度下一次tick
    world.scheduleFluidTick(pos, *this, getTickDelay());
}

FluidState FlowingFluid::calculateCorrectFlowingState(IWorld& world,
                                                       const BlockPos& pos,
                                                       const BlockState* blockState) {
    i32 maxLevel = 0;
    i32 sourceCount = 0;

    // 检查四个水平方向
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        const BlockState* neighborBlock = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        // TODO: 获取流体状态

        // 检查是否为同种流体
        // if (neighborFluid.getFluid().isEquivalentTo(*this) &&
        //     doesSideHaveHoles(dir, world, pos, blockState, neighborPos, neighborBlock)) {
        //     if (neighborFluid.isSource()) {
        //         sourceCount++;
        //     }
        //     maxLevel = std::max(maxLevel, neighborFluid.getLevel());
        // }
    }

    // 检查是否可以形成源头
    if (sourceCount >= 2 && canSourcesMultiply()) {
        BlockPos below = pos.down();
        const BlockState* belowBlock = world.getBlockState(below.x, below.y, below.z);
        // TODO: 检查下方是否为固体或同种流体

        // if (belowBlock->isSolid() || isSameSource(belowFluid)) {
        //     return getStillState(false);
        // }
    }

    // 检查上方是否有下落流体
    BlockPos above = pos.up();
    const BlockState* aboveBlock = world.getBlockState(above.x, above.y, above.z);
    // TODO: 获取上方流体状态

    // if (!aboveFluid.isEmpty() && aboveFluid.getFluid().isEquivalentTo(*this) &&
    //     doesSideHaveHoles(Direction::Up, world, pos, blockState, above, aboveBlock)) {
    //     return getFlowingState(8, true);  // 下落流体
    // }

    // 计算新等级
    i32 newLevel = maxLevel - getLevelDecrease(world);
    if (newLevel <= 0) {
        // 返回空流体
        return Fluid::getFluid(0)->defaultState();  // EmptyFluid
    }

    return getFlowingState(newLevel, false);
}

bool FlowingFluid::canFlow(IBlockReader& world, const BlockPos& fromPos,
                            const BlockState* fromBlock, Direction dir,
                            const BlockPos& toPos, const BlockState* toBlock,
                            const FluidState& toFluid, const Fluid& fluid) const {
    // 检查目标流体是否可以被替换
    // if (!toFluid.canDisplace(world, toPos, fluid, dir)) {
    //     return false;
    // }

    // 检查侧面是否有孔洞
    if (!doesSideHaveHoles(dir, world, fromPos, fromBlock, toPos, toBlock)) {
        return false;
    }

    // 检查目标方块是否可被流体替换
    return isBlocked(world, toPos, toBlock, fluid);
}

bool FlowingFluid::doesSideHaveHoles(Direction dir, IBlockReader& world,
                                      const BlockPos& pos, const BlockState* state,
                                      const BlockPos& neighborPos,
                                      const BlockState* neighborState) const {
    // TODO: 实现基于VoxelShape的孔洞检测
    // 简化实现：检查方块材质是否阻挡
    if (state == nullptr || neighborState == nullptr) {
        return true;
    }

    const Block& fromBlock = state->owner();
    const Block& toBlock = neighborState->owner();

    const Material& fromMaterial = fromBlock.material();
    const Material& toMaterial = toBlock.material();

    // 流体可以穿过非固体方块
    return !fromMaterial.blocksMovement() || !toMaterial.blocksMovement();
}

bool FlowingFluid::canFormSource(IWorld& world, const BlockPos& pos) {
    if (!canSourcesMultiply()) {
        return false;
    }

    i32 sourceCount = getHorizontalSourceCount(world, pos);
    if (sourceCount < 2) {
        return false;
    }

    // 检查下方
    BlockPos below = pos.down();
    const BlockState* belowBlock = world.getBlockState(below.x, below.y, below.z);
    if (belowBlock == nullptr) {
        return false;
    }

    // 下方必须是固体或同种流体源头
    if (belowBlock->isSolid()) {
        return true;
    }

    // TODO: 检查下方是否为同种流体源头
    return false;
}

i32 FlowingFluid::getHorizontalSourceCount(IWorld& world, const BlockPos& pos) const {
    i32 count = 0;

    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        // TODO: 获取流体状态并检查是否为源头
        // const FluidState& neighborFluid = world.getFluidState(neighborPos);
        // if (isSameSource(neighborFluid)) {
        //     count++;
        // }
    }

    return count;
}

bool FlowingFluid::isBlocked(IBlockReader& world, const BlockPos& pos,
                              const BlockState* block, const Fluid& fluid) const {
    if (block == nullptr || block->isAir()) {
        return true;
    }

    const Block& blockRef = block->owner();
    const Material& material = blockRef.material();

    // 传送门、结构空位、海草等特殊材质
    // if (material == Material::PORTAL || material == Material::STRUCTURE_VOID ||
    //     material == Material::OCEAN_PLANT || material == Material::SEA_GRASS) {
    //     return false;
    // }

    // 门、告示牌、梯子、甘蔗、气泡柱
    // TODO: 检查特定方块类型

    // 默认：非固体方块可以被流体替换
    return !material.blocksMovement();
}

bool FlowingFluid::isSameOrEmpty(const FluidState& state) const {
    return state.isEmpty() || isEquivalentTo(state.getFluid());
}

bool FlowingFluid::isSameSource(const FluidState& state) const {
    return isEquivalentTo(state.getFluid()) && state.isSource();
}

std::unordered_map<Direction, FluidState> FlowingFluid::getFlowDirections(
    IWorld& world, const BlockPos& pos, const BlockState* blockState) {
    std::unordered_map<Direction, FluidState> result;

    // TODO: 实现完整的流向计算
    // 1. 检查每个水平方向
    // 2. 计算目标位置的流体状态
    // 3. 按优先级排序（先流向低处）

    return result;
}

} // namespace fluid
} // namespace mc
