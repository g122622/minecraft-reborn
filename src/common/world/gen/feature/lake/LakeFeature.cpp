#include "LakeFeature.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc::world::gen::feature::lake {

LakeFeature::LakeFeature(const LakeFeatureConfig& config)
    : m_config(config)
{
}

bool LakeFeature::place(IWorldWriter& world, math::Random& rng, i32 x, i32 y, i32 z) {
    // 湖泊大小参数
    constexpr i32 RADIUS_X = 8;
    constexpr i32 RADIUS_Y = 4;
    constexpr i32 RADIUS_Z = 8;

    // 检查是否适合生成
    if (!canPlaceAt(world, x, y, z)) {
        return false;
    }

    // 生成椭圆形湖泊
    for (i32 dx = -RADIUS_X; dx <= RADIUS_X; ++dx) {
        for (i32 dy = -RADIUS_Y; dy <= RADIUS_Y; ++dy) {
            for (i32 dz = -RADIUS_Z; dz <= RADIUS_Z; ++dz) {
                // 椭球方程
                f32 dist = static_cast<f32>(dx * dx) / static_cast<f32>(RADIUS_X * RADIUS_X) +
                           static_cast<f32>(dy * dy) / static_cast<f32>(RADIUS_Y * RADIUS_Y) +
                           static_cast<f32>(dz * dz) / static_cast<f32>(RADIUS_Z * RADIUS_Z);

                if (dist <= 1.0f) {
                    // 内部填充流体
                    world.setBlock(x + dx, y + dy, z + dz, m_config.fluidState);
                } else if (dist <= 1.25f && dy <= 0) {
                    // 边界区域
                    if (m_config.borderState) {
                        world.setBlock(x + dx, y + dy, z + dz, m_config.borderState);
                    }
                }
            }
        }
    }

    // 在底部添加一些随机方块（增加自然感）
    if (m_config.borderState) {
        for (i32 i = 0; i < 8; ++i) {
            i32 dx = rng.nextInt(RADIUS_X * 2) - RADIUS_X;
            i32 dz = rng.nextInt(RADIUS_Z * 2) - RADIUS_Z;
            i32 dy = -RADIUS_Y + rng.nextInt(2);

            f32 dist = static_cast<f32>(dx * dx) / static_cast<f32>(RADIUS_X * RADIUS_X) +
                       static_cast<f32>(dy * dy) / static_cast<f32>(RADIUS_Y * RADIUS_Y) +
                       static_cast<f32>(dz * dz) / static_cast<f32>(RADIUS_Z * RADIUS_Z);

            if (dist <= 1.25f) {
                world.setBlock(x + dx, y + dy - 1, z + dz, m_config.borderState);
            }
        }
    }

    return true;
}

bool LakeFeature::canPlaceAt(IWorldWriter& world, i32 x, i32 y, i32 z) const {
    // 参考 MC 1.16.5: 湖泊生成位置检查
    // 水湖限制: Y >= 8
    // 熔岩湖限制: Y >= 8，在较低高度更常见

    // 水湖限制
    if (m_config.fluidBlock == VanillaBlocks::WATER) {
        return y >= 8 && y <= 256;
    }

    // 熔岩湖限制
    if (m_config.fluidBlock == VanillaBlocks::LAVA) {
        return y >= 8 && y <= 256;
    }

    // 检查周围是否有足够的固体方块来支撑湖泊
    // 这可以防止湖泊生成在空中或不稳定位置
    i32 solidCount = 0;
    constexpr i32 CHECK_RADIUS = 2;

    for (i32 dx = -CHECK_RADIUS; dx <= CHECK_RADIUS; ++dx) {
        for (i32 dy = -CHECK_RADIUS; dy <= CHECK_RADIUS; ++dy) {
            for (i32 dz = -CHECK_RADIUS; dz <= CHECK_RADIUS; ++dz) {
                // 跳过湖泊中心区域
                if (std::abs(dx) <= 1 && std::abs(dy) <= 1 && std::abs(dz) <= 1) {
                    continue;
                }

                // 这里简化处理：假设检查位置的方块
                // 完整实现需要从 world 读取方块
                ++solidCount;
            }
        }
    }

    // 至少需要一定数量的固体方块
    return solidCount >= 20;
}

LakeFeatureConfig LakeFeature::createWaterLake() {
    return LakeFeatureConfig(VanillaBlocks::WATER, VanillaBlocks::STONE);
}

LakeFeatureConfig LakeFeature::createLavaLake() {
    return LakeFeatureConfig(VanillaBlocks::LAVA, VanillaBlocks::STONE);
}

std::unique_ptr<LakeFeature> createWaterLakeFeature() {
    return std::make_unique<LakeFeature>(LakeFeature::createWaterLake());
}

std::unique_ptr<LakeFeature> createLavaLakeFeature() {
    return std::make_unique<LakeFeature>(LakeFeature::createLavaLake());
}

} // namespace mc::world::gen::feature::lake
