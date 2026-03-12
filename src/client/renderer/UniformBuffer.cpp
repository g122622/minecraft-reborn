#include "UniformBuffer.hpp"
#include <cstring>

namespace mc::client {

// ============================================================================
// UniformBuffer 实现
// ============================================================================

UniformBuffer::~UniformBuffer() {
    destroy();
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_buffers(std::move(other.m_buffers))
    , m_size(other.m_size)
    , m_frameCount(other.m_frameCount)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_size = 0;
    other.m_frameCount = 0;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_buffers = std::move(other.m_buffers);
        m_size = other.m_size;
        m_frameCount = other.m_frameCount;

        other.m_device = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_frameCount = 0;
    }
    return *this;
}

Result<void> UniformBuffer::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    u32 frameCount)
{
    m_device = device;
    m_size = size;
    m_frameCount = frameCount;

    m_buffers.resize(frameCount);

    for (u32 i = 0; i < frameCount; ++i) {
        auto result = m_buffers[i].create(
            device,
            physicalDevice,
            size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (!result.success()) {
            destroy();
            return Error(ErrorCode::InitializationFailed,
                "Failed to create uniform buffer for frame " + std::to_string(i));
        }
    }

    return {};
}

void UniformBuffer::destroy() {
    for (auto& buffer : m_buffers) {
        buffer.destroy();
    }
    m_buffers.clear();
    m_device = VK_NULL_HANDLE;
    m_size = 0;
    m_frameCount = 0;
}

void UniformBuffer::update(const void* data, VkDeviceSize size, u32 frameIndex) {
    if (frameIndex >= m_frameCount || !m_buffers[frameIndex].isValid()) {
        return;
    }

    void* mapped = m_buffers[frameIndex].map().value();
    if (mapped) {
        std::memcpy(mapped, data, static_cast<size_t>(std::min(size, m_size)));
        m_buffers[frameIndex].unmap();
    }
}

void UniformBuffer::update(const void* data, VkDeviceSize size) {
    // 更新所有帧的缓冲区
    for (u32 i = 0; i < m_frameCount; ++i) {
        update(data, size, i);
    }
}

VulkanBuffer& UniformBuffer::buffer(u32 frameIndex) {
    return m_buffers[frameIndex];
}

const VulkanBuffer& UniformBuffer::buffer(u32 frameIndex) const {
    return m_buffers[frameIndex];
}

VkDescriptorBufferInfo UniformBuffer::descriptorInfo(u32 frameIndex) const {
    VkDescriptorBufferInfo info{};
    info.buffer = m_buffers[frameIndex].buffer();
    info.offset = 0;
    info.range = m_size;
    return info;
}

} // namespace mc::client
