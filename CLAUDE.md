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
# 注：即使在开发过程中，也要尽量使用Release构建，因为Debug运行非常慢，除非必要否则不要用。
cmake --build build --config Release

# Run tests
./build/bin/Release/mc_tests.exe

# Run server
./build/bin/Release/minecraft-server.exe --help

# Run client
./build/bin/Release/minecraft-client.exe
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `MC_BUILD_CLIENT` | ON | Build client application |
| `MC_BUILD_SERVER` | ON | Build server application |
| `MC_BUILD_TESTS` | ON | Build unit tests |
| `MC_ENABLE_SANITIZERS` | OFF | Enable sanitizers for debug |
| `MC_ENABLE_VULKAN_VALIDATION` | ON | Enable Vulkan validation layers |
| `MC_ENABLE_TRACING` | OFF | Enable Perfetto performance tracing |

## Architecture

```
src/
├── common/          # Shared code between client and server
│   ├── core/        # Types, Result, Constants
│   ├── math/        # Vector3, MathUtils, PerlinNoise, SimplexNoise
│   ├── network/     # Packet, PacketSerializer, IServerConnection
│   ├── world/       # World generation and chunk management
│   │   ├── block/   # Block system (Block, BlockState, BlockRegistry)
│   │   ├── chunk/   # Chunk data structures
│   │   │   ├── ChunkData.hpp       # Final chunk data
│   │   │   ├── ChunkPos.hpp        # Chunk position
│   │   │   ├── ChunkStatus.hpp     # Generation stages
│   │   │   ├── ChunkPrimer.hpp     # Intermediate chunk state
│   │   │   ├── ChunkHolder.hpp     # Chunk state management
│   │   │   ├── IChunk.hpp          # Chunk interface
│   │   │   └── ChunkLoadTicketManager.hpp
│   │   ├── time/    # Game time system
│   │   │   └── GameTime.hpp        # Day/night cycle
│   │   └── gen/     # World generation
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
│   ├── application/ # ServerApplication, IntegratedServer
│   ├── core/        # ServerCore module (NEW)
│   │   ├── ServerCore.hpp/cpp        # Facade class coordinating all managers
│   │   ├── ServerCoreConfig.hpp      # Configuration struct
│   │   ├── ServerPlayerData.hpp      # Player data structure
│   │   ├── PlayerManager.hpp/cpp     # Player lifecycle management
│   │   ├── ConnectionManager.hpp/cpp # Network communication
│   │   ├── TimeManager.hpp/cpp       # Game time, tick count, day cycle
│   │   ├── TeleportManager.hpp/cpp   # Teleport request/confirmation
│   │   ├── KeepAliveManager.hpp/cpp  # Heartbeat, ping, timeout
│   │   ├── PositionTracker.hpp/cpp   # Player position, chunk subscription
│   │   └── PacketHandler.hpp/cpp     # Unified packet handling
│   ├── network/     # TcpServer, TcpSession, TcpConnection
│   ├── command/     # Command system
│   │   ├── CommandRegistry.hpp       # Command registration
│   │   ├── ServerCommandSource.hpp   # Command execution context
│   │   └── commands/                 # Command implementations
│   ├── menu/        # Container menu system
│   │   └── CraftingMenu.hpp/cpp
│   ├── player/      # Server player
│   │   └── ServerPlayer.hpp/cpp
│   └── world/       # ServerWorld
│       ├── ServerWorld.hpp
│       ├── ServerChunkManager.hpp  # Chunk manager
│       ├── ChunkWorkerPool.hpp     # Async generation
│       ├── spawn/                  # Mob spawning
│       │   ├── NaturalSpawner.hpp
│       │   └── SpawnConditions.hpp
│       └── entity/EntityTracker.hpp
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
├── common/
│   └── perfetto/      # Perfetto 性能追踪
│       ├── PerfettoConfig.hpp      # 编译时配置开关
│       ├── TraceCategories.hpp     # 追踪分类定义
│       ├── TraceCategories.cpp     # 分类静态存储
│       ├── PerfettoManager.hpp     # 单例管理器
│       ├── PerfettoManager.cpp     # 管理器实现
│       └── TraceEvents.hpp         # 便捷追踪宏
└── modding/         # JavaScript mod system (future)
```

## Key Types

All types are in namespace `mc` (client types in `mc::client`, server types in `mc::server`):

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

### MC Version Compatibility
The system supports both MC 1.12 and MC 1.13+ resource packs with automatic texture path and name conversion:
- **Path compatibility**: `textures/block/` ↔ `textures/blocks/` automatic conversion
- **Name variants**: Handles naming differences like `white_wool` ↔ `wool_colored_white`, `oak_planks` ↔ `planks_oak`
- **Stone variants**: `granite` ↔ `stone_granite`, `andesite` ↔ `stone_andesite`, etc.
- **Grass block**: `grass_block_top` ↔ `grass_top`, `grass_block_side` ↔ `grass_side`
- **Sandstone**: `cut_sandstone` ↔ `sandstone_carved`, `chiseled_sandstone` ↔ `sandstone_smooth`

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
- `textures/blocks/*.png` - Block textures (MC 1.12)
- `textures/block/*.png` - Block textures (MC 1.13+)

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

- **Namespaces**: lowercase (`mc`, `mc::client`, `mc::server`)
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

## Random Module

The project provides a unified random number generation module with multiple algorithm implementations:

### Directory Structure
```
src/common/math/random/
├── IRandom.hpp/cpp              # Random interface with MC-style methods
├── Mt19937Random.hpp/cpp        # Mersenne Twister (default, highest compatibility)
├── Xoroshiro128ppRandom.hpp/cpp # xoroshiro128++ (small state, high performance)
├── Xoshiro256ppRandom.hpp/cpp   # xoshiro256++ (high quality)
├── LcgRandom.hpp/cpp            # Linear Congruential (minimal state)
├── Random.hpp                   # Unified wrapper (algorithm selection via macro)
├── UniformIntDistribution.hpp/cpp   # Integer distribution wrapper
└── UniformRealDistribution.hpp/cpp  # Float distribution wrapper
```

注意：Xoroshiro128ppRandom这些类是内部使用的，禁止外部引用。外部统一使用Random类进行随机数生成。

### Usage
```cpp
#include "math/random/Random.hpp"

mc::math::Random rng(seed);
i32 value = rng.nextInt(100);   // [0, 100)
i32 range = rng.nextInt(10, 20); // [10, 20]
f32 f = rng.nextFloat();         // [0.0, 1.0)
f64 d = rng.nextDouble();        // [0.0, 1.0)
bool b = rng.nextBoolean();      // true/false
f32 g = rng.nextGaussian();      // Standard normal distribution
```

### Algorithm Selection
Compile-time algorithm selection via macro in `Random.hpp`:
- `MC_RANDOM_XOROSHIRO128PP` - xoroshiro128++ (small state, high performance)
- `MC_RANDOM_XOSHIRO256PP` - xoshiro256++ (high quality)
- `MC_RANDOM_LCG` - Linear Congruential (minimal state)
- Default: Mersenne Twister (highest compatibility)

### Interface Methods
All random implementations provide MC-style methods:
- `setSeed(u64)` - Set random seed
- `nextU64()` - 64-bit random integer
- `nextU32()` - 32-bit random integer
- `nextInt()` - Random i32
- `nextInt(bound)` - Random i32 in [0, bound)
- `nextInt(min, max)` - Random i32 in [min, max]
- `nextBoolean()` - Random bool
- `nextFloat()` - Random f32 in [0.0, 1.0)
- `nextFloat(min, max)` - Random f32 in [min, max)
- `nextDouble()` - Random f64 in [0.0, 1.0)
- `nextGaussian(mean, stddev)` - Normal distribution
- `skip(count)` - Skip ahead in sequence

## 注意，你可随时访问mc java版本的源码来供自己参考：`D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft`，这很重要，因为当前项目是一个复刻项目，目标是完全使用cpp尽可能一致地复刻java版mc的游戏体验，并在存档、数据包等层面上尽可能兼容和复用现有java版minecraft生态。务必要有清晰、优雅、能让人赏心悦目的目录结构，不要把很多文件全部堆在一个目录下，要划分好细分的子目录。

## 你需要先阅读readme文件了解怎么构建项目

## 需要断言+单测来保证代码质量；每个方法前都要附上doc注释说明方法的用法和注意事项（容易踩坑的地方）

## 务必要有清晰、优雅、能让人赏心悦目的目录结构，不要把很多文件全部堆在一个目录下，要划分好细分的子目录！

## 编译过程中遇到的warning你也要一并解决

## 当单测不通过的时候，首先应该反思待测代码的问题，而不是急于修改测试代码；测试覆盖率必须95%以上，并坚持“测试即契约”

## 注意：你必须完整实现所有任务，不允许暂时跳过或留任何todo。你被给予了充足时间做全部任务，放心。

## 需要使用命名空间隔离各个子系统的标识符。下面是最佳实践：

```cpp

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性修改器操作类型
 *
 * 定义属性修改器如何影响基础值
 *
 * 参考 MC 1.16.5 Operation
 */
enum class Operation : u8 {
    // ...
}}}}

```

## 代码复用与质量规范

### 使用现有工具类

1. **Vulkan工具类**: 优先使用 `VulkanBuffer`, `VertexBuffer`, `IndexBuffer`, `StagingBuffer` 而非直接调用 Vulkan API
2. **数学工具**: 使用 `math::toRadians()`, `math::toDegrees()`, `math::clamp()`, `math::lerp()` 等，避免内联魔法数如 `3.14159265f / 180.0f`
3. **Result API**: 使用 `Error(ErrorCode, message)` 返回错误，使用隐式转换 `return value` 返回成功值，而非 `Result<T>::ok(value)` 或 `Result<T>::error(...)`

### 避免重复代码

- 如果类似功能在多处出现（如 `beginSingleTimeCommands`/`endSingleTimeCommands`），考虑提取到共享工具类
- 如果同一模式重复（如缓冲区创建），使用模板或辅助函数

### 参数设计

- 函数参数超过5个时，考虑使用配置结构体
- 使用枚举或类型安全的标识符替代原始字符串比较

### Shader 路径解析

使用 `resolveShaderPath()` 工具函数来解析 shader 文件路径，它会自动搜索多个可能的位置：

```cpp
#include "renderer/ShaderPath.hpp"

// 使用方式
const auto vertPath = resolveShaderPath("entity.vert.spv");
const auto fragPath = resolveShaderPath("entity.frag.spv");
if (vertPath.empty() || fragPath.empty()) {
    return Error(ErrorCode::FileNotFound, "Failed to resolve shader binaries");
}
config.vertexShaderPath = vertPath.string();
config.fragmentShaderPath = fragPath.string();
```

搜索顺序包括：
1. `当前目录/build/shaders/`
2. `当前目录/shaders/`
3. `当前目录/bin/shaders/`
4. 向上级目录递归搜索

### Vulkan单次命令模式

当需要在多个地方执行单次Vulkan命令时，使用以下模式：
```cpp
VkCommandBuffer beginSingleTimeCommands();
void endSingleTimeCommands(VkCommandBuffer cmd);
```
这两个方法应该在需要时添加到各自的类中，或考虑提取到共享的Vulkan工具类。

### stb_image 使用

整个项目只需在一处定义 `STB_IMAGE_IMPLEMENTATION`（目前已在 `TextureAtlasBuilder.cpp`），其他文件只需 `#include <stb_image.h>`。

## Current Status

- **Core**: Complete (types, math, error handling)
- **Network**: Complete (TCP server, packet serialization, LocalConnection for integrated server)
- **Server Core Module**: Complete (NEW - Modular refactoring)
  - ServerCore: Facade class coordinating all managers
  - PlayerManager: Player lifecycle, session mapping, chunk sync
  - ConnectionManager: Packet sending, broadcasting, disconnection
  - TimeManager: Game time, tick count, day cycle
  - TeleportManager: Teleport request/confirmation
  - KeepAliveManager: Heartbeat, ping calculation, timeout detection
  - PositionTracker: Position updates, chunk subscription
  - PacketHandler: Unified packet handling with callbacks
- **World**: Complete (chunk storage, terrain generation)
  - ChunkStatus: Generation stages (EMPTY → BIOMES → NOISE → SURFACE → CARVERS → FEATURES → HEIGHTMAPS → FULL)
  - ChunkPrimer: Intermediate chunk state during generation
  - ChunkHolder: Future-based chunk state management
  - ImprovedNoiseGenerator: MC-style Perlin noise
  - OctavesNoiseGenerator: Multi-octave noise (16 octaves)
  - NoiseChunkGenerator: Reference MC 1.16.5 terrain generation
  - LayerBiomeProvider: Layer-based biome distribution (MC 1.16.5)
  - ChunkWorkerPool: Async generation thread pool
  - ServerChunkManager: Central chunk coordination
- **Renderer**: In progress (Vulkan context, basic mesh generation, texture atlas with MC 1.12/1.13+ compatibility)
- **Resource Pack System**: Complete (model/blockstate parsing, texture atlas, MC version compatibility)
- **Block Properties**: Complete (property encoding, variant mapping)
- **Performance Tracing**: Complete (NEW - Perfetto integration)
  - PerfettoConfig.hpp: Compile-time configuration switches
  - TraceCategories.hpp/cpp: Category definitions for organized filtering
  - PerfettoManager: Singleton manager for tracing lifecycle
  - TraceEvents.hpp: Convenient macros (MC_TRACE_EVENT, MC_TRACE_COUNTER, etc.)
  - Tests: 2 tests (disabled mode) / 29 tests (enabled mode)
- **Tests**: 1251+ tests passing

## Self-Maintenance Rule

**After every major change** (new model, new page, new controller, route changes, migration changes, new test files, architectural shifts), update this CLAUDE.md file to reflect the current state. Specifically:

- Add new models/controllers/pages/routes to the relevant tables below
- Update test count if new tests are added
- Add any new gotchas or patterns to the "Gotchas & Pitfalls" section
- Update the "Current Status" section if the status changes
- Keep this file as the single source of truth for AI sessions working on this project
