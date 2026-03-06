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
│   ├── world/       # World generation and chunk management
│   │   ├── block/   # Block system (Block, BlockState, BlockRegistry)
│   │   ├── chunk/   # Chunk data structures
│   │   │   ├── ChunkData.hpp       # Final chunk data
│   │   │   ├── ChunkPos.hpp        # Chunk position
│   │   │   ├── ChunkStatus.hpp     # Generation stages (NEW)
│   │   │   ├── ChunkPrimer.hpp     # Intermediate chunk state (NEW)
│   │   │   ├── ChunkHolder.hpp     # Chunk state management (NEW)
│   │   │   ├── IChunk.hpp          # Chunk interface (NEW)
│   │   │   └── ChunkLoadTicketManager.hpp
│   │   └── gen/     # World generation (NEW)
│   │       ├── ImprovedNoiseGenerator.hpp  # MC-style Perlin noise
│   │       ├── OctavesNoiseGenerator.hpp   # Multi-octave noise
│   │       ├── NoiseSettings.hpp           # Noise configuration
│   │       ├── IChunkGenerator.hpp         # Generator interface
│   │       ├── NoiseChunkGenerator.hpp     # MC-style terrain generator
│   │       ├── BiomeProvider.hpp           # Biome distribution
│   │       └── WorldGenRegion.hpp          # Limited world view
│   ├── resource/    # Resource system
│   │   ├── ResourceLocation.hpp   # Resource identifier (namespace:path)
│   │   ├── IResourcePack.hpp      # Resource pack interface
│   │   ├── FolderResourcePack.hpp # Folder resource pack implementation
│   │   └── PackMetadata.hpp       # pack.mcmeta parsing
│   └── renderer/    # MeshTypes, ChunkMesher (shared rendering data)
├── server/          # Server application
│   ├── application/ # ServerApplication, ServerLoop
│   ├── network/     # TcpServer, TcpSession
│   └── world/       # ServerWorld
│       ├── ServerWorld.hpp
│       ├── ServerChunkManager.hpp  # Chunk manager (NEW)
│       └── ChunkWorkerPool.hpp     # Async generation (NEW)
├── client/          # Client application
│   ├── application/ # ClientApplication, GameLoop
│   ├── window/      # Window (GLFW wrapper)
│   ├── input/       # InputManager
│   ├── renderer/    # Vulkan rendering
│   │   ├── VulkanContext.hpp    # Vulkan instance, device, queues
│   │   ├── VulkanSwapchain.hpp  # Swapchain management
│   │   ├── VulkanPipeline.hpp   # Pipeline and render pass
│   │   ├── VulkanRenderer.hpp   # Main renderer
│   │   ├── VulkanBuffer.hpp     # GPU buffer management
│   │   ├── VulkanTexture.hpp    # Texture and texture atlas
│   │   └── ChunkRenderer.hpp    # Chunk mesh GPU buffers
│   └── resource/    # Client resource loading
│       ├── BlockModelLoader.hpp    # Model JSON parsing
│       ├── BlockStateLoader.hpp    # Block state JSON parsing
│       ├── TextureAtlasBuilder.hpp # Texture atlas construction
│       └── ResourceManager.hpp     # Resource manager facade
└── modding/         # JavaScript mod system (future)
```

## Key Types

All types are in namespace `mr` (client types in `mr::client`, server types in `mr::server`):

- **Primitive types**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64` （浮点数尽量使用f32而非f64以提升性能。）
- **String types**: `String` (std::string), `StringView` (std::string_view)
- **Game types**: `ChunkCoord`, `BlockCoord`, `BlockId`, `EntityId`, `DimensionId`
- **World types**: `ChunkId`, `BlockPos`, `ChunkPos`, `BlockState`, `ChunkSection`, `ChunkData`
- **Chunk generation types** (NEW):
  - `ChunkStatus`: Generation stages (EMPTY, BIOMES, NOISE, SURFACE, CARVERS, FEATURES, HEIGHTMAPS, FULL)
  - `ChunkPrimer`: Intermediate chunk state during generation
  - `ChunkHolder`: Manages chunk loading state and futures
  - `ChunkTask`: Generation task for worker pool
  - `BiomeId`, `BiomeDefinition`, `BiomeContainer`: Biome system
  - `Heightmap`, `HeightmapType`: Height tracking
  - `NoiseSettings`, `DimensionSettings`: Noise configuration
  - `IChunkGenerator`, `NoiseChunkGenerator`: Terrain generation
- **Renderer types**: `Vertex`, `Face`, `MeshData`, `TextureRegion`, `BlockModel`, `TextureAtlas`
- **Resource types**: `ResourceLocation`, `PackMetadata`, `IResourcePack`, `FolderResourcePack`
- **Model types**: `Direction`, `ModelElement`, `ModelFace`, `UnbakedBlockModel`, `BakedBlockModel`
- **Block state types**: `BlockStateVariant`, `VariantList`, `BlockStateDefinition`
- **Error handling**: `Result<T>` with `Error` class and `ErrorCode` enum

## Chunk Generation System

The chunk generation system follows MC 1.16.5 architecture:

### Generation Stages
```
EMPTY → BIOMES → NOISE → SURFACE → CARVERS → FEATURES → HEIGHTMAPS → FULL
```

Each stage has specific responsibilities:
- **EMPTY**: Initial state
- **BIOMES**: Generate biome data
- **NOISE**: Generate terrain density using multi-octave Perlin noise
- **SURFACE**: Apply surface blocks (grass, sand, etc.)
- **CARVERS**: Apply cave carving
- **FEATURES**: Place trees, ores, structures
- **HEIGHTMAPS**: Calculate final heightmaps
- **FULL**: Complete chunk

### Key Classes

```cpp
// Create a chunk generator
DimensionSettings settings = DimensionSettings::overworld();
NoiseChunkGenerator generator(seed, std::move(settings));

// Generate a chunk
ChunkPrimer primer(chunkX, chunkZ);
WorldGenRegion region(chunkX, chunkZ, neighbors);
generator.generateBiomes(region, primer);
generator.generateNoise(region, primer);
generator.buildSurface(region, primer);

// Convert to final chunk data
std::unique_ptr<ChunkData> data = primer.toChunkData();
```

### Async Generation

```cpp
// ServerChunkManager handles async generation
ServerChunkManager manager(world, std::move(generator));
manager.initialize();
manager.startWorkers(4);  // 4 worker threads

// Request chunk asynchronously
auto future = manager.getChunkAsync(x, z, &ChunkStatus::FULL);

// Tick the manager
manager.tick();
```

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

## Resource Pack System

The resource pack system parses standard Minecraft resource pack format:

### Resource Location
```cpp
// Parse "minecraft:textures/blocks/stone"
ResourceLocation loc("minecraft:textures/blocks/stone");
loc.namespace_();  // "minecraft"
loc.path();        // "textures/blocks/stone"
loc.toFilePath();  // "assets/minecraft/textures/blocks/stone"
```

### Loading a Resource Pack
```cpp
// Create folder resource pack
auto pack = std::make_shared<FolderResourcePack>("z:/方块概念材质");
auto result = pack->initialize();

// Load block states
BlockStateLoader stateLoader;
stateLoader.loadFromResourcePack(*pack);

// Load and bake models
BlockModelLoader modelLoader;
modelLoader.loadFromResourcePack(*pack);
auto bakedModel = modelLoader.bakeModel(ResourceLocation("minecraft:block/stone"));

// Build texture atlas
TextureAtlasBuilder atlasBuilder;
atlasBuilder.addTexture(*pack, ResourceLocation("minecraft:textures/blocks/stone"));
auto atlas = atlasBuilder.build();
```

### Model Inheritance
Models support parent inheritance (e.g., `cobblestone` inherits from `cube_all`):
- Parent models are resolved recursively
- Textures are inherited and can be overridden
- Elements from parent are used if child has none

### Block State Mapping
```cpp
// Get variant for block state
const auto* variant = blockStateLoader.getVariant(
    ResourceLocation("minecraft:oak_log"),
    "axis=y"  // or properties map
);
```

### Supported Files
- `pack.mcmeta` - Pack metadata
- `blockstates/*.json` - Block state definitions
- `models/block/*.json` - Block models
- `textures/blocks/*.png` - Block textures

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
- Resource Pack: `ResourcePackNotFound`, `ResourcePackInvalid`, `ResourceNotFound`, `ResourceParseError`, `TextureLoadFailed`, `TextureAtlasFull`, `ModelNotFound`, `BlockStateNotFound`

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

## 注意，你可随时访问mc java版本的源码来供自己参考：`D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft`，这很重要，因为当前项目是一个复刻项目，目标是完全使用cpp尽可能一致地复刻java版mc的游戏体验，并在存档、数据包等层面上尽可能兼容和复用现有java版minecraft生态

## 你需要先阅读readme文件了解怎么构建项目

## 需要断言+单测来保证代码质量；每个方法前都要附上doc注释说明方法的用法和注意事项（容易踩坑的地方）

## Current Status

- **Core**: Complete (types, math, error handling)
- **Network**: Basic implementation (TCP server, packet serialization)
- **World**: Complete (chunk storage, terrain generation)
- **Chunk Generation**: Complete (NEW)
  - ChunkStatus: Generation stages (EMPTY → BIOMES → NOISE → SURFACE → CARVERS → FEATURES → HEIGHTMAPS → FULL)
  - ChunkPrimer: Intermediate chunk state during generation
  - ChunkHolder: Future-based chunk state management
  - ImprovedNoiseGenerator: MC-style Perlin noise
  - OctavesNoiseGenerator: Multi-octave noise (16 octaves)
  - NoiseChunkGenerator: Reference MC 1.16.5 terrain generation
  - SimpleBiomeProvider: Biome distribution
  - ChunkWorkerPool: Async generation thread pool
  - ServerChunkManager: Central chunk coordination
- **Renderer**: In progress (Vulkan context, basic mesh generation)
- **Resource Pack System**: Complete (model/blockstate parsing, texture atlas)
- **Block Properties**: Complete (property encoding, variant mapping)
- **Tests**: 250+ tests (163 common + 27 resource + 60+ chunk generation)

## Self-Maintenance Rule

**After every major change** (new model, new page, new controller, route changes, migration changes, new test files, architectural shifts), update this CLAUDE.md file to reflect the current state. Specifically:

- Add new models/controllers/pages/routes to the relevant tables below
- Update test count if new tests are added
- Add any new gotchas or patterns to the "Gotchas & Pitfalls" section
- Update the "Current Status" section if the status changes
- Keep this file as the single source of truth for AI sessions working on this project
