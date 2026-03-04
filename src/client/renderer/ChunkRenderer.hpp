#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "../../common/renderer/MeshTypes.hpp"
#include "../../common/renderer/ChunkMesher.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <memory>
#include <vector>

namespace mr::client {

// 区块GPU缓冲区
struct ChunkGpuBuffer {
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    u32 indexCount = 0;
    ChunkId chunkId{0, 0};
    bool isValid = false;

    void destroy() {
        vertexBuffer.destroy();
        indexBuffer.destroy();
        indexCount = 0;
        isValid = false;
    }
};

// 区块渲染器
class ChunkRenderer {
public:
    ChunkRenderer();
    ~ChunkRenderer();

    // 禁止拷贝
    ChunkRenderer(const ChunkRenderer&) = delete;
    ChunkRenderer& operator=(const ChunkRenderer&) = delete;

    // 初始化
    [[nodiscard]] Result<void> initialize(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        u32 maxChunks = 1024);

    void destroy();

    // 区块管理
    [[nodiscard]] Result<void> updateChunk(
        const ChunkId& chunkId,
        const MeshData& meshData);

    void removeChunk(const ChunkId& chunkId);

    void clearChunks();

    // 纹理图集
    [[nodiscard]] Result<void> loadTextureAtlas(
        const u8* pixelData,
        u32 textureSize,
        u32 tileSize);

    VulkanTextureAtlas& textureAtlas() { return m_textureAtlas; }
    const VulkanTextureAtlas& textureAtlas() const { return m_textureAtlas; }

    // 渲染
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    // 渲染（带推送常量回调）
    // pushConstantsCallback: 设置推送常量的回调函数，参数是 chunkId
    using PushConstantsCallback = std::function<void(const ChunkId&)>;
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                PushConstantsCallback pushConstantsCallback);

    // 统计
    u32 chunkCount() const { return static_cast<u32>(m_chunkBuffers.size()); }
    u32 totalVertexCount() const { return m_totalVertices; }
    u32 totalIndexCount() const { return m_totalIndices; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    // 区块缓冲区
    std::unordered_map<u64, std::unique_ptr<ChunkGpuBuffer>> m_chunkBuffers;
    u32 m_maxChunks = 1024;

    // 统计
    u32 m_totalVertices = 0;
    u32 m_totalIndices = 0;

    // 纹理图集
    VulkanTextureAtlas m_textureAtlas;

    // 暂存缓冲区
    std::unique_ptr<StagingBuffer> m_stagingBuffer;
    VkDeviceSize m_stagingBufferSize = 16 * 1024 * 1024; // 16MB

    // 单次命令缓冲区
    [[nodiscard]] Result<VkCommandBuffer> beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // 创建/更新缓冲区
    [[nodiscard]] Result<void> createChunkBuffer(
        ChunkGpuBuffer& buffer,
        const MeshData& meshData);

    [[nodiscard]] Result<void> uploadBufferData(
        VulkanBuffer& dstBuffer,
        const void* data,
        VkDeviceSize size);
};

} // namespace mr::client
