#include "ChunkRenderer.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace mc::client {

// ============================================================================
// ChunkGpuBuffer 实现
// ============================================================================

void ChunkGpuBuffer::destroy(VkDevice device) {
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if (indexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, indexMemory, nullptr);
        indexMemory = VK_NULL_HANDLE;
    }
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexMemory, nullptr);
        vertexMemory = VK_NULL_HANDLE;
    }
    indexCount = 0;
    vertexCount = 0;
    isValid = false;
}

// ============================================================================
// ChunkTextureAtlas 实现
// ============================================================================

void ChunkTextureAtlas::destroy(VkDevice device) {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
    isValid = false;
}

TextureRegion ChunkTextureAtlas::getRegion(u32 tileX, u32 tileY) const {
    TextureRegion region;
    region.u0 = static_cast<f32>(tileX * tileSize) / static_cast<f32>(width);
    region.v0 = static_cast<f32>(tileY * tileSize) / static_cast<f32>(height);
    region.u1 = region.u0 + tileU;
    region.v1 = region.v0 + tileV;
    return region;
}

TextureRegion ChunkTextureAtlas::getRegion(u32 tileIndex) const {
    u32 tileX = tileIndex % tilesPerRow;
    u32 tileY = tileIndex / tilesPerRow;
    return getRegion(tileX, tileY);
}

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

    // 销毁所有区块缓冲区
    for (auto& pair : m_chunkBuffers) {
        if (pair.second) {
            pair.second->destroy(m_device);
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
                pending.buffer->destroy(m_device);
            }
        }
        m_pendingDestroys.clear();
    }

    // 销毁暂存缓冲区
    if (m_stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
        m_stagingBuffer = VK_NULL_HANDLE;
    }
    if (m_stagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_stagingMemory, nullptr);
        m_stagingMemory = VK_NULL_HANDLE;
    }

    // 销毁纹理图集
    m_textureAtlas.destroy(m_device);

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
}

Result<void> ChunkRenderer::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    auto memTypeResult = findMemoryType(memRequirements.memoryTypeBits, properties);
    if (memTypeResult.failed()) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return memTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return Error(ErrorCode::OperationFailed, "Failed to allocate buffer memory");
    }

    vkBindBufferMemory(m_device, buffer, memory, 0);
    return {};
}

Result<u32> ChunkRenderer::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::OperationFailed, "Failed to find suitable memory type");
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
        m_totalVertices -= it->second->vertexCount;
        m_totalIndices -= it->second->indexCount;

        // 将缓冲区移入延迟销毁队列
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

Result<void> ChunkRenderer::createChunkBuffer(
    ChunkGpuBuffer& buffer,
    const MeshData& meshData)
{
    VkDeviceSize vertexSize = static_cast<VkDeviceSize>(meshData.vertices.size() * sizeof(Vertex));
    VkDeviceSize indexSize = static_cast<VkDeviceSize>(meshData.indices.size() * sizeof(u32));

    // 如果缓冲区已存在且大小足够，重用
    bool needNewVertex = buffer.vertexBuffer == VK_NULL_HANDLE || buffer.vertexCount < meshData.vertices.size();
    bool needNewIndex = buffer.indexBuffer == VK_NULL_HANDLE || buffer.indexCount < meshData.indices.size();

    // 创建顶点缓冲区
    if (needNewVertex) {
        if (buffer.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, buffer.vertexBuffer, nullptr);
            vkFreeMemory(m_device, buffer.vertexMemory, nullptr);
            buffer.vertexBuffer = VK_NULL_HANDLE;
            buffer.vertexMemory = VK_NULL_HANDLE;
        }

        auto result = createBuffer(
            vertexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffer.vertexBuffer,
            buffer.vertexMemory);

        if (result.failed()) {
            return Error(ErrorCode::InitializationFailed,
                "Failed to create vertex buffer: " + result.error().message());
        }
    }

    // 创建索引缓冲区
    if (needNewIndex) {
        if (buffer.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, buffer.indexBuffer, nullptr);
            vkFreeMemory(m_device, buffer.indexMemory, nullptr);
            buffer.indexBuffer = VK_NULL_HANDLE;
            buffer.indexMemory = VK_NULL_HANDLE;
        }

        auto result = createBuffer(
            indexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffer.indexBuffer,
            buffer.indexMemory);

        if (result.failed()) {
            return Error(ErrorCode::InitializationFailed,
                "Failed to create index buffer: " + result.error().message());
        }
    }

    // 上传数据
    auto cmdResult = beginSingleTimeCommands();
    if (!cmdResult.success()) {
        buffer.destroy(m_device);
        return cmdResult.error();
    }
    VkCommandBuffer commandBuffer = cmdResult.value();

    // 确保暂存缓冲区足够大
    VkDeviceSize maxDataSize = std::max(vertexSize, indexSize);
    if (maxDataSize > m_stagingBufferSize || m_stagingBuffer == VK_NULL_HANDLE) {
        // 销毁旧的暂存缓冲区
        if (m_stagingBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
            vkFreeMemory(m_device, m_stagingMemory, nullptr);
        }

        m_stagingBufferSize = std::max(maxDataSize, static_cast<VkDeviceSize>(16 * 1024 * 1024));
        auto result = createBuffer(
            m_stagingBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_stagingBuffer,
            m_stagingMemory);

        if (result.failed()) {
            endSingleTimeCommands(commandBuffer);
            return Error(ErrorCode::OperationFailed, "Failed to create staging buffer");
        }
    }

    // 映射并上传顶点数据
    void* mapped;
    vkMapMemory(m_device, m_stagingMemory, 0, vertexSize, 0, &mapped);
    std::memcpy(mapped, meshData.vertices.data(), static_cast<size_t>(vertexSize));
    vkUnmapMemory(m_device, m_stagingMemory);

    // 复制顶点数据
    VkBufferCopy copyRegion{};
    copyRegion.size = vertexSize;
    vkCmdCopyBuffer(commandBuffer, m_stagingBuffer, buffer.vertexBuffer, 1, &copyRegion);

    // 映射并上传索引数据
    vkMapMemory(m_device, m_stagingMemory, 0, indexSize, 0, &mapped);
    std::memcpy(mapped, meshData.indices.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(m_device, m_stagingMemory);

    // 复制索引数据
    copyRegion.size = indexSize;
    vkCmdCopyBuffer(commandBuffer, m_stagingBuffer, buffer.indexBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);

    buffer.indexCount = static_cast<u32>(meshData.indices.size());
    buffer.vertexCount = static_cast<u32>(meshData.vertices.size());
    buffer.isValid = true;

    // 更新统计
    m_totalVertices += buffer.vertexCount;
    m_totalIndices += buffer.indexCount;

    return {};
}

Result<void> ChunkRenderer::loadTextureAtlas(
    const u8* pixelData,
    u32 textureSize,
    u32 tileSize)
{
    // 创建纹理图集
    auto result = createTextureAtlas(textureSize, textureSize);
    if (result.failed()) {
        return result;
    }

    m_textureAtlas.tileSize = tileSize;
    m_textureAtlas.tilesPerRow = textureSize / tileSize;
    m_textureAtlas.tileU = static_cast<f32>(tileSize) / static_cast<f32>(textureSize);
    m_textureAtlas.tileV = static_cast<f32>(tileSize) / static_cast<f32>(textureSize);

    // 上传纹理数据
    return uploadTextureData(pixelData, textureSize, textureSize);
}

Result<void> ChunkRenderer::createTextureAtlas(u32 width, u32 height) {
    // 销毁旧纹理
    m_textureAtlas.destroy(m_device);

    // 创建图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_textureAtlas.image) != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create texture atlas image");
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_textureAtlas.image, &memRequirements);

    auto memTypeResult = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memTypeResult.failed()) {
        m_textureAtlas.destroy(m_device);
        return memTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_textureAtlas.memory) != VK_SUCCESS) {
        m_textureAtlas.destroy(m_device);
        return Error(ErrorCode::OperationFailed, "Failed to allocate texture atlas memory");
    }

    vkBindImageMemory(m_device, m_textureAtlas.image, m_textureAtlas.memory, 0);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_textureAtlas.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_textureAtlas.imageView) != VK_SUCCESS) {
        m_textureAtlas.destroy(m_device);
        return Error(ErrorCode::OperationFailed, "Failed to create texture atlas image view");
    }

    // 创建采样器
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureAtlas.sampler) != VK_SUCCESS) {
        m_textureAtlas.destroy(m_device);
        return Error(ErrorCode::OperationFailed, "Failed to create texture atlas sampler");
    }

    m_textureAtlas.width = width;
    m_textureAtlas.height = height;
    m_textureAtlas.isValid = true;

    return {};
}

Result<void> ChunkRenderer::uploadTextureData(const u8* pixelData, u32 width, u32 height) {
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4);

    // 创建暂存缓冲区
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    auto result = createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    if (result.failed()) {
        return result;
    }

    // 映射并复制数据
    void* mapped;
    vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mapped);
    std::memcpy(mapped, pixelData, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingMemory);

    // 转换图像布局并复制
    VkCommandBuffer cmd = beginSingleTimeCommands().value();

    // 转换到传输目标布局
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_textureAtlas.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    // 复制缓冲区到图像
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
        cmd,
        stagingBuffer,
        m_textureAtlas.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region);

    // 转换到着色器只读布局
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
}

void ChunkRenderer::render(VkCommandBuffer commandBuffer, VkPipelineLayout /*pipelineLayout*/) {
    // 绑定纹理（如果有效）
    // 注意: 实际的描述符绑定需要在外部处理

    // 渲染所有区块
    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (!buffer->isValid || buffer->vertexBuffer == VK_NULL_HANDLE || buffer->indexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        VkBuffer vertexBuffers[] = { buffer->vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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

void ChunkRenderer::render(VkCommandBuffer commandBuffer, VkPipelineLayout /*pipelineLayout*/,
                           PushConstantsCallback pushConstantsCallback) {
    // 渲染所有区块
    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (!buffer->isValid || buffer->vertexBuffer == VK_NULL_HANDLE || buffer->indexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        // 调用回调设置推送常量（区块偏移）
        if (pushConstantsCallback) {
            pushConstantsCallback(buffer->chunkId);
        }

        VkBuffer vertexBuffers[] = { buffer->vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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

        // 同步上传
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
    auto it = m_pendingDestroys.begin();
    while (it != m_pendingDestroys.end()) {
        u64 frameDiff = currentCounter >= it->frameIndex
            ? currentCounter - it->frameIndex
            : (UINT64_MAX - it->frameIndex) + currentCounter + 1;

        if (frameDiff >= framesToKeep) {
            if (it->buffer) {
                it->buffer->destroy(m_device);
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

} // namespace mc::client
