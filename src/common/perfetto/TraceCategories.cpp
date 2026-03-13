/**
 * @file TraceCategories.cpp
 * @brief Perfetto 追踪分类静态存储定义和 SDK 编译
 *
 * 此文件定义 PERFETTO_TRACK_EVENT_STATIC_STORAGE 并包含 Perfetto SDK 实现。
 * 将 perfetto.cc 包含到此文件中可以确保所有符号在同一个编译单元中。
 */

#include "TraceCategories.hpp"

#if MC_ENABLE_TRACING

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <perfetto.h>

// 为追踪分类预留内部静态存储
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

// 包含 Perfetto SDK 实现
// 这确保所有模板实例化和符号都在同一个编译单元中
#if defined(__has_include)
#if __has_include("perfetto.cc")
#include "perfetto.cc"
#endif
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace mc {
namespace perfetto {

void initTraceCategories() {
    // 分类已在 PERFETTO_DEFINE_CATEGORIES 中静态定义
    // 此函数预留给未来可能的动态分类注册需求
}

} // namespace perfetto
} // namespace mc

#endif // MC_ENABLE_TRACING
