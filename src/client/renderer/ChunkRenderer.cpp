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
    // 清空待上传队列
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        while (!m_pendingUploads.empty()) {
            m_pendingUploads.pop();
        }
    }

    // 清理 Fence 管理器
    m_fenceManager.destroy(m_device, m_commandPool);

    // 立即销毁所有缓冲区（渲染器销毁时 GPU 已经不再使用）
    for (auto& pair : m_chunkBuffers) {
        if (pair.second) {
            pair.second->destroy();
        }
    }
    m_chunkBuffers.clear();
    m_totalVertices = 0;
    m_totalIndices = 0;

    // 清理延迟销毁队列
    {
        std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
        for (auto& pending : m_pendingDestroys) {
            if (pending.buffer) {
                pending.buffer->destroy();
            }
        }
        m_pendingDestroys.clear();
    }

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

        // 将缓冲区移入延迟销毁队列，而不是立即销毁
        // 这样可以避免在 GPU 仍在使用缓冲区时销毁它
        {
            std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
            PendingBufferDestroy pending;
            pending.buffer = std::move(it->second);
            pending.frameIndex = m_destroyFrameCounter;
            m_pendingDestroys.push_back(std::move(pending));
        }

        m_chunkBuffers.erase(it);
    }
}

void ChunkRenderer::clearChunks() {
    // 将所有缓冲区移入延迟销毁队列
    {
        std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
        for (auto& pair : m_chunkBuffers) {
            if (pair.second && pair.second->isValid) {
                PendingBufferDestroy pending;
                pending.buffer = std::move(pair.second);
                pending.frameIndex = m_destroyFrameCounter;
                m_pendingDestroys.push_back(std::move(pending));
            }
        }
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
    // 创建纹理图集 (正方形)
    auto result = m_textureAtlas.create(m_device, m_physicalDevice, textureSize, textureSize);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to create texture atlas: " + result.error().message());
    }

    // 确保暂存缓冲区足够大
    VkDeviceSize imageSize = textureSize * textureSize * 4;  // RGBA8
    if (imageSize > m_stagingBufferSize) {
        m_stagingBufferSize = imageSize;
        auto stagingResult = m_stagingBuffer->create(m_device, m_physicalDevice, m_stagingBufferSize);
        if (!stagingResult.success()) {
            return Error(ErrorCode::InitializationFailed,
                "Failed to resize staging buffer: " + stagingResult.error().message());
        }
    }

    // 复制数据到暂存缓冲区
    auto mapResult = m_stagingBuffer->map();
    if (mapResult.failed() || !mapResult.value()) {
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }
    void* data = mapResult.value();
    std::memcpy(data, pixelData, imageSize);
    m_stagingBuffer->unmap();

    // 创建命令缓冲区进行上传
    auto cmdResult = beginSingleTimeCommands();
    if (!cmdResult.success()) {
        return cmdResult.error();
    }
    VkCommandBuffer commandBuffer = cmdResult.value();

    // 上传纹理数据 (数据已在 staging buffer 中)
    result = m_textureAtlas.upload(commandBuffer, *m_stagingBuffer);
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
        // 将旧缓冲区移入延迟销毁队列（如果有效）
        if (buffer.vertexBuffer.isValid()) {
            std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
            PendingBufferDestroy pending;
            pending.buffer = std::make_unique<ChunkGpuBuffer>();
            pending.buffer->vertexBuffer = std::move(buffer.vertexBuffer);
            pending.frameIndex = m_destroyFrameCounter;
            m_pendingDestroys.push_back(std::move(pending));
        }

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
        // 将旧缓冲区移入延迟销毁队列（如果有效）
        if (buffer.indexBuffer.isValid()) {
            std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
            PendingBufferDestroy pending;
            pending.buffer = std::make_unique<ChunkGpuBuffer>();
            pending.buffer->indexBuffer = std::move(buffer.indexBuffer);
            pending.frameIndex = m_destroyFrameCounter;
            m_pendingDestroys.push_back(std::move(pending));
        }

        auto result = buffer.indexBuffer.create(
            m_device,
            m_physicalDevice,
            static_cast<VkDeviceSize>(meshData.indices.size()));

        if (!result.success()) {
            // 注意：顶点缓冲区已经在延迟销毁队列中，不需要在这里销毁
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

// ============================================================================
// 异步 GPU 上传
// ============================================================================

void ChunkRenderer::submitMeshUpload(const ChunkId& chunkId, MeshData&& meshData) {
    PendingMeshUpload upload;
    upload.chunkId = chunkId;
    upload.meshData = std::move(meshData);
    upload.submitTime = m_uploadTimestamp++;

    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingUploads.push(std::move(upload));
}

u32 ChunkRenderer::processPendingUploads(u32 maxPerFrame) {
    // 首先检查已完成的 fence，回收资源
    m_fenceManager.cleanup(m_device, m_commandPool);

    u32 processed = 0;

    while (processed < maxPerFrame) {
        PendingMeshUpload upload;

        {
            std::lock_guard<std::mutex> lock(m_pendingMutex);
            if (m_pendingUploads.empty()) {
                break;
            }
            upload = std::move(m_pendingUploads.front());
            m_pendingUploads.pop();
        }

        // 同步上传（简化实现，后续可改为真正的异步上传）
        // TODO: 使用 fence 和命令缓冲区池实现真正的非阻塞上传
        auto result = updateChunk(upload.chunkId, upload.meshData);
        if (result.success()) {
            ++processed;
        } else {
            spdlog::warn("Failed to upload mesh for chunk ({}, {}): {}",
                         upload.chunkId.x, upload.chunkId.z, result.error().message());
        }
    }

    return processed;
}

size_t ChunkRenderer::pendingUploadCount() const {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    return m_pendingUploads.size();
}

void ChunkRenderer::processPendingDestroys(u32 framesToKeep) {
    std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);

    // 递增帧计数器
    u64 currentCounter = m_destroyFrameCounter++;

    // 销毁超过保留帧数的缓冲区
    // framesToKeep 应该 >= MAX_FRAMES_IN_FLIGHT (2)，默认使用 3 确保安全
    auto it = m_pendingDestroys.begin();
    while (it != m_pendingDestroys.end()) {
        // 使用帧计数器差值判断是否可以安全销毁
        u64 frameDiff = currentCounter >= it->frameIndex
            ? currentCounter - it->frameIndex
            : (UINT64_MAX - it->frameIndex) + currentCounter + 1;

        if (frameDiff >= framesToKeep) {
            // 安全销毁
            if (it->buffer) {
                it->buffer->destroy();
            }
            it = m_pendingDestroys.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Fence 管理器
// ============================================================================

void FenceManager::cleanup(VkDevice device, VkCommandPool commandPool) {
    for (size_t i = 0; i < inUse.size(); ++i) {
        if (inUse[i] && fences[i] != VK_NULL_HANDLE) {
            VkResult result = vkGetFenceStatus(device, fences[i]);
            if (result == VK_SUCCESS) {
                // Fence 已 signaled，可以回收
                vkDestroyFence(device, fences[i], nullptr);
                fences[i] = VK_NULL_HANDLE;

                if (commandBuffers[i] != VK_NULL_HANDLE) {
                    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffers[i]);
                    commandBuffers[i] = VK_NULL_HANDLE;
                }

                inUse[i] = false;
            }
        }
    }
}

void FenceManager::destroy(VkDevice device, VkCommandPool commandPool) {
    for (size_t i = 0; i < fences.size(); ++i) {
        if (fences[i] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &fences[i], VK_TRUE, UINT64_MAX);
            vkDestroyFence(device, fences[i], nullptr);
        }
        if (commandBuffers[i] != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffers[i]);
        }
    }
    fences.clear();
    commandBuffers.clear();
    inUse.clear();
    nextIndex = 0;
}

} // namespace mr::client
