#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

namespace mc::client {

// 描述符绑定信息
struct DescriptorBinding {
    u32 binding = 0;
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    u32 descriptorCount = 1;
};

// 描述符集布局创建信息
struct DescriptorSetLayoutInfo {
    std::vector<DescriptorBinding> bindings;
    VkDescriptorSetLayoutCreateFlags flags = 0;
};

// 描述符集布局管理
class DescriptorLayoutManager {
public:
    DescriptorLayoutManager() = default;
    ~DescriptorLayoutManager();

    // 禁止拷贝
    DescriptorLayoutManager(const DescriptorLayoutManager&) = delete;
    DescriptorLayoutManager& operator=(const DescriptorLayoutManager&) = delete;

    // 初始化
    void initialize(VkDevice device);
    void destroy();

    // 创建布局
    [[nodiscard]] Result<VkDescriptorSetLayout> createLayout(
        const String& name,
        const DescriptorSetLayoutInfo& info);

    // 获取布局
    [[nodiscard]] VkDescriptorSetLayout getLayout(const String& name) const;
    [[nodiscard]] bool hasLayout(const String& name) const;

    VkDevice device() const { return m_device; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    std::unordered_map<String, VkDescriptorSetLayout> m_layouts;
};

// 描述符池管理
class DescriptorPool {
public:
    DescriptorPool() = default;
    ~DescriptorPool();

    // 禁止拷贝
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    // 创建池
    [[nodiscard]] Result<void> create(
        VkDevice device,
        const std::vector<VkDescriptorPoolSize>& poolSizes,
        u32 maxSets);

    void destroy();

    // 分配描述符集
    [[nodiscard]] Result<VkDescriptorSet> allocate(VkDescriptorSetLayout layout);
    [[nodiscard]] Result<std::vector<VkDescriptorSet>> allocate(
        VkDescriptorSetLayout layout,
        u32 count);

    // 重置池（释放所有描述符集）
    void reset();

    VkDescriptorPool pool() const { return m_pool; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
};

// 描述符集更新工具
namespace DescriptorWriter {

// 写入Uniform缓冲区
void writeUniformBuffer(
    VkDescriptorSet set,
    u32 binding,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkDeviceSize range);

// 写入组合图像采样器
void writeCombinedImageSampler(
    VkDescriptorSet set,
    u32 binding,
    VkImageView imageView,
    VkSampler sampler,
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

// 批量更新
void update(VkDevice device, const std::vector<VkWriteDescriptorSet>& writes);

} // namespace DescriptorWriter

} // namespace mc::client
