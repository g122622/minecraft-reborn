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
│   ├── math/        # Vector3, MathUtils, PerlinNoise, SimplexNoise
│   ├── network/     # Packet, PacketSerializer
│   ├── world/       # BlockID, ChunkPos, BlockPos, ChunkData, TerrainGenerator
│   └── renderer/    # MeshTypes, ChunkMesher (shared rendering data)
├── server/          # Server application
│   ├── application/ # ServerApplication, ServerLoop
│   ├── network/     # TcpServer, TcpSession
│   └── world/       # ServerWorld, ServerChunk
├── client/          # Client application
│   ├── application/ # ClientApplication, GameLoop
│   ├── window/      # Window (GLFW wrapper)
│   ├── input/       # InputManager
│   └── renderer/    # Vulkan rendering
│       ├── VulkanContext.hpp    # Vulkan instance, device, queues
│       ├── VulkanSwapchain.hpp  # Swapchain management
│       ├── VulkanPipeline.hpp   # Pipeline and render pass
│       ├── VulkanRenderer.hpp   # Main renderer
│       ├── VulkanBuffer.hpp     # GPU buffer management
│       ├── VulkanTexture.hpp    # Texture and texture atlas
│       └── ChunkRenderer.hpp    # Chunk mesh GPU buffers
└── modding/         # JavaScript mod system (future)
```

## Key Types

All types are in namespace `mr` (client types in `mr::client`, server types in `mr::server`):

- **Primitive types**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64`
- **String types**: `String` (std::string), `StringView` (std::string_view)
- **Game types**: `ChunkCoord`, `BlockCoord`, `BlockId`, `EntityId`, `DimensionId`
- **World types**: `ChunkId`, `BlockPos`, `ChunkPos`, `BlockState`, `ChunkSection`, `ChunkData`
- **Renderer types**: `Vertex`, `Face`, `MeshData`, `TextureRegion`, `BlockModel`, `TextureAtlas`
- **Error handling**: `Result<T>` with `Error` class and `ErrorCode` enum

## Chunk Mesh Generation

The chunk mesh system generates renderable meshes from chunk data:

- **BlockGeometry**: Provides face vertices, normals, and directions for cube blocks
- **MeshData**: Stores vertex and index buffers for a mesh
- **ChunkMesher**: Generates mesh from ChunkData with face culling
- **ChunkRenderData**: Per-chunk render data with solid/transparent mesh separation
- **ChunkMeshCache**: LRU cache for chunk mesh data

```cpp
// Generate mesh for a chunk
MeshData mesh;
ChunkMesher::generateMesh(chunkData, mesh, neighborChunks);
```

## Vulkan Rendering

The client uses Vulkan for rendering:

- **VulkanContext**: Vulkan instance, physical/logical device, queues
- **VulkanSwapchain**: Swapchain and image views
- **VulkanPipeline**: Graphics pipeline, render pass, descriptor layouts
- **VulkanRenderer**: Main renderer coordinating all Vulkan objects
- **VulkanBuffer**: GPU buffer management (vertex, index, staging)
- **VulkanTexture**: Texture image, view, sampler management
- **VulkanTextureAtlas**: Texture atlas for block textures
- **ChunkRenderer**: Manages chunk GPU buffers and rendering

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

### Error Codes

Key error codes in `ErrorCode` enum:
- General: `Unknown`, `InvalidArgument`, `NullPointer`, `OutOfRange`
- Resource: `NotFound`, `AlreadyExists`, `ResourceExhausted`, `OutOfMemory`
- File: `FileNotFound`, `FileOpenFailed`, `FileReadFailed`, `FileWriteFailed`
- Network: `ConnectionFailed`, `ConnectionClosed`, `ConnectionTimeout`, `InvalidPacket`
- Game: `InvalidBlock`, `InvalidItem`, `InvalidEntity`, `InvalidWorld`
- Render: `InitializationFailed`, `OperationFailed`, `CapacityExceeded`, `Unsupported`

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
- **VulkanMemoryAllocator** - GPU memory management
- **asio** - Networking (async I/O)
- **GTest** - Testing framework
- **stb** - Image loading

## Code Style

- C++17 standard
- `#pragma once` for header guards
- `[[nodiscard]]` for functions returning values that must be checked
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Use `const&` for large object parameters
- Use `string_view` for read-only string parameters

## Current Status

- **Core**: Complete (types, math, error handling)
- **Network**: Basic implementation (TCP server, packet serialization)
- **World**: Complete (chunk storage, terrain generation)
- **Renderer**: In progress (Vulkan context, basic mesh generation)
- **Tests**: 163 tests passing

## Self-Maintenance Rule

**After every major change** (new model, new page, new controller, route changes, migration changes, new test files, architectural shifts), update this CLAUDE.md file to reflect the current state. Specifically:

- Add new models/controllers/pages/routes to the relevant tables below
- Update test count if new tests are added
- Add any new gotchas or patterns to the "Gotchas & Pitfalls" section
- Update the "Current Status" section if the status changes
- Keep this file as the single source of truth for AI sessions working on this project
