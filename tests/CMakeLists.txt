# tests/CMakeLists.txt
# 测试配置

find_package(GTest CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)

# 通用测试可执行文件
add_executable(mc_tests
    common/test_math.cpp
    common/test_network.cpp
    common/test_world.cpp
    common/test_core.cpp
    # common/test_blockid.cpp  # 已移除 - 使用新的方块系统
    # common/test_noise.cpp     # 已移除 - Noise 类已删除
    common/test_entity.cpp
    common/test_chunksync.cpp
    common/test_localconnection.cpp
    common/test_chunkloadticket.cpp
    # common/test_collision.cpp  # 暂时禁用 - 依赖旧的碰撞系统
    common/test_property.cpp
    common/test_block.cpp
    common/test_chunk_generation.cpp
    common/test_biome.cpp
    common/test_surface.cpp
    common/test_ore_feature.cpp
    common/test_carver.cpp
    common/test_tree_feature.cpp
    common/test_decoration_stage.cpp
    common/test_time.cpp
    common/test_item.cpp
    common/test_block_item.cpp
    common/test_inventory.cpp
    common/test_item_entity.cpp
    common/test_container.cpp
    common/item/crafting/IngredientTest.cpp
    common/item/crafting/RecipeManagerTest.cpp
    common/item/crafting/ShapedRecipeTest.cpp
    common/item/crafting/ShapelessRecipeTest.cpp
    common/item/crafting/RecipeLoaderTest.cpp
    common/entity/inventory/CraftingInventoryTest.cpp
    common/world/blockentity/BlockEntityTest.cpp
    common/command/test_command_dispatcher.cpp
    entity/EntityCoreTests.cpp
    entity/AttributeTests.cpp
    entity/LivingEntityTests.cpp
    entity/GoalTests.cpp
    entity/PathfindingTests.cpp
    common/SettingsTest.cpp
    server/ServerWorldTest.cpp
    server/test_integrated_server.cpp
    server/test_chunk_worker_pool.cpp
    server/test_server_chunk_manager.cpp
    # ServerCore 模块测试
    server/core/TimeManagerTest.cpp
    server/core/PlayerManagerTest.cpp
    server/core/ConnectionManagerTest.cpp
    server/core/TeleportManagerTest.cpp
    server/core/KeepAliveManagerTest.cpp
    server/core/PositionTrackerTest.cpp
    server/core/ServerCoreTest.cpp
    client/test_mesh_worker_pool.cpp
    client/renderer/test_renderer.cpp
    client/renderer/test_gui_item_color.cpp
    network/EntityPacketsTest.cpp
    network/LocalServerConnectionTest.cpp
    common/entity/loot/LootTest.cpp
    common/world/gen/WorldGenSpawnerTest.cpp
    common/world/gen/ChunkSpawnIntegrationTest.cpp
    # Perfetto 测试（集成到主测试套件）
    common/perfetto/PerfettoTest.cpp
)

# 添加服务端源文件
target_sources(mc_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/server/core/ServerCore.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PlayerManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/ConnectionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/TimeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/TeleportManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/KeepAliveManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PositionTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PacketHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
        ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpSession.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
        ${CMAKE_SOURCE_DIR}/src/server/settings/ServerSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameModeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TimeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/KillCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ListCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/HelpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SeedCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeleportCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GiveCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/LocalConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/LocalServerConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/ChunkSync.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/PacketSerializer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/Packet.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/EntityPackets.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/WorldCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/Biome.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeGenerationSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/LazyArea.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/LayerUtil.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/transformers/BasicTransformers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/transformers/BiomeTransformers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/surface/SurfaceBuilders.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/Feature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/FeatureSpread.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/ConfiguredFeature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/ore/OreFeature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/TreeFeature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/trunk/TrunkPlacer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/trunk/StraightTrunkPlacer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/foliage/FoliagePlacer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/foliage/BlobFoliagePlacer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/placement/Placement.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/spawn/WorldGenSpawner.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/spawn/MobSpawnInfo.cpp
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
        # 物品系统
        ${CMAKE_SOURCE_DIR}/src/common/item/Item.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/ItemStack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/ItemRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/Items.cpp
        # 背包系统
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/Slot.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/PlayerInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/Container.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/CraftingInventory.cpp
        # 合成系统
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/Ingredient.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/ShapedRecipe.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/ShapelessRecipe.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeSerializers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeLoader.cpp
        # 方块实体系统
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/BlockEntityType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/BlockEntity.cpp
        # 物品实体和掉落表
        ${CMAKE_SOURCE_DIR}/src/common/entity/ItemEntity.cpp
        # 掉落系统
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/RandomRanges.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootPool.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootTable.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/drop/DropTables.cpp
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

target_link_libraries(mc_tests
    PRIVATE
        mc_common
        GTest::gtest
        GTest::gtest_main
        asio::asio
        nlohmann_json::nlohmann_json
        glm::glm
        spdlog::spdlog
)

if(WIN32)
    target_link_libraries(mc_tests PRIVATE ws2_32)
endif()

target_include_directories(mc_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

mc_set_compiler_warnings(mc_tests)

# 资源测试可执行文件
add_executable(mc_resource_tests
    client/resource/test_resource_location.cpp
    client/resource/test_model_loader.cpp
)

target_link_libraries(mc_resource_tests
    PRIVATE
        mc_common
        GTest::gtest
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        glm::glm
        spdlog::spdlog
)

# 添加客户端资源源文件
target_sources(mc_resource_tests
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

target_include_directories(mc_resource_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

# stb_image 实现放在TextureAtlasBuilder.cpp中

mc_set_compiler_warnings(mc_resource_tests)

# 发现测试
include(GoogleTest)
gtest_discover_tests(mc_tests)
gtest_discover_tests(mc_resource_tests)

# 输出测试可执行文件位置
set_target_properties(mc_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(mc_resource_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
