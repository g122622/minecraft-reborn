#include "ClientWorld.hpp"
#include "../../common/renderer/ChunkMesher.hpp"
#include "../../common/world/WorldConstants.hpp"
#include "../../common/network/ChunkSync.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace mr::client {

using namespace mr::world;

ClientWorld::ClientWorld() = default;

ClientWorld::~ClientWorld() {
    destroy();
}

Result<void> ClientWorld::initialize(u64 seed) {
    m_seed = seed;

    // 网络模式下不创建本地地形生成器
    if (!m_networkMode) {
        m_terrainGenerator = TerrainGenFactory::createStandard(seed);
    }

    spdlog::info("ClientWorld initialized with seed: {} (networkMode: {})", seed, m_networkMode);
    return Result<void>::ok();
}

void ClientWorld::destroy() {
    m_chunks.clear();
    m_terrainGenerator.reset();

    while (!m_loadQueue.empty()) {
        m_loadQueue.pop();
    }
    m_queuedChunks.clear();

    spdlog::info("ClientWorld destroyed");
}

void ClientWorld::update(const glm::vec3& cameraPosition, i32 renderDistance) {
    m_renderDistance = renderDistance;

    // 网络模式下，区块加载由服务端控制，客户端只处理卸载
    if (!m_networkMode) {
        loadChunksInRange(cameraPosition, renderDistance);
        processLoadQueue();
    }
    unloadChunksOutOfRange(cameraPosition, renderDistance + 2); // 多保留2个区块的缓冲
}

ClientChunk* ClientWorld::getChunk(const ChunkId& id) {
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const ClientChunk* ClientWorld::getChunk(const ChunkId& id) const {
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

BlockState ClientWorld::getBlock(i32 x, i32 y, i32 z) const {
    // 检查Y范围
    if (!isValidY(y)) {
        return BlockState(BlockId::Air);
    }

    // 获取区块
    i32 chunkX = toChunkCoord(x);
    i32 chunkZ = toChunkCoord(z);
    ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return BlockState(BlockId::Air);
    }

    // 转换为本地坐标
    i32 localX = toLocalCoord(x);
    i32 localZ = toLocalCoord(z);

    return chunk->data->getBlock(localX, y, localZ);
}

void ClientWorld::setBlock(i32 x, i32 y, i32 z, BlockState block) {
    // 检查Y范围
    if (!isValidY(y)) {
        return;
    }

    // 获取区块
    i32 chunkX = toChunkCoord(x);
    i32 chunkZ = toChunkCoord(z);
    ChunkId id(chunkX, chunkZ);

    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return;
    }

    // 转换为本地坐标
    i32 localX = toLocalCoord(x);
    i32 localZ = toLocalCoord(z);

    chunk->data->setBlock(localX, y, localZ, block);
    chunk->needsMeshUpdate = true;
    chunk->data->setDirty(true);

    // 标记相邻区块也需要更新（如果方块在边界）
    if (localX == 0) markChunkDirty(ChunkId(chunkX - 1, chunkZ));
    if (localX == CHUNK_WIDTH - 1) markChunkDirty(ChunkId(chunkX + 1, chunkZ));
    if (localZ == 0) markChunkDirty(ChunkId(chunkX, chunkZ - 1));
    if (localZ == CHUNK_WIDTH - 1) markChunkDirty(ChunkId(chunkX, chunkZ + 1));
}

void ClientWorld::forEachChunk(std::function<void(const ChunkId&, ClientChunk&)> func) {
    for (auto& [id, chunk] : m_chunks) {
        func(id, *chunk);
    }
}

void ClientWorld::forEachDirtyMesh(std::function<void(const ChunkId&, ClientChunk&)> func) {
    for (auto& [id, chunk] : m_chunks) {
        if (chunk && chunk->needsMeshUpdate && chunk->isLoaded) {
            func(id, *chunk);
        }
    }
}

void ClientWorld::getChunksInRange(const glm::vec3& position, i32 range,
                                    std::vector<ChunkId>& outChunks) const {
    i32 centerChunkX = toChunkCoord(static_cast<i32>(position.x));
    i32 centerChunkZ = toChunkCoord(static_cast<i32>(position.z));

    outChunks.clear();
    outChunks.reserve(static_cast<size_t>((2 * range + 1) * (2 * range + 1)));

    for (i32 dx = -range; dx <= range; ++dx) {
        for (i32 dz = -range; dz <= range; ++dz) {
            // 使用圆形范围
            f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));
            if (dist <= static_cast<f32>(range)) {
                outChunks.emplace_back(centerChunkX + dx, centerChunkZ + dz);
            }
        }
    }
}

void ClientWorld::loadChunk(const ChunkId& id) {
    // 检查是否已加载
    if (m_chunks.find(id) != m_chunks.end()) {
        return;
    }

    // 添加到加载队列
    if (m_queuedChunks.find(id) == m_queuedChunks.end()) {
        m_loadQueue.push({id, ChunkLoadPriority::Normal});
        m_queuedChunks.insert(id);
    }
}

void ClientWorld::unloadChunk(const ChunkId& id) {
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        m_chunks.erase(it);
        m_chunksUnloaded++;
        spdlog::debug("Unloaded chunk ({}, {})", id.x, id.z);
    }
}

void ClientWorld::rebuildChunkMesh(const ChunkId& id) {
    ClientChunk* chunk = getChunk(id);
    if (chunk && chunk->data) {
        rebuildMesh(*chunk);
    }
}

void ClientWorld::markChunkDirty(const ChunkId& id) {
    ClientChunk* chunk = getChunk(id);
    if (chunk) {
        chunk->needsMeshUpdate = true;
    }
}

void ClientWorld::loadChunksInRange(const glm::vec3& position, i32 range) {
    std::vector<ChunkId> chunksToLoad;
    getChunksInRange(position, range, chunksToLoad);

    for (const auto& id : chunksToLoad) {
        // 检查是否已加载
        if (m_chunks.find(id) != m_chunks.end()) {
            continue;
        }

        // 添加到加载队列
        if (m_queuedChunks.find(id) == m_queuedChunks.end()) {
            world::ChunkLoadPriority priority = calculatePriority(id, position);
            m_loadQueue.push({id, priority});
            m_queuedChunks.insert(id);
        }
    }
}

void ClientWorld::unloadChunksOutOfRange(const glm::vec3& position, i32 range) {
    i32 centerChunkX = toChunkCoord(static_cast<i32>(position.x));
    i32 centerChunkZ = toChunkCoord(static_cast<i32>(position.z));

    std::vector<ChunkId> toRemove;
    for (const auto& [id, chunk] : m_chunks) {
        i32 dx = id.x - centerChunkX;
        i32 dz = id.z - centerChunkZ;
        f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));

        if (dist > static_cast<f32>(range)) {
            toRemove.push_back(id);
        }
    }

    for (const auto& id : toRemove) {
        unloadChunk(id);
    }
}

void ClientWorld::processLoadQueue() {
    i32 loaded = 0;
    while (!m_loadQueue.empty() && loaded < m_maxChunksPerFrame) {
        ChunkLoadRequest request = m_loadQueue.top();
        m_loadQueue.pop();
        m_queuedChunks.erase(request.chunkId);

        // 跳过已加载的区块
        if (m_chunks.find(request.chunkId) != m_chunks.end()) {
            continue;
        }

        // 创建新区块
        auto chunk = std::make_unique<ClientChunk>();
        chunk->chunkId = request.chunkId;
        chunk->data = std::make_unique<ChunkData>(request.chunkId.x, request.chunkId.z);

        // 生成地形
        generateChunk(*chunk);

        // 构建网格
        rebuildMesh(*chunk);

        chunk->isLoaded = true;
        chunk->needsMeshUpdate = true;  // 标记需要上传到GPU

        m_chunks[request.chunkId] = std::move(chunk);
        m_chunksLoaded++;
        loaded++;
    }
}

void ClientWorld::generateChunk(ClientChunk& chunk) {
    if (chunk.data && m_terrainGenerator) {
        m_terrainGenerator->generateChunk(*chunk.data);
        chunk.isGenerating = false;
    }
}

void ClientWorld::rebuildMesh(ClientChunk& chunk) {
    if (!chunk.data) return;

    // 获取相邻区块（用于边界面的剔除）
    const ChunkData* neighbors[6] = {nullptr};

    // -X
    auto neighborXNeg = getChunk(ChunkId(chunk.chunkId.x - 1, chunk.chunkId.z));
    if (neighborXNeg && neighborXNeg->data) neighbors[0] = neighborXNeg->data.get();

    // +X
    auto neighborXPos = getChunk(ChunkId(chunk.chunkId.x + 1, chunk.chunkId.z));
    if (neighborXPos && neighborXPos->data) neighbors[1] = neighborXPos->data.get();

    // -Z
    auto neighborZNeg = getChunk(ChunkId(chunk.chunkId.x, chunk.chunkId.z - 1));
    if (neighborZNeg && neighborZNeg->data) neighbors[2] = neighborZNeg->data.get();

    // +Z
    auto neighborZPos = getChunk(ChunkId(chunk.chunkId.x, chunk.chunkId.z + 1));
    if (neighborZPos && neighborZPos->data) neighbors[3] = neighborZPos->data.get();

    // -Y 和 +Y 暂时不考虑（多区块高度）

    // 生成网格
    ChunkMesher::generateMesh(*chunk.data, chunk.solidMesh, neighbors);

    chunk.needsMeshUpdate = false;
}

void ClientWorld::getNeighborChunks(const ChunkId& id, const ChunkData* neighbors[6]) {
    // -X
    auto neighbor = getChunk(ChunkId(id.x - 1, id.z));
    neighbors[0] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    // +X
    neighbor = getChunk(ChunkId(id.x + 1, id.z));
    neighbors[1] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    // -Z
    neighbor = getChunk(ChunkId(id.x, id.z - 1));
    neighbors[2] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    // +Z
    neighbor = getChunk(ChunkId(id.x, id.z + 1));
    neighbors[3] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    // -Y 和 +Y
    neighbors[4] = nullptr;
    neighbors[5] = nullptr;
}

world::ChunkLoadPriority ClientWorld::calculatePriority(const ChunkId& id, const glm::vec3& cameraPos) const {
    i32 centerChunkX = toChunkCoord(static_cast<i32>(cameraPos.x));
    i32 centerChunkZ = toChunkCoord(static_cast<i32>(cameraPos.z));

    i32 dx = id.x - centerChunkX;
    i32 dz = id.z - centerChunkZ;
    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));

    if (dist < 1.0f) {
        return world::ChunkLoadPriority::Critical;
    } else if (dist < 3.0f) {
        return world::ChunkLoadPriority::High;
    } else if (dist < static_cast<f32>(m_renderDistance) * 0.5f) {
        return world::ChunkLoadPriority::Normal;
    } else {
        return world::ChunkLoadPriority::Low;
    }
}

void ClientWorld::onChunkData(ChunkCoord x, ChunkCoord z, std::vector<u8>&& data) {
    ChunkId id(x, z);

    // 反序列化区块数据
    auto result = network::ChunkSerializer::deserializeChunk(x, z, data);
    if (result.failed()) {
        spdlog::error("Failed to deserialize chunk ({}, {}): {}", x, z, result.error().message());
        return;
    }
    auto chunkData = std::move(result.value());

    auto chunk = std::make_unique<ClientChunk>();
    chunk->chunkId = id;
    chunk->data = std::move(chunkData);
    chunk->isLoaded = true;
    chunk->needsMeshUpdate = true;

    // 重建网格
    rebuildMesh(*chunk);

    m_chunks[id] = std::move(chunk);
    m_chunksLoaded++;
    spdlog::debug("Chunk ({}, {}) loaded", x, z);
}

void ClientWorld::onChunkUnload(ChunkCoord x, ChunkCoord z) {
    ChunkId id(x, z);
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        m_chunks.erase(it);
        m_chunksUnloaded++;
        spdlog::debug("Unloaded chunk ({}, {}) from server", x, z);
    }
}

} // namespace mr::client
