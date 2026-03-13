/**
 * @file TraceCategories.hpp
 * @brief Perfetto 追踪分类定义
 *
 * 此文件定义 Perfetto 追踪分类。根据 Perfetto SDK 要求：
 * - PERFETTO_DEFINE_CATEGORIES 放在头文件中
 * - PERFETTO_TRACK_EVENT_STATIC_STORAGE() 放在一个 .cpp 文件中 (TraceCategories.cpp)
 *
 * 所有使用 MC_TRACE_EVENT 宏的文件都必须包含此头文件。
 */

#pragma once

#include "PerfettoConfig.hpp"

#if MC_ENABLE_TRACING

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <perfetto.h>

// 定义追踪分类
PERFETTO_DEFINE_CATEGORIES(
    // === 渲染分类 ===
    perfetto::Category("rendering.frame")
        .SetDescription("帧渲染生命周期事件"),
    perfetto::Category("rendering.vulkan")
        .SetDescription("Vulkan API 调用"),
    perfetto::Category("rendering.chunk_mesh")
        .SetDescription("区块网格生成和上传"),
    perfetto::Category("rendering.entity")
        .SetDescription("实体渲染"),

    // === 游戏逻辑分类 ===
    perfetto::Category("game.tick")
        .SetDescription("游戏刻处理"),
    perfetto::Category("game.entity")
        .SetDescription("实体更新"),
    perfetto::Category("game.physics")
        .SetDescription("物理模拟"),
    perfetto::Category("game.ai")
        .SetDescription("AI 目标处理"),

    // === 世界分类 ===
    perfetto::Category("world.chunk")
        .SetDescription("区块操作"),
    perfetto::Category("world.chunk_gen")
        .SetDescription("区块生成各阶段"),
    perfetto::Category("world.chunk_load")
        .SetDescription("区块加载和卸载"),
    perfetto::Category("world.biome")
        .SetDescription("生物群系生成"),

    // === 网络分类 ===
    perfetto::Category("network.packet")
        .SetDescription("网络包处理"),
    perfetto::Category("network.sync")
        .SetDescription("状态同步"),
    perfetto::Category("network.connection")
        .SetDescription("连接管理"),

    // === I/O 分类 ===
    perfetto::Category("io.file")
        .SetDescription("文件 I/O 操作"),
    perfetto::Category("io.resource")
        .SetDescription("资源加载"),

    // === 内存分类 ===
    perfetto::Category("memory.allocation")
        .SetDescription("内存分配追踪"),
    perfetto::Category("memory.cache")
        .SetDescription("缓存操作")
);

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace mc {
namespace perfetto {

/**
 * @brief 初始化追踪分类
 *
 * 必须在程序启动时调用，通常由 PerfettoManager::initialize() 自动调用。
 */
void initTraceCategories();

} // namespace perfetto
} // namespace mc

#else // MC_ENABLE_TRACING == 0

// 禁用追踪时的空声明

namespace mc {
namespace perfetto {

inline void initTraceCategories() {}

} // namespace perfetto
} // namespace mc

#endif // MC_ENABLE_TRACING
