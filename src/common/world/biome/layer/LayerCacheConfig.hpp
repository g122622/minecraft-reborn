#pragma once

#include "../../../core/Types.hpp"

namespace mc {

/**
 * @brief 生物群系层缓存配置
 *
 * 控制层系统的缓存行为，可通过编译时宏调整。
 */
namespace LayerCacheConfig {

/**
 * @brief 默认缓存大小
 *
 * 缓存存储坐标 -> 生物群系ID 的映射。
 * 较大的缓存提高命中率但占用更多内存。
 */
#ifndef MC_LAYER_CACHE_SIZE
#define MC_LAYER_CACHE_SIZE 4096
#endif

constexpr i32 DEFAULT_CACHE_SIZE = MC_LAYER_CACHE_SIZE;

} // namespace LayerCacheConfig

} // namespace mc
