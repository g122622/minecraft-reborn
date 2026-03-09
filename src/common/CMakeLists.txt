# src/common/CMakeLists.txt
# 公共库 - 客户端和服务端共享

find_package(LibArchive REQUIRED)

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
    network/InventoryPackets.cpp

    # Entity
    entity/Entity.cpp
    entity/Player.cpp
    entity/PlayerManager.cpp
    entity/ItemEntity.cpp
    entity/inventory/Slot.cpp
    entity/inventory/PlayerInventory.cpp
    entity/inventory/Container.cpp

    # Item System
    item/Item.cpp
    item/ItemStack.cpp
    item/ItemRegistry.cpp
    item/Items.cpp

    # World - Drop Tables
    world/drop/DropTables.cpp

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

    # World - Generation - Noise
    world/gen/noise/ImprovedNoiseGenerator.cpp
    world/gen/noise/OctavesNoiseGenerator.cpp

    # World - Generation - Chunk Generators
    world/gen/chunk/IChunkGenerator.cpp
    world/gen/chunk/NoiseChunkGenerator.cpp

    # World - Generation - Carvers
    world/gen/carver/WorldCarver.cpp
    world/gen/carver/CaveCarver.cpp
    world/gen/carver/CanyonCarver.cpp

    # World - Biome
    world/biome/Biome.cpp
    world/biome/BiomeProvider.cpp
    world/biome/BiomeRegistry.cpp
    world/biome/BiomeGenerationSettings.cpp

    # World - Biome - Layer
    world/biome/layer/LazyArea.cpp
    world/biome/layer/LayerUtil.cpp
    world/biome/layer/transformers/BasicTransformers.cpp
    world/biome/layer/transformers/BiomeTransformers.cpp

    # World - Generation - Surface
    world/gen/surface/SurfaceBuilders.cpp

    # World - Generation - Feature
    world/gen/feature/Feature.cpp
    world/gen/feature/FeatureSpread.cpp
    world/gen/feature/ConfiguredFeature.cpp
    world/gen/feature/ore/OreFeature.cpp
    world/gen/feature/tree/TreeFeature.cpp
    world/gen/feature/tree/trunk/TrunkPlacer.cpp
    world/gen/feature/tree/trunk/StraightTrunkPlacer.cpp
    world/gen/feature/tree/foliage/FoliagePlacer.cpp
    world/gen/feature/tree/foliage/BlobFoliagePlacer.cpp

    # World - Generation - Placement
    world/gen/placement/Placement.cpp

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
    resource/ZipResourcePack.cpp
    resource/ResourcePackList.cpp
    resource/InMemoryResourcePack.cpp
    resource/VanillaResources.cpp
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
    PRIVATE
        LibArchive::LibArchive
)

mr_set_compiler_warnings(mr_common)

target_compile_features(mr_common PUBLIC cxx_std_17)

# 导出头文件
target_include_directories(mr_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
