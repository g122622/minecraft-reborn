# src/common/CMakeLists.txt
# 公共库 - 客户端和服务端共享

add_library(mr_common STATIC
    # Core
    core/Result.cpp

    # Math
    math/MathUtils.cpp

    # Network
    network/Packet.cpp
    network/PacketSerializer.cpp
)

target_include_directories(mr_common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(mr_common
    PUBLIC
        glm::glm
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

mr_set_compiler_warnings(mr_common)

target_compile_features(mr_common PUBLIC cxx_std_17)

# 导出头文件
target_include_directories(mr_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
