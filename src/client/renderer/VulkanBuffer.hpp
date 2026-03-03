#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace mr::client {

// Vulkan缓冲区
class VulkanBuffer {
public:
    VulkanBuffer();
    ~VulkanBuffer();

    // 禁止拷贝
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    // 允许移动
    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    // 创建缓冲区
    [[nodiscard]] Result<void> create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    void destroy();

    // 数据上传
    [[nodiscard]] Result<void> upload(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    // 映射/取消映射
    [[nodiscard]] Result<void*> map();
    void unmap();

    // 获取器
    VkBuffer buffer() const { return m_buffer; }
    VkDeviceMemory memory() const { return m_memory; }
    VkDeviceSize size() const { return m_size; }
    bool isValid() const { return m_buffer != VK_NULL_HANDLE; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    void* m_mapped = nullptr;

    [[nodiscard]] Result<u32> findMemoryType(
        u32 typeFilter,
        VkMemoryPropertyFlags properties,
        VkPhysicalDevice physicalDevice);
};

// 临时缓冲区 (用于暂存数据)
class StagingBuffer : public VulkanBuffer {
public:
    [[nodiscard]] Result<void> create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size);

    [[nodiscard]] Result<void> copyTo(
        VkCommandBuffer commandBuffer,
        VulkanBuffer& dstBuffer,
        VkDeviceSize size);
};

// 顶点缓冲区
class VertexBuffer : public VulkanBuffer {
public:
    [[nodiscard]] Result<void> create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize vertexCount,
        VkDeviceSize vertexSize);

    void bind(VkCommandBuffer commandBuffer) const;
};

// 索引缓冲区
class IndexBuffer : public VulkanBuffer {
public:
    [[nodiscard]] Result<void> create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize indexCount,
        VkIndexType indexType = VK_INDEX_TYPE_UINT32);

    void bind(VkCommandBuffer commandBuffer) const;

    VkIndexType indexType() const { return m_indexType; }
    VkDeviceSize indexCount() const { return m_indexCount; }

private:
    VkIndexType m_indexType = VK_INDEX_TYPE_UINT32;
    VkDeviceSize m_indexCount = 0;
};

} // namespace mr::client
