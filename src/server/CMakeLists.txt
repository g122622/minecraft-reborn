# src/server/CMakeLists.txt
# 服务端可执行文件

add_executable(minecraft-server
    main.cpp
    application/ServerApplication.cpp
    application/IntegratedServer.cpp
    settings/ServerSettings.cpp
    network/TcpServer.cpp
    network/TcpSession.cpp
    world/ServerWorld.cpp
    world/ChunkWorkerPool.cpp
    world/ServerChunkManager.cpp
    player/ServerPlayer.cpp
    # 命令系统
    command/ServerCommandSource.cpp
    command/CommandRegistry.cpp
    command/commands/GameModeCommand.cpp
    command/commands/TimeCommand.cpp
    command/commands/KillCommand.cpp
    command/commands/ListCommand.cpp
    command/commands/HelpCommand.cpp
    command/commands/SeedCommand.cpp
    command/commands/TeleportCommand.cpp
    command/commands/GiveCommand.cpp
    command/commands/ClearCommand.cpp
    # 菜单系统
    menu/CraftingMenu.cpp
)

target_link_libraries(minecraft-server
    PRIVATE
        mr_common
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(minecraft-server PRIVATE ws2_32)
endif()

mr_set_compiler_warnings(minecraft-server)

target_compile_features(minecraft-server PRIVATE cxx_std_17)

set_target_properties(minecraft-server PROPERTIES
    OUTPUT_NAME "minecraft-server"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
