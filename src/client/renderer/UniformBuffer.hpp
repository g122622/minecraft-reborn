#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "VulkanBuffer.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace mr::client {

// Uniform缓冲区基类
class UniformBuffer {
public:
    UniformBuffer() = default;
    ~UniformBuffer();

    // 禁止拷贝
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    // 允许移动
    UniformBuffer(UniformBuffer&& other) noexcept;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept;

    // 创建缓冲区（为每一帧创建一个）
    [[nodiscard]] Result<void> create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        u32 frameCount = 2);

    void destroy();

    // 更新数据
    void update(const void* data, VkDeviceSize size, u32 frameIndex);
    void update(const void* data, VkDeviceSize size);

    // 获取当前帧的缓冲区
    VulkanBuffer& buffer(u32 frameIndex);
    const VulkanBuffer& buffer(u32 frameIndex) const;

    // 获取描述符信息
    VkDescriptorBufferInfo descriptorInfo(u32 frameIndex) const;

    VkDeviceSize size() const { return m_size; }
    u32 frameCount() const { return m_frameCount; }
    bool isValid() const { return !m_buffers.empty(); }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    std::vector<VulkanBuffer> m_buffers;
    VkDeviceSize m_size = 0;
    u32 m_frameCount = 0;
};

// 相机Uniform缓冲区数据结构
struct CameraUBO {
    alignas(16) float view[16];
    alignas(16) float projection[16];
    alignas(16) float viewProjection[16];
};

// 光照Uniform缓冲区数据结构
struct LightingUBO {
    alignas(16) float sunDirection[3];
    alignas(4) float sunIntensity;
    alignas(16) float ambientColor[3];
    alignas(4) float ambientIntensity;
    alignas(16) float cameraPosition[3];
    alignas(4) float padding1;
    alignas(16) float fogColor[3];
    alignas(4) float fogStart;
    alignas(4) float fogEnd;
    alignas(4) float fogDensity;
    alignas(4) uint32_t fogMode;
    // 时间相关字段
    alignas(4) float celestialAngle;     // 天体角度 0.0-1.0
    alignas(4) float skyBrightness;      // 天空亮度
    alignas(4) int32_t moonPhase;        // 月相 0-7
    alignas(4) float starBrightness;     // 星星亮度
};

// 模型推送常量数据结构
struct ModelPushConstants {
    alignas(16) float model[16];
    alignas(16) float chunkOffset[3];
    alignas(4) float padding;
};

} // namespace mr::client
