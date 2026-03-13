#include "Raycast.hpp"
#include "../../world/block/Block.hpp"
#include <cmath>
#include <algorithm>

namespace mc {

namespace {

/**
 * @brief 计算浮点数的小数部分
 */
inline f32 fract(f32 x)
{
    return x - std::floor(x);
}

/**
 * @brief 获取符号
 */
inline i32 signum(f32 x)
{
    if (x > 0.0f) return 1;
    if (x < 0.0f) return -1;
    return 0;
}

} // anonymous namespace

BlockRaycastResult raycastBlocks(
    const RaycastContext& context,
    const IBlockReader& world)
{
    const Vector3& start = context.ray.origin;
    const Vector3& dir = context.ray.direction;

    if (dir.lengthSquared() < 0.0001f) {
        // 方向为零向量，返回miss
        return BlockRaycastResult::miss();
    }

    // 计算终点
    const Vector3 end = context.endPosition();

    // 使用MC风格的端点偏移，避免在边界处产生精度问题
    // MathHelper.lerp(-1.0E-7D, end, start)
    const f32 eps = 1.0e-7f;
    const Vector3 adjustedEnd(
        end.x + (start.x - end.x) * eps,
        end.y + (start.y - end.y) * eps,
        end.z + (start.z - end.z) * eps
    );

    // 计算步进方向和参数
    const f32 dx = adjustedEnd.x - start.x;
    const f32 dy = adjustedEnd.y - start.y;
    const f32 dz = adjustedEnd.z - start.z;

    // 当前方块坐标
    i32 currentX = static_cast<i32>(std::floor(adjustedEnd.x));
    i32 currentY = static_cast<i32>(std::floor(adjustedEnd.y));
    i32 currentZ = static_cast<i32>(std::floor(adjustedEnd.z));

    // 检查起始位置的方块
    if (world.isWithinWorldBounds(currentX, currentY, currentZ)) {
        const BlockState* state = world.getBlockState(currentX, currentY, currentZ);
        if (state != nullptr && !state->isAir() && state->blocksMovement()) {
            // 起始位置有方块，计算击中点
            // 由于起点在方块内部，我们返回起点位置
            Vector3 hitPos = start;
            // 计算击中的面（从方向推断）
            Direction face = Directions::fromVector(-dir.x, -dir.y, -dir.z);
            return BlockRaycastResult::hit(hitPos, BlockPos(currentX, currentY, currentZ), face, 0.0f);
        }
    }

    // 计算步进方向
    const i32 stepX = signum(dx);
    const i32 stepY = signum(dy);
    const i32 stepZ = signum(dz);

    // 计算每单位距离穿过方块边界的次数
    // tDelta = 1 / |direction component|
    const f32 tDeltaX = (stepX == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepX) / dx;
    const f32 tDeltaY = (stepY == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepY) / dy;
    const f32 tDeltaZ = (stepZ == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepZ) / dz;

    // 计算到下一个边界的初始t值
    // tMax = distance to next boundary / total distance
    // 使用 adjustedEnd 作为起始点计算
    f32 tMaxX = (stepX == 0) ? std::numeric_limits<f32>::max()
        : tDeltaX * (stepX > 0 ? (1.0f - fract(adjustedEnd.x)) : fract(adjustedEnd.x));
    f32 tMaxY = (stepY == 0) ? std::numeric_limits<f32>::max()
        : tDeltaY * (stepY > 0 ? (1.0f - fract(adjustedEnd.y)) : fract(adjustedEnd.y));
    f32 tMaxZ = (stepZ == 0) ? std::numeric_limits<f32>::max()
        : tDeltaZ * (stepZ > 0 ? (1.0f - fract(adjustedEnd.z)) : fract(adjustedEnd.z));

    // 记录上次步进的面
    Direction lastFace = Direction::None;

    // DDA步进
    // MC使用 1.0 作为循环条件（代表遍历完整的射线）
    while (tMaxX <= 1.0f || tMaxY <= 1.0f || tMaxZ <= 1.0f) {
        // 选择最小的t值前进
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                // X方向步进
                currentX += stepX;
                tMaxX += tDeltaX;
                lastFace = (stepX > 0) ? Direction::West : Direction::East;
            } else {
                // Z方向步进
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
                lastFace = (stepZ > 0) ? Direction::North : Direction::South;
            }
        } else {
            if (tMaxY < tMaxZ) {
                // Y方向步进
                currentY += stepY;
                tMaxY += tDeltaY;
                lastFace = (stepY > 0) ? Direction::Down : Direction::Up;
            } else {
                // Z方向步进
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
                lastFace = (stepZ > 0) ? Direction::North : Direction::South;
            }
        }

        // 检查世界边界
        if (!world.isWithinWorldBounds(currentX, currentY, currentZ)) {
            // 超出世界边界，返回miss
            return BlockRaycastResult::miss();
        }

        // 获取方块状态
        const BlockState* state = world.getBlockState(currentX, currentY, currentZ);

        // 区块未加载，视为空气继续
        if (state == nullptr) {
            continue;
        }

        // 空气或无碰撞方块，继续
        if (state->isAir() || !state->blocksMovement()) {
            continue;
        }

        // 击中方块！
        // 计算击中点的世界坐标
        // t值表示从起点到击中点的比例
        f32 hitT;
        if (lastFace == Direction::West || lastFace == Direction::East) {
            hitT = tMaxX - tDeltaX;  // 上一个t值
        } else if (lastFace == Direction::Down || lastFace == Direction::Up) {
            hitT = tMaxY - tDeltaY;
        } else {
            hitT = tMaxZ - tDeltaZ;
        }

        // 限制在有效范围内
        hitT = std::max(0.0f, hitT);

        // 计算击中点（从起点出发）
        Vector3 hitPos = context.ray.at(hitT);
        f32 distance = hitPos.distance(start);

        return BlockRaycastResult::hit(
            hitPos,
            BlockPos(currentX, currentY, currentZ),
            lastFace,
            distance
        );
    }

    // 未击中任何方块
    return BlockRaycastResult::miss();
}

} // namespace mc
