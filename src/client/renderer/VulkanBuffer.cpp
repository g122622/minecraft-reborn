#include "VulkanBuffer.hpp"
#include <cstring>
#include <spdlog/spdlog.h>

namespace mc::client {

// ============================================================================
// VulkanBuffer 实现
// ============================================================================

VulkanBuffer::VulkanBuffer() = default;

VulkanBuffer::~VulkanBuffer() {
    destroy();
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_buffer(other.m_buffer)
    , m_memory(other.m_memory)
    , m_size(other.m_size)
    , m_mapped(other.m_mapped)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
    other.m_mapped = nullptr;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_mapped = other.m_mapped;

        other.m_device = VK_NULL_HANDLE;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_mapped = nullptr;
    }
    return *this;
}

Result<void> VulkanBuffer::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    if (m_buffer != VK_NULL_HANDLE) {
        destroy();
    }

    m_device = device;
    m_size = size;

    // 创建缓冲区
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create buffer");
    }

    // 获取内存需求
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

    // 分配内存
    auto memoryTypeResult = findMemoryType(
        memRequirements.memoryTypeBits,
        properties,
        physicalDevice);

    if (!memoryTypeResult.success()) {
        vkDestroyBuffer(device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
        return memoryTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeResult.value();

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        vkDestroyBuffer(device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
        return Error(ErrorCode::InitializationFailed, "Failed to allocate buffer memory");
    }

    vkBindBufferMemory(device, m_buffer, m_memory, 0);
    return {};
}

void VulkanBuffer::destroy() {
    if (m_mapped) {
        unmap();
    }

    if (m_device != VK_NULL_HANDLE) {
        if (m_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
        }

        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }

        m_device = VK_NULL_HANDLE;
    }

    m_size = 0;
}

Result<void> VulkanBuffer::upload(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (offset + size > m_size) {
        return Error(ErrorCode::InvalidArgument, "Upload size exceeds buffer size");
    }

    void* mapped = map().value();
    if (!mapped) {
        return Error(ErrorCode::OperationFailed, "Failed to map buffer");
    }

    std::memcpy(static_cast<char*>(mapped) + offset, data, static_cast<size_t>(size));
    unmap();

    return {};
}

Result<void*> VulkanBuffer::map() {
    if (m_mapped) {
        return m_mapped;
    }

    if (vkMapMemory(m_device, m_memory, 0, m_size, 0, &m_mapped) != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to map buffer memory");
    }

    return m_mapped;
}

void VulkanBuffer::unmap() {
    if (m_mapped && m_device != VK_NULL_HANDLE) {
        vkUnmapMemory(m_device, m_memory);
        m_mapped = nullptr;
    }
}

Result<u32> VulkanBuffer::findMemoryType(
    u32 typeFilter,
    VkMemoryPropertyFlags properties,
    VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::NotFound, "Failed to find suitable memory type");
}

// ============================================================================
// StagingBuffer 实现
// ============================================================================

Result<void> StagingBuffer::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size)
{
    return VulkanBuffer::create(
        device,
        physicalDevice,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

Result<void> StagingBuffer::copyTo(
    VkCommandBuffer commandBuffer,
    VulkanBuffer& dstBuffer,
    VkDeviceSize size)
{
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, buffer(), dstBuffer.buffer(), 1, &copyRegion);
    return {};
}

// ============================================================================
// VertexBuffer 实现
// ============================================================================

Result<void> VertexBuffer::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize vertexCount,
    VkDeviceSize vertexSize)
{
    return VulkanBuffer::create(
        device,
        physicalDevice,
        vertexCount * vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void VertexBuffer::bind(VkCommandBuffer commandBuffer) const {
    VkBuffer vertexBuffers[] = { buffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

// ============================================================================
// IndexBuffer 实现
// ============================================================================

Result<void> IndexBuffer::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize indexCount,
    VkIndexType indexType)
{
    m_indexType = indexType;
    m_indexCount = indexCount;

    VkDeviceSize indexSize = (indexType == VK_INDEX_TYPE_UINT32) ? 4 : 2;

    return VulkanBuffer::create(
        device,
        physicalDevice,
        indexCount * indexSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void IndexBuffer::bind(VkCommandBuffer commandBuffer) const {
    vkCmdBindIndexBuffer(commandBuffer, buffer(), 0, m_indexType);
}

} // namespace mc::client
