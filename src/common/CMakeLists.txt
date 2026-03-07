# src/common/CMakeLists.txt
# 公共库 - 客户端和服务端共享

add_library(mr_common STATIC
    # Core
    core/Result.cpp
    core/settings/SettingsBase.cpp

    # Input
    input/KeyBinding.cpp

    # Math
    math/MathUtils.cpp
    math/Noise.cpp
    math/ray/Raycast.cpp

    # Network
    network/Packet.cpp
    network/PacketSerializer.cpp
    network/ChunkSync.cpp
    network/LocalConnection.cpp

    # Entity
    entity/Entity.cpp
    entity/Player.cpp
    entity/PlayerManager.cpp

    # World - Block System
    world/block/Material.cpp
    world/block/Block.cpp
    world/block/BlockRegistry.cpp
    world/block/VanillaBlocks.cpp
    world/block/blocks/AirBlock.cpp
    world/block/blocks/SimpleBlock.cpp
    world/block/blocks/RotatedPillarBlock.cpp

    # World - Chunk
    world/chunk/ChunkData.cpp
    world/chunk/ChunkDistanceGraph.cpp
    world/chunk/ChunkLoadTicketManager.cpp
    world/chunk/ChunkStatus.cpp
    world/chunk/ChunkPrimer.cpp
    world/chunk/IChunk.cpp
    world/chunk/ChunkHolder.cpp

    # World - Generation
    world/gen/ImprovedNoiseGenerator.cpp
    world/gen/OctavesNoiseGenerator.cpp
    world/gen/NoiseSettings.cpp
    world/gen/IChunkGenerator.cpp
    world/gen/NoiseChunkGenerator.cpp

    # World - Time
    world/time/GameTime.cpp

    # Physics
    physics/PhysicsEngine.cpp
    # physics/collision/BlockCollision.cpp  # 已移除 - 使用新的方块系统

    # Resource
    resource/ResourceLocation.cpp
    resource/PackMetadata.cpp
    resource/IResourcePack.cpp
    resource/FolderResourcePack.cpp

    # Renderer (common)
    renderer/MeshTypes.cpp
    renderer/ChunkMesher.cpp
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
