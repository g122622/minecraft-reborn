#include "EntityPipeline.hpp"
#include "../VulkanContext.hpp"
#include "../VulkanPipeline.hpp"
#include "../Descriptor.hpp"
#include "../ShaderPath.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <array>

namespace mc::client {

// ============================================================================
// EntityPipeline
// ============================================================================

EntityPipeline::EntityPipeline() = default;

EntityPipeline::~EntityPipeline() {
    destroy();
}

VkVertexInputBindingDescription EntityPipeline::getVertexBindingDescription() {
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(renderer::ModelVertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}

std::vector<VkVertexInputAttributeDescription> EntityPipeline::getVertexAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> descs(3);

    // 位置
    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs[0].offset = offsetof(renderer::ModelVertex, position);

    // 纹理坐标
    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].format = VK_FORMAT_R32G32_SFLOAT;
    descs[1].offset = offsetof(renderer::ModelVertex, texCoord);

    // 法线
    descs[2].binding = 0;
    descs[2].location = 2;
    descs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs[2].offset = offsetof(renderer::ModelVertex, normal);

    return descs;
}

Result<void> EntityPipeline::initialize(VulkanContext* context,
                                        VkRenderPass renderPass,
                                        VkDescriptorSetLayout cameraDescriptorLayout,
                                        VkDescriptorPool descriptorPool,
                                        VkCommandPool commandPool) {
    if (m_initialized) {
        return Result<void>::ok();
    }

    m_context = context;
    m_descriptorPool = descriptorPool;
    m_commandPool = commandPool;

    // 创建描述符布局
    auto result = createDescriptorLayouts();
    if (!result.success()) {
        return result.error();
    }

    // 创建纹理采样器
    result = createTextureSampler();
    if (!result.success()) {
        return result.error();
    }

    // 创建描述符集
    result = createDescriptorSets();
    if (!result.success()) {
        return result.error();
    }

    // 创建管线
    PipelineConfig config{};

    // 着色器路径 - 使用 resolveShaderPath 解析路径
    const auto entityVertPath = resolveShaderPath("entity.vert.spv");
    const auto entityFragPath = resolveShaderPath("entity.frag.spv");
    if (entityVertPath.empty() || entityFragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve entity shader binaries");
    }
    config.vertexShaderPath = entityVertPath.string();
    config.fragmentShaderPath = entityFragPath.string();

    // 顶点输入
    auto bindingDesc = getVertexBindingDescription();
    auto attrDescs = getVertexAttributeDescriptions();
    config.vertexBindings.push_back(bindingDesc);
    config.vertexAttributes = attrDescs;

    // 输入装配
    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.primitiveRestartEnable = VK_FALSE;

    // 光栅化
    config.polygonMode = VK_POLYGON_MODE_FILL;
    // 实体模型由运行时网格生成，部分面的绕序在当前实现下不完全一致。
    // 禁用剔除可避免出现“刺状/破碎”外观。
    config.cullMode = VK_CULL_MODE_NONE;
    config.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.lineWidth = 1.0f;

    // 多重采样
    config.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 深度/模板
    config.depthTestEnable = VK_TRUE;
    config.depthWriteEnable = VK_TRUE;
    config.depthCompareOp = VK_COMPARE_OP_LESS;
    config.stencilTestEnable = VK_FALSE;

    // 颜色混合 - 启用alpha混合
    config.blendEnable = VK_TRUE;
    config.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    config.colorBlendOp = VK_BLEND_OP_ADD;
    config.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    config.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    config.alphaBlendOp = VK_BLEND_OP_ADD;
    config.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // 渲染通道
    config.renderPass = renderPass;
    config.subpass = 0;

    // 描述符布局
    config.descriptorSetLayouts.push_back(cameraDescriptorLayout);
    config.descriptorSetLayouts.push_back(m_textureDescriptorLayout);

    // 推送常量
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(f32) * 16 + sizeof(f32) * 4;  // mat4 + vec3 + float
    config.pushConstantRanges.push_back(pushConstantRange);

    m_pipeline = std::make_unique<VulkanPipeline>();
    auto pipelineResult = m_pipeline->initialize(context, config);
    if (!pipelineResult.success()) {
        return pipelineResult.error();
    }

    m_initialized = true;
    spdlog::info("EntityPipeline initialized");
    return Result<void>::ok();
}

void EntityPipeline::destroy() {
    if (!m_initialized) {
        return;
    }

    // 销毁采样器
    if (m_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_context->device(), m_textureSampler, nullptr);
        m_textureSampler = VK_NULL_HANDLE;
    }

    // 销毁描述符布局
    if (m_textureDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_context->device(), m_textureDescriptorLayout, nullptr);
        m_textureDescriptorLayout = VK_NULL_HANDLE;
    }

    // 管线通过unique_ptr自动销毁
    m_pipeline.reset();

    m_initialized = false;
    spdlog::info("EntityPipeline destroyed");
}

void EntityPipeline::bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipeline());
}

Result<EntityMesh> EntityPipeline::createMesh(const std::vector<renderer::ModelVertex>& vertices,
                                               const std::vector<u32>& indices) {
    EntityMesh mesh;
    mesh.vertexCount = static_cast<u32>(vertices.size());
    mesh.indexCount = static_cast<u32>(indices.size());

    if (vertices.empty() || indices.empty()) {
        return mesh;  // 隐式转换为Result<EntityMesh>
    }

    VkDevice device = m_context->device();

    // 创建顶点缓冲区
    VkDeviceSize vertexBufferSize = sizeof(renderer::ModelVertex) * vertices.size();
    VkBuffer vertexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexStagingMemory = VK_NULL_HANDLE;

    // 创建暂存缓冲区
    auto result = createBuffer(vertexBufferSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               vertexStagingBuffer, vertexStagingMemory);
    if (!result.success()) {
        return result.error();
    }

    // 填充暂存缓冲区
    void* data;
    vkMapMemory(device, vertexStagingMemory, 0, vertexBufferSize, 0, &data);
    std::memcpy(data, vertices.data(), vertexBufferSize);
    vkUnmapMemory(device, vertexStagingMemory);

    // 创建设备本地缓冲区
    result = createBuffer(vertexBufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          mesh.vertexBuffer, mesh.vertexMemory);
    if (!result.success()) {
        vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
        vkFreeMemory(device, vertexStagingMemory, nullptr);
        return result.error();
    }

    // 复制到设备本地缓冲区
    copyBuffer(vertexStagingBuffer, mesh.vertexBuffer, vertexBufferSize);

    // 清理暂存缓冲区
    vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
    vkFreeMemory(device, vertexStagingMemory, nullptr);

    // 创建索引缓冲区
    VkDeviceSize indexBufferSize = sizeof(u32) * indices.size();
    VkBuffer indexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexStagingMemory = VK_NULL_HANDLE;

    result = createBuffer(indexBufferSize,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          indexStagingBuffer, indexStagingMemory);
    if (!result.success()) {
        vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        vkFreeMemory(device, mesh.vertexMemory, nullptr);
        return result.error();
    }

    // 填充暂存缓冲区
    vkMapMemory(device, indexStagingMemory, 0, indexBufferSize, 0, &data);
    std::memcpy(data, indices.data(), indexBufferSize);
    vkUnmapMemory(device, indexStagingMemory);

    // 创建设备本地缓冲区
    result = createBuffer(indexBufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          mesh.indexBuffer, mesh.indexMemory);
    if (!result.success()) {
        vkDestroyBuffer(device, indexStagingBuffer, nullptr);
        vkFreeMemory(device, indexStagingMemory, nullptr);
        vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        vkFreeMemory(device, mesh.vertexMemory, nullptr);
        return result.error();
    }

    // 复制到设备本地缓冲区
    copyBuffer(indexStagingBuffer, mesh.indexBuffer, indexBufferSize);

    // 清理暂存缓冲区
    vkDestroyBuffer(device, indexStagingBuffer, nullptr);
    vkFreeMemory(device, indexStagingMemory, nullptr);

    return mesh;  // 隐式转换为Result<EntityMesh>
}

Result<void> EntityPipeline::updateMesh(EntityMesh& mesh,
                                        const std::vector<renderer::ModelVertex>& vertices,
                                        const std::vector<u32>& indices) {
    // 销毁旧的缓冲区
    destroyMesh(mesh);

    // 创建新的缓冲区
    auto result = createMesh(vertices, indices);
    if (!result.success()) {
        return result.error();
    }

    // 复制新数据
    EntityMesh newMesh = std::move(result.value());
    mesh.vertexBuffer = newMesh.vertexBuffer;
    mesh.vertexMemory = newMesh.vertexMemory;
    mesh.indexBuffer = newMesh.indexBuffer;
    mesh.indexMemory = newMesh.indexMemory;
    mesh.vertexCount = newMesh.vertexCount;
    mesh.indexCount = newMesh.indexCount;

    return Result<void>::ok();
}

void EntityPipeline::destroyMesh(EntityMesh& mesh) {
    VkDevice device = m_context->device();

    if (mesh.vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        mesh.vertexBuffer = VK_NULL_HANDLE;
    }

    if (mesh.vertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, mesh.vertexMemory, nullptr);
        mesh.vertexMemory = VK_NULL_HANDLE;
    }

    if (mesh.indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
        mesh.indexBuffer = VK_NULL_HANDLE;
    }

    if (mesh.indexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, mesh.indexMemory, nullptr);
        mesh.indexMemory = VK_NULL_HANDLE;
    }

    mesh.vertexCount = 0;
    mesh.indexCount = 0;
}

void EntityPipeline::drawMesh(VkCommandBuffer cmd,
                               const EntityMesh& mesh,
                               const std::array<f32, 16>& modelMatrix,
                               const Vector3f& position,
                               f32 scale) {
    if (mesh.vertexCount == 0 || mesh.indexCount == 0) {
        return;
    }

    // 绑定顶点缓冲区
    VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // 绑定索引缓冲区
    vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // 推送常量
    struct PushConstants {
        std::array<f32, 16> model;
        f32 posX, posY, posZ;
        f32 scale;
    } pc;

    pc.model = modelMatrix;
    pc.posX = position.x;
    pc.posY = position.y;
    pc.posZ = position.z;
    pc.scale = scale;

    vkCmdPushConstants(cmd, m_pipeline->layout(), VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(PushConstants), &pc);

    // 绘制
    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
}

void EntityPipeline::bindTextureDescriptor(VkCommandBuffer cmd) {
    if (m_textureDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_pipeline->layout(), 1, 1, &m_textureDescriptorSet, 0, nullptr);
    }
}

void EntityPipeline::setTextureAtlas(VkImageView textureView, VkSampler sampler) {
    if (m_textureDescriptorSet == VK_NULL_HANDLE || textureView == VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_context->device();

    // 更新描述符集
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureView;
    imageInfo.sampler = sampler != VK_NULL_HANDLE ? sampler : m_textureSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_textureDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

VkPipelineLayout EntityPipeline::pipelineLayout() const {
    return m_pipeline ? m_pipeline->layout() : VK_NULL_HANDLE;
}

Result<void> EntityPipeline::createDescriptorLayouts() {
    VkDevice device = m_context->device();

    // 纹理采样器描述符布局（绑定到 set 1）
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                                   &m_textureDescriptorLayout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to create texture descriptor layout");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::createTextureSampler() {
    VkDevice device = m_context->device();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;  // 实体使用最近邻过滤以保持像素风格
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &m_textureSampler);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to create texture sampler");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::createDescriptorSets() {
    VkDevice device = m_context->device();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_textureDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &m_textureDescriptorSet);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to allocate texture descriptor set");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::createBuffer(VkDeviceSize size,
                                           VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags properties,
                                           VkBuffer& buffer,
                                           VkDeviceMemory& memory) {
    VkDevice device = m_context->device();

    // 创建缓冲区
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create buffer");
    }

    // 获取内存需求
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // 分配内存
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = m_context->findMemoryType(memRequirements.memoryTypeBits, properties);
    if (!memTypeResult.success()) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &memory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate buffer memory");
    }

    // 绑定内存
    vkBindBufferMemory(device, buffer, memory, 0);

    return Result<void>::ok();
}

void EntityPipeline::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer cmd = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(cmd);
}

VkCommandBuffer EntityPipeline::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_context->device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void EntityPipeline::endSingleTimeCommands(VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_context->graphicsQueue());

    vkFreeCommandBuffers(m_context->device(), m_commandPool, 1, &cmd);
}

} // namespace mc::client
