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
    renderer/trident/core/TridentContext.cpp
    renderer/trident/core/TridentSwapchain.cpp
    renderer/trident/core/TridentEngine.cpp
    renderer/trident/core/buffer/TridentBuffer.cpp
    renderer/trident/core/texture/TridentTexture.cpp
    renderer/trident/core/pipeline/TridentPipeline.cpp
    renderer/trident/core/render/RenderPassManager.cpp
    renderer/trident/core/render/FrameManager.cpp
    renderer/trident/core/render/DescriptorManager.cpp
    renderer/trident/core/render/UniformManager.cpp
    # 雾效果模块
    renderer/trident/fog/FogManager.cpp
    # 云渲染模块
    renderer/trident/cloud/CloudRenderer.cpp
    # 天气渲染模块
    renderer/trident/weather/WeatherRenderer.cpp
    # 粒子系统
    renderer/trident/particle/Particle.cpp
    renderer/trident/particle/ParticleManager.cpp
    renderer/trident/particle/particles/RainParticle.cpp
    renderer/trident/particle/particles/SnowParticle.cpp
    # 子渲染器
    renderer/trident/chunk/ChunkRenderer.cpp
    renderer/trident/chunk/ChunkMesher.cpp
    renderer/trident/sky/SkyRenderer.cpp
    renderer/trident/sky/CelestialCalculations.cpp
    renderer/trident/gui/GuiRenderer.cpp
    renderer/trident/gui/GuiTextureAtlas.cpp
    renderer/trident/item/ItemRenderer.cpp
    renderer/trident/entity/EntityRenderer.cpp
    renderer/trident/entity/EntityRendererManager.cpp
    renderer/trident/entity/model/ModelRenderer.cpp
    renderer/trident/entity/model/EntityModel.cpp
    renderer/trident/entity/model/AnimalModels.cpp
    renderer/trident/entity/EntityPipeline.cpp
    renderer/trident/entity/EntityTextureAtlas.cpp
    # 共享渲染组件
    renderer/mesh/MeshWorkerPool.cpp
    renderer/MeshTypes.cpp
    renderer/Camera.cpp
    # 资源加载
    resource/BlockModelLoader.cpp
    resource/BlockStateLoader.cpp
    resource/TextureAtlasBuilder.cpp
    resource/ResourceManager.cpp
    resource/BlockModelCache.cpp
    resource/ItemTextureAtlas.cpp
    resource/EntityTextureLoader.cpp
    # 世界
    world/ClientWorld.cpp
    network/NetworkClient.cpp
    # UI (平台无关)
    ui/FontTextureAtlas.cpp
    ui/Font.cpp
    ui/FontRenderer.cpp
    ui/DefaultAsciiFont.cpp
    ui/debug/DebugScreen.cpp
    ui/crosshair/CrosshairRenderer.cpp
    ui/hud/HudRenderer.cpp
    ui/inventory/InventoryScreen.cpp
    ui/inventory/SlotRenderer.cpp
    ui/screen/ScreenManager.cpp
    ui/screen/CraftingScreen.cpp
    ui/chat/ChatHistory.cpp
    ui/chat/ChatScreen.cpp
    # 客户端实体管理
    world/entity/ClientEntity.cpp
    world/entity/ClientEntityManager.cpp
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
    ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
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
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/WeatherCommand.cpp
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
