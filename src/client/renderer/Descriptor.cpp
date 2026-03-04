#include "Descriptor.hpp"
#include <spdlog/spdlog.h>

namespace mr::client {

// ============================================================================
// DescriptorLayoutManager 实现
// ============================================================================

DescriptorLayoutManager::~DescriptorLayoutManager() {
    destroy();
}

void DescriptorLayoutManager::initialize(VkDevice device) {
    m_device = device;
}

void DescriptorLayoutManager::destroy() {
    if (m_device != VK_NULL_HANDLE) {
        for (auto& pair : m_layouts) {
            if (pair.second != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(m_device, pair.second, nullptr);
            }
        }
        m_layouts.clear();
        m_device = VK_NULL_HANDLE;
    }
}

Result<VkDescriptorSetLayout> DescriptorLayoutManager::createLayout(
    const String& name,
    const DescriptorSetLayoutInfo& info)
{
    if (hasLayout(name)) {
        return Error(ErrorCode::AlreadyExists, "Descriptor set layout already exists: " + name);
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(info.bindings.size());

    for (const auto& binding : info.bindings) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding.binding;
        layoutBinding.descriptorType = binding.type;
        layoutBinding.descriptorCount = binding.descriptorCount;
        layoutBinding.stageFlags = binding.stageFlags;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.flags = info.flags;
    createInfo.bindingCount = static_cast<u32>(bindings.size());
    createInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &layout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor set layout: " + name);
    }

    m_layouts[name] = layout;
    spdlog::debug("Created descriptor set layout: {}", name);
    return layout;
}

VkDescriptorSetLayout DescriptorLayoutManager::getLayout(const String& name) const {
    auto it = m_layouts.find(name);
    return (it != m_layouts.end()) ? it->second : VK_NULL_HANDLE;
}

bool DescriptorLayoutManager::hasLayout(const String& name) const {
    return m_layouts.find(name) != m_layouts.end();
}

// ============================================================================
// DescriptorPool 实现
// ============================================================================

DescriptorPool::~DescriptorPool() {
    destroy();
}

Result<void> DescriptorPool::create(
    VkDevice device,
    const std::vector<VkDescriptorPoolSize>& poolSizes,
    u32 maxSets)
{
    m_device = device;

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = maxSets;
    createInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &m_pool) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor pool");
    }

    return {};
}

void DescriptorPool::destroy() {
    if (m_device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
    m_device = VK_NULL_HANDLE;
}

Result<VkDescriptorSet> DescriptorPool::allocate(VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet set;
    VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, &set);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to allocate descriptor set");
    }

    return set;
}

Result<std::vector<VkDescriptorSet>> DescriptorPool::allocate(
    VkDescriptorSetLayout layout,
    u32 count)
{
    std::vector<VkDescriptorSetLayout> layouts(count, layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> sets(count);
    VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, sets.data());
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to allocate descriptor sets");
    }

    return sets;
}

void DescriptorPool::reset() {
    if (m_device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE) {
        vkResetDescriptorPool(m_device, m_pool, 0);
    }
}

// ============================================================================
// DescriptorWriter 实现
// ============================================================================

namespace DescriptorWriter {

void writeUniformBuffer(
    VkDescriptorSet set,
    u32 binding,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkDeviceSize range)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;

    // 注意：需要单独调用 vkUpdateDescriptorSets
}

void writeCombinedImageSampler(
    VkDescriptorSet set,
    u32 binding,
    VkImageView imageView,
    VkSampler sampler,
    VkImageLayout imageLayout)
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;
    imageInfo.imageLayout = imageLayout;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    // 注意：需要单独调用 vkUpdateDescriptorSets
}

void update(VkDevice device, const std::vector<VkWriteDescriptorSet>& writes) {
    vkUpdateDescriptorSets(
        device,
        static_cast<u32>(writes.size()),
        writes.data(),
        0,
        nullptr);
}

} // namespace DescriptorWriter

} // namespace mr::client
