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
    # common/test_blockid.cpp  # 已移除 - 使用新的方块系统
    common/test_noise.cpp
    common/test_entity.cpp
    common/test_chunksync.cpp
    common/test_localconnection.cpp
    common/test_chunkloadticket.cpp
    # common/test_collision.cpp  # 暂时禁用 - 依赖旧的碰撞系统
    common/test_property.cpp
    common/test_block.cpp
    common/test_chunk_generation.cpp
    common/test_time.cpp
    common/SettingsTest.cpp
    server/ServerWorldTest.cpp
    server/test_integrated_server.cpp
    server/test_chunk_worker_pool.cpp
    server/test_server_chunk_manager.cpp
    client/test_mesh_worker_pool.cpp
    client/renderer/test_renderer.cpp
)

# 添加服务端源文件
target_sources(mr_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpSession.cpp
        ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/settings/ServerSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/LocalConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/ChunkSync.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/PacketSerializer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/Packet.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkData.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkDistanceGraph.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkLoadTicketManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkStatus.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkPrimer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/IChunk.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkHolder.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/noise/ImprovedNoiseGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/noise/OctavesNoiseGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/chunk/IChunkGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/chunk/NoiseChunkGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/CaveCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/CanyonCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/Biome.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeRegistry.cpp
        # ${CMAKE_SOURCE_DIR}/src/common/world/BlockID.cpp  # 已移除 - 使用新的方块系统
        ${CMAKE_SOURCE_DIR}/src/common/entity/Player.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/Entity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/PhysicsEngine.cpp
        # ${CMAKE_SOURCE_DIR}/src/common/physics/collision/BlockCollision.cpp  # 已移除 - 使用新的方块系统
        # 新的方块系统源文件
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/VanillaBlocks.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
        # 时间系统
        ${CMAKE_SOURCE_DIR}/src/common/world/time/GameTime.cpp
        # 天体计算
        ${CMAKE_SOURCE_DIR}/src/client/renderer/sky/CelestialCalculations.cpp
        # 设置系统
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        # 网格构建线程池
        ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshWorkerPool.cpp
        # 区块网格生成（已在 client/renderer）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/ChunkMesher.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
        # 区块模型缓存
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
)

target_link_libraries(mr_tests
    PRIVATE
        mr_common
        GTest::gtest
        GTest::gtest_main
        asio::asio
        nlohmann_json::nlohmann_json
        glm::glm
        spdlog::spdlog
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
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ResourcePackList.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ZipResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/FolderResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/IResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/PackMetadata.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ResourceLocation.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/VanillaBlocks.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
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
