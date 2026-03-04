# src/client/CMakeLists.txt
# 客户端可执行文件

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(minecraft-client
    main.cpp
    application/ClientApplication.cpp
    window/Window.cpp
    input/InputManager.cpp
    renderer/VulkanContext.cpp
    renderer/VulkanSwapchain.cpp
    renderer/VulkanPipeline.cpp
    renderer/VulkanRenderer.cpp
    renderer/VulkanBuffer.cpp
    renderer/VulkanTexture.cpp
    renderer/ChunkRenderer.cpp
    renderer/Camera.cpp
    renderer/UniformBuffer.cpp
    renderer/Descriptor.cpp
    renderer/DefaultTextureAtlas.cpp
    resource/BlockModelLoader.cpp
    resource/BlockStateLoader.cpp
    resource/TextureAtlasBuilder.cpp
    resource/ResourceManager.cpp
    world/ClientWorld.cpp
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
    target_link_libraries(minecraft-client PRIVATE user32 gdi32)
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
