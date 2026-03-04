#include "ChunkRenderer.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace mr::client {

// ============================================================================
// ChunkRenderer 实现
// ============================================================================

ChunkRenderer::ChunkRenderer() = default;

ChunkRenderer::~ChunkRenderer() {
    destroy();
}

Result<void> ChunkRenderer::initialize(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 maxChunks)
{
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_maxChunks = maxChunks;

    // 创建暂存缓冲区
    m_stagingBuffer = std::make_unique<StagingBuffer>();
    auto result = m_stagingBuffer->create(device, physicalDevice, m_stagingBufferSize);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to create staging buffer: " + result.error().message());
    }

    spdlog::info("ChunkRenderer initialized (max chunks: {})", maxChunks);
    return {};
}

void ChunkRenderer::destroy() {
    clearChunks();

    if (m_stagingBuffer) {
        m_stagingBuffer->destroy();
        m_stagingBuffer.reset();
    }

    m_textureAtlas.destroy();

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
}

Result<void> ChunkRenderer::updateChunk(
    const ChunkId& chunkId,
    const MeshData& meshData)
{
    if (meshData.empty()) {
        removeChunk(chunkId);
        return {};
    }

    // 查找或创建缓冲区
    u64 id = chunkId.toId();
    auto it = m_chunkBuffers.find(id);

    if (it == m_chunkBuffers.end()) {
        if (m_chunkBuffers.size() >= m_maxChunks) {
            return Error(ErrorCode::CapacityExceeded, "Maximum chunk count reached");
        }

        auto buffer = std::make_unique<ChunkGpuBuffer>();
        buffer->chunkId = chunkId;

        auto result = createChunkBuffer(*buffer, meshData);
        if (!result.success()) {
            return result;
        }

        m_chunkBuffers[id] = std::move(buffer);
    } else {
        // 更新现有缓冲区
        auto result = createChunkBuffer(*it->second, meshData);
        if (!result.success()) {
            return result;
        }
    }

    return {};
}

void ChunkRenderer::removeChunk(const ChunkId& chunkId) {
    u64 id = chunkId.toId();
    auto it = m_chunkBuffers.find(id);

    if (it != m_chunkBuffers.end()) {
        m_totalVertices -= static_cast<u32>(it->second->vertexBuffer.size() / sizeof(Vertex));
        m_totalIndices -= it->second->indexCount;
        it->second->destroy();
        m_chunkBuffers.erase(it);
    }
}

void ChunkRenderer::clearChunks() {
    for (auto& pair : m_chunkBuffers) {
        pair.second->destroy();
    }
    m_chunkBuffers.clear();
    m_totalVertices = 0;
    m_totalIndices = 0;
}

Result<void> ChunkRenderer::loadTextureAtlas(
    const u8* pixelData,
    u32 textureSize,
    u32 tileSize)
{
    // 创建纹理图集
    auto result = m_textureAtlas.create(m_device, m_physicalDevice, textureSize, tileSize);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to create texture atlas: " + result.error().message());
    }

    // 创建命令缓冲区进行上传
    auto cmdResult = beginSingleTimeCommands();
    if (!cmdResult.success()) {
        return cmdResult.error();
    }
    VkCommandBuffer commandBuffer = cmdResult.value();

    // 上传纹理数据
    result = m_textureAtlas.upload(commandBuffer, *m_stagingBuffer, pixelData);
    if (!result.success()) {
        endSingleTimeCommands(commandBuffer);
        return Error(ErrorCode::OperationFailed,
            "Failed to upload texture atlas: " + result.error().message());
    }

    endSingleTimeCommands(commandBuffer);

    spdlog::info("Texture atlas loaded: {}x{} (tile size: {})",
        textureSize, textureSize, tileSize);
    return {};
}

void ChunkRenderer::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    // 绑定纹理
    if (m_textureAtlas.isValid()) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureAtlas.texture().imageView();
        imageInfo.sampler = m_textureAtlas.texture().sampler();

        // 注意: 这里需要配合 descriptor set 使用
        // 实际使用时需要在渲染管线中设置 descriptor
    }

    // 渲染所有区块（不设置偏移，由外部设置）
    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (!buffer->isValid) {
            continue;
        }

        buffer->vertexBuffer.bind(commandBuffer);
        buffer->indexBuffer.bind(commandBuffer);

        vkCmdDrawIndexed(
            commandBuffer,
            buffer->indexCount,
            1,  // instance count
            0,  // first index
            0,  // vertex offset
            0   // first instance
        );
    }
}

void ChunkRenderer::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                           PushConstantsCallback pushConstantsCallback) {
    // 绑定纹理
    if (m_textureAtlas.isValid()) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureAtlas.texture().imageView();
        imageInfo.sampler = m_textureAtlas.texture().sampler();
    }

    // 渲染所有区块
    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (!buffer->isValid) {
            continue;
        }

        // 调用回调设置推送常量（区块偏移）
        if (pushConstantsCallback) {
            pushConstantsCallback(buffer->chunkId);
        }

        buffer->vertexBuffer.bind(commandBuffer);
        buffer->indexBuffer.bind(commandBuffer);

        vkCmdDrawIndexed(
            commandBuffer,
            buffer->indexCount,
            1,  // instance count
            0,  // first index
            0,  // vertex offset
            0   // first instance
        );
    }
}

Result<VkCommandBuffer> ChunkRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to allocate command buffer");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
        return Error(ErrorCode::OperationFailed, "Failed to begin command buffer");
    }

    return commandBuffer;
}

void ChunkRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

Result<void> ChunkRenderer::createChunkBuffer(
    ChunkGpuBuffer& buffer,
    const MeshData& meshData)
{
    VkDeviceSize vertexSize = static_cast<VkDeviceSize>(meshData.vertices.size() * sizeof(Vertex));
    VkDeviceSize indexSize = static_cast<VkDeviceSize>(meshData.indices.size() * sizeof(u32));

    // 如果缓冲区已存在且大小足够，重用；否则重建
    bool needNewVertex = !buffer.vertexBuffer.isValid() || buffer.vertexBuffer.size() < vertexSize;
    bool needNewIndex = !buffer.indexBuffer.isValid() || buffer.indexBuffer.size() < indexSize;

    // 创建顶点缓冲区（设备本地内存）
    if (needNewVertex) {
        buffer.vertexBuffer.destroy();
        auto result = buffer.vertexBuffer.create(
            m_device,
            m_physicalDevice,
            static_cast<VkDeviceSize>(meshData.vertices.size()),
            sizeof(Vertex));

        if (!result.success()) {
            return Error(ErrorCode::InitializationFailed,
                "Failed to create vertex buffer: " + result.error().message());
        }
    }

    // 创建索引缓冲区（设备本地内存）
    if (needNewIndex) {
        buffer.indexBuffer.destroy();
        auto result = buffer.indexBuffer.create(
            m_device,
            m_physicalDevice,
            static_cast<VkDeviceSize>(meshData.indices.size()));

        if (!result.success()) {
            buffer.vertexBuffer.destroy();
            return Error(ErrorCode::InitializationFailed,
                "Failed to create index buffer: " + result.error().message());
        }
    }

    // 上传数据
    auto cmdResult = beginSingleTimeCommands();
    if (!cmdResult.success()) {
        buffer.destroy();
        return cmdResult.error();
    }
    VkCommandBuffer commandBuffer = cmdResult.value();

    // 使用暂存缓冲区上传顶点数据
    VkDeviceSize maxDataSize = std::max(vertexSize, indexSize);
    if (maxDataSize > m_stagingBufferSize) {
        m_stagingBufferSize = maxDataSize;
        auto result = m_stagingBuffer->create(m_device, m_physicalDevice, m_stagingBufferSize);
        if (!result.success()) {
            endSingleTimeCommands(commandBuffer);
            return Error(ErrorCode::OperationFailed, "Failed to resize staging buffer");
        }
    }

    // 上传顶点数据
    void* mapped = m_stagingBuffer->map().value();
    if (!mapped) {
        endSingleTimeCommands(commandBuffer);
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }
    std::memcpy(mapped, meshData.vertices.data(), static_cast<size_t>(vertexSize));
    m_stagingBuffer->unmap();

    // 复制顶点数据到设备本地缓冲区
    m_stagingBuffer->copyTo(commandBuffer, buffer.vertexBuffer, vertexSize);

    // 上传索引数据
    mapped = m_stagingBuffer->map().value();
    if (!mapped) {
        endSingleTimeCommands(commandBuffer);
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }
    std::memcpy(mapped, meshData.indices.data(), static_cast<size_t>(indexSize));
    m_stagingBuffer->unmap();

    // 复制索引数据到设备本地缓冲区
    m_stagingBuffer->copyTo(commandBuffer, buffer.indexBuffer, indexSize);

    endSingleTimeCommands(commandBuffer);

    buffer.indexCount = static_cast<u32>(meshData.indices.size());
    buffer.isValid = true;

    // 更新统计
    m_totalVertices += static_cast<u32>(meshData.vertices.size());
    m_totalIndices += buffer.indexCount;

    return {};
}

Result<void> ChunkRenderer::uploadBufferData(
    VulkanBuffer& dstBuffer,
    const void* data,
    VkDeviceSize size)
{
    // 检查暂存缓冲区大小
    if (size > m_stagingBufferSize) {
        // 需要更大的暂存缓冲区
        m_stagingBufferSize = size;
        auto result = m_stagingBuffer->create(m_device, m_physicalDevice, m_stagingBufferSize);
        if (!result.success()) {
            return Error(ErrorCode::OperationFailed,
                "Failed to resize staging buffer: " + result.error().message());
        }
    }

    // 复制数据到暂存缓冲区
    void* mapped = m_stagingBuffer->map().value();
    if (!mapped) {
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }

    std::memcpy(mapped, data, static_cast<size_t>(size));
    m_stagingBuffer->unmap();

    // 获取命令缓冲区用于复制
    // 注意: 这里假设外部已经开始了命令缓冲区
    // 实际使用时可能需要调整接口

    return {};
}

} // namespace mr::client
