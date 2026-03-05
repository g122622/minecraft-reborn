# tests/CMakeLists.txt
# 测试配置

find_package(GTest CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)

# 通用测试可执行文件
add_executable(mr_tests
    common/test_math.cpp
    common/test_network.cpp
    common/test_world.cpp
    common/test_core.cpp
    common/test_blockid.cpp
    common/test_noise.cpp
    common/test_renderer.cpp
    common/test_entity.cpp
    common/test_chunksync.cpp
    common/test_localconnection.cpp
    common/test_chunkloadticket.cpp
    common/test_collision.cpp
    server/ServerWorldTest.cpp
    server/test_integrated_server.cpp
)

# 添加服务端源文件
target_sources(mr_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpSession.cpp
        ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/LocalConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/ChunkSync.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/PacketSerializer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/Packet.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkData.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/TerrainGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkDistanceGraph.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkLoadTicketManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/BlockID.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/Player.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/Entity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/PhysicsEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/collision/BlockCollision.cpp
)

target_link_libraries(mr_tests
    PRIVATE
        mr_common
        GTest::gtest
        GTest::gtest_main
        asio::asio
)

if(WIN32)
    target_link_libraries(mr_tests PRIVATE ws2_32)
endif()

target_include_directories(mr_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

mr_set_compiler_warnings(mr_tests)

# 资源测试可执行文件
add_executable(mr_resource_tests
    client/resource/test_resource_location.cpp
    client/resource/test_model_loader.cpp
)

target_link_libraries(mr_resource_tests
    PRIVATE
        mr_common
        GTest::gtest
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        glm::glm
        spdlog::spdlog
)

# 添加客户端资源源文件
target_sources(mr_resource_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
)

target_include_directories(mr_resource_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

# stb_image 实现放在TextureAtlasBuilder.cpp中

mr_set_compiler_warnings(mr_resource_tests)

# 发现测试
include(GoogleTest)
gtest_discover_tests(mr_tests)
gtest_discover_tests(mr_resource_tests)

# 输出测试可执行文件位置
set_target_properties(mr_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(mr_resource_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
