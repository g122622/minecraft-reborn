# src/common/perfetto/CMakeLists.txt
# Perfetto 性能追踪库
#
# 注意：perfetto.cc 通过 TraceCategories.cpp 包含，不单独编译

add_library(mc_perfetto STATIC
    PerfettoManager.cpp
    TraceCategories.cpp
)

target_include_directories(mc_perfetto
    PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/perfetto
)

target_link_libraries(mc_perfetto
    PUBLIC
        spdlog::spdlog
)

# Windows 平台特殊配置
if(WIN32)
    # Perfetto SDK 生成大量符号，需要 /bigobj 选项
    target_compile_options(mc_perfetto PRIVATE "/bigobj")

    # 禁用 Windows 遗留定义，避免与 Perfetto 冲突
    target_compile_definitions(mc_perfetto PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

    # 链接 WinSock2
    target_link_libraries(mc_perfetto PRIVATE ws2_32)
endif()

# 根据追踪开关设置编译定义
if(MC_ENABLE_TRACING)
    target_compile_definitions(mc_perfetto PUBLIC MC_ENABLE_TRACING=1)
else()
    target_compile_definitions(mc_perfetto PUBLIC MC_ENABLE_TRACING=0)
endif()

# 设置 C++17 标准
target_compile_features(mc_perfetto PUBLIC cxx_std_17)

# 应用编译警告设置
mc_set_compiler_warnings(mc_perfetto)

# 创建别名库，便于使用
add_library(mc::perfetto ALIAS mc_perfetto)
