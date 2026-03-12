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
    renderer/VulkanContext.cpp
    renderer/VulkanSwapchain.cpp
    renderer/VulkanPipeline.cpp
    renderer/VulkanRenderer.cpp
    renderer/VulkanBuffer.cpp
    renderer/VulkanTexture.cpp
    renderer/ChunkRenderer.cpp
    renderer/MeshWorkerPool.cpp
    renderer/MeshTypes.cpp
    renderer/ChunkMesher.cpp
    renderer/Camera.cpp
    renderer/UniformBuffer.cpp
    renderer/Descriptor.cpp
    renderer/DefaultTextureAtlas.cpp
    renderer/sky/CelestialCalculations.cpp
    renderer/sky/SkyRenderer.cpp
    resource/BlockModelLoader.cpp
    resource/BlockStateLoader.cpp
    resource/TextureAtlasBuilder.cpp
    resource/ResourceManager.cpp
    resource/BlockModelCache.cpp
    world/ClientWorld.cpp
    network/NetworkClient.cpp
    ui/FontTextureAtlas.cpp
    ui/Font.cpp
    ui/FontRenderer.cpp
    ui/GuiRenderer.cpp
    ui/DefaultAsciiFont.cpp
    ui/DebugScreen.cpp
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
    # 客户端实体管理
    world/entity/ClientEntity.cpp
    world/entity/ClientEntityManager.cpp
    # 实体纹理加载
    resource/EntityTextureLoader.cpp
    # 内置服务端支持
    ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ChunkWorkerPool.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
    ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameModeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TimeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/KillCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/HelpCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SeedCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeleportCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/GiveCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
)

target_link_libraries(minecraft-client
    PRIVATE
        mr_common
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        glfw
        Vulkan::Vulkan
)

if(WIN32)
    target_link_libraries(minecraft-client PRIVATE user32 gdi32 ws2_32)
endif()

mr_set_compiler_warnings(minecraft-client)

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
