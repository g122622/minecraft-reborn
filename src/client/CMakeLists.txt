# src/client/CMakeLists.txt
# 客户端可执行文件

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(minecraft-client
    main.cpp
    application/ClientApplication.cpp
    window/Window.cpp
    input/InputManager.cpp
    settings/ClientSettings.cpp
    # Trident 渲染引擎 API 层
    renderer/api/Types.cpp
    # Trident 渲染引擎核心 (Vulkan 实现)
    renderer/trident/TridentContext.cpp
    renderer/trident/TridentSwapchain.cpp
    renderer/trident/TridentEngine.cpp
    renderer/trident/buffer/TridentBuffer.cpp
    renderer/trident/texture/TridentTexture.cpp
    renderer/trident/pipeline/TridentPipeline.cpp
    renderer/trident/render/RenderPassManager.cpp
    renderer/trident/render/FrameManager.cpp
    renderer/trident/render/DescriptorManager.cpp
    renderer/trident/render/UniformManager.cpp
    # Vulkan 工具类 (仍被部分子渲染器使用)
    renderer/VulkanContext.cpp
    renderer/VulkanBuffer.cpp
    renderer/VulkanTexture.cpp
    renderer/VulkanPipeline.cpp
    # 子渲染器
    renderer/ChunkRenderer.cpp
    renderer/mesh/MeshWorkerPool.cpp
    renderer/MeshTypes.cpp
    renderer/chunk/ChunkMesher.cpp
    renderer/Camera.cpp
    renderer/sky/CelestialCalculations.cpp
    renderer/sky/SkyRenderer.cpp
    resource/BlockModelLoader.cpp
    resource/BlockStateLoader.cpp
    resource/TextureAtlasBuilder.cpp
    resource/ResourceManager.cpp
    resource/BlockModelCache.cpp
    resource/ItemTextureAtlas.cpp
    world/ClientWorld.cpp
    network/NetworkClient.cpp
    ui/FontTextureAtlas.cpp
    ui/Font.cpp
    ui/FontRenderer.cpp
    ui/GuiRenderer.cpp
    ui/DefaultAsciiFont.cpp
    ui/debug/DebugScreen.cpp
    ui/crosshair/CrosshairRenderer.cpp
    ui/hud/HudRenderer.cpp
    ui/inventory/InventoryScreen.cpp
    ui/inventory/SlotRenderer.cpp
    ui/screen/ScreenManager.cpp
    ui/screen/CraftingScreen.cpp
    ui/texture/GuiTextureAtlas.cpp
    ui/chat/ChatHistory.cpp
    ui/chat/ChatScreen.cpp
    # 实体渲染
    renderer/entity/EntityRenderer.cpp
    renderer/entity/EntityRendererManager.cpp
    renderer/entity/model/ModelRenderer.cpp
    renderer/entity/model/EntityModel.cpp
    renderer/entity/model/AnimalModels.cpp
    renderer/entity/EntityPipeline.cpp
    renderer/entity/EntityTextureAtlas.cpp
    # 物品渲染
    renderer/item/ItemRenderer.cpp
    # 客户端实体管理
    world/entity/ClientEntity.cpp
    world/entity/ClientEntityManager.cpp
    # 实体纹理加载
    resource/EntityTextureLoader.cpp
    # 内置服务端支持
    ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
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
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
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
)

target_link_libraries(minecraft-client
    PRIVATE
        mc_common
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        glfw
        Vulkan::Vulkan
)

if(WIN32)
    target_link_libraries(minecraft-client PRIVATE user32 gdi32 ws2_32)
endif()

mc_set_compiler_warnings(minecraft-client)

target_compile_features(minecraft-client PRIVATE cxx_std_17)

target_compile_definitions(minecraft-client PRIVATE
    GLFW_INCLUDE_VULKAN
)

set_target_properties(minecraft-client PROPERTIES
    OUTPUT_NAME "minecraft-client"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 依赖着色器编译
if(TARGET compile_shaders)
    add_dependencies(minecraft-client compile_shaders)
endif()
