# src/common/CMakeLists.txt
# 公共库 - 客户端和服务端共享

add_library(mr_common STATIC
    # Core
    core/Result.cpp

    # Math
    math/MathUtils.cpp
    math/Noise.cpp

    # Network
    network/Packet.cpp
    network/PacketSerializer.cpp
    network/ChunkSync.cpp
    network/LocalConnection.cpp

    # Entity
    entity/Entity.cpp
    entity/Player.cpp
    entity/PlayerManager.cpp

    # World
    world/BlockID.cpp
    world/chunk/ChunkData.cpp
    world/TerrainGenerator.cpp
    world/BlockProperties.cpp
    world/chunk/ChunkDistanceGraph.cpp
    world/chunk/ChunkLoadTicketManager.cpp

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
