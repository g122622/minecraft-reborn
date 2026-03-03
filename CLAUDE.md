# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Minecraft Reborn is a modern Minecraft clone with client-server architecture written in C++17 using Vulkan for rendering.

## Build Commands

```powershell
# Set vcpkg environment (required)
$env:VCPKG_ROOT = "D:\tools\vcpkg"

# Configure project (first time or after CMakeLists changes)
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Debug
cmake --build build --config Release

# Run tests
./build/bin/Debug/mr_tests.exe

# Run server
./build/bin/Debug/minecraft-server.exe --help

# Run client
./build/bin/Debug/minecraft-client.exe
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `MR_BUILD_CLIENT` | ON | Build client application |
| `MR_BUILD_SERVER` | ON | Build server application |
| `MR_BUILD_TESTS` | ON | Build unit tests |
| `MR_ENABLE_SANITIZERS` | OFF | Enable sanitizers for debug |
| `MR_ENABLE_VULKAN_VALIDATION` | ON | Enable Vulkan validation layers |

## Architecture

```
src/
├── common/          # Shared code between client and server
│   ├── core/        # Types, Result, Constants
│   ├── math/        # Vector3, MathUtils, PerlinNoise
│   ├── world/       # BlockID, ChunkPos, BlockPos
│   └── util/        # UUID, Hash, Endian
├── server/          # Server application
│   ├── application/ # ServerApplication, ServerLoop
│   ├── network/     # TcpServer, SessionManager
│   └── world/       # ServerWorld, ServerChunk
├── client/          # Client application
│   ├── application/ # ClientApplication, GameLoop
│   ├── window/      # Window (GLFW wrapper)
│   ├── input/       # InputManager
│   └── renderer/    # Vulkan rendering
└── modding/         # JavaScript mod system (future)
```

## Key Types

All types are in namespace `mr`:

- **Primitive types**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64`
- **String types**: `String` (std::string), `StringView` (std::string_view)
- **Game types**: `ChunkCoord`, `BlockCoord`, `BlockId`, `EntityId`, `DimensionId`
- **Error handling**: `Result<T>` with `Error` class and `ErrorCode` enum

## Error Handling Pattern

Use `Result<T>` for fallible operations:

```cpp
// Returning a value or error
Result<int> divide(int a, int b) {
    if (b == 0) {
        return Error(ErrorCode::InvalidArgument, "Division by zero");
    }
    return a / b;
}

// Checking result
auto result = divide(10, 2);
if (result.success()) {
    int value = result.value();
} else {
    // Handle error
}
```

## Naming Conventions

From AGENTS.md:

- **Namespaces**: lowercase (`mr`, `mr::client`, `mr::server`)
- **Classes/Structs**: PascalCase (`ChunkManager`, `Vector3`)
- **Functions**: camelCase (`loadChunk`, `getPlayerName`)
- **Member variables**: `m_` prefix + camelCase (`m_health`, `m_position`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_PLAYERS`, `CHUNK_WIDTH`)
- **Enum values**: PascalCase (`BlockType::Stone`, `ErrorCode::NotFound`)
- **Files**: PascalCase (`ChunkManager.hpp`, `ChunkManager.cpp`)

## Include Order

1. Corresponding header file (for .cpp files)
2. Project internal headers
3. Third-party library headers
4. Standard library headers

## Dependencies

Managed via vcpkg:
- **glm** - Math library
- **spdlog** - Logging
- **nlohmann-json** - JSON parsing
- **glfw3** - Window/input
- **Vulkan** - Graphics API
- **asio** - Networking (async I/O)
- **GTest** - Testing framework

## Code Style

- C++17 standard
- `#pragma once` for header guards
- `[[nodiscard]]` for functions returning values that must be checked
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Use `const&` for large object parameters
- Use `string_view` for read-only string parameters
