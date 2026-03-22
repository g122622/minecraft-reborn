#include "BreakProgressRenderer.hpp"
#include "BreakProgressManager.hpp"
#include "client/resource/DestroyStageTextures.hpp"
#include "client/renderer/util/ShaderPath.hpp"
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>

namespace mc {
namespace client {
namespace renderer {
namespace trident {
namespace block {

// ============================================================================
// 常量
// ============================================================================

namespace {
    /// 破坏覆盖层立方体顶点数（6面 * 4顶点）
    constexpr size_t VERTICES_PER_CUBE = 24;

    /// 破坏覆盖层立方体索引数（6面 * 2三角形 * 3索引）
    constexpr size_t INDICES_PER_CUBE = 36;

    /// 默认缓冲区容量
    constexpr size_t DEFAULT_MAX_CUBES = 64;
}

// ============================================================================
// 构造/析构
// ============================================================================

BreakProgressRenderer::~BreakProgressRenderer() {
    cleanup();
}

// ============================================================================
// 初始化
// ============================================================================

bool BreakProgressRenderer::initialize(const Config& config) {
    if (m_initialized) {
        return true;
    }

    spdlog::info("BreakProgressRenderer: Initializing...");

    m_config = config;

    if (m_config.device == VK_NULL_HANDLE) {
        spdlog::error("BreakProgressRenderer: Device is null");
        return false;
    }

    // 创建管线
    if (!createPipeline()) {
        spdlog::error("BreakProgressRenderer: Failed to create pipeline");
        return false;
    }

    // 创建缓冲区
    if (!createBuffers()) {
        spdlog::error("BreakProgressRenderer: Failed to create buffers");
        return false;
    }

    // 创建描述符集
    if (!createDescriptorSets()) {
        spdlog::error("BreakProgressRenderer: Failed to create descriptor sets");
        return false;
    }

    // 上传纹理图集
    if (!uploadTextureAtlas()) {
        spdlog::error("BreakProgressRenderer: Failed to upload texture atlas");
        return false;
    }

    m_initialized = true;
    spdlog::info("BreakProgressRenderer: Initialized successfully");
    return true;
}

void BreakProgressRenderer::cleanup() {
    if (!m_initialized) {
        return;
    }

    VkDevice device = m_config.device;

    // 等待设备空闲
    vkDeviceWaitIdle(device);

    // 销毁纹理资源
    if (m_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_textureSampler, nullptr);
        m_textureSampler = VK_NULL_HANDLE;
    }
    if (m_textureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_textureImageView, nullptr);
        m_textureImageView = VK_NULL_HANDLE;
    }
    if (m_textureImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_textureImage, nullptr);
        m_textureImage = VK_NULL_HANDLE;
    }
    if (m_textureImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_textureImageMemory, nullptr);
        m_textureImageMemory = VK_NULL_HANDLE;
    }

    // 销毁暂存缓冲区
    if (m_stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_stagingBuffer, nullptr);
        m_stagingBuffer = VK_NULL_HANDLE;
    }
    if (m_stagingBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_stagingBufferMemory, nullptr);
        m_stagingBufferMemory = VK_NULL_HANDLE;
    }

    // 销毁顶点缓冲区
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }
    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    // 销毁索引缓冲区
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_indexBuffer, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
    }
    if (m_indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_indexBufferMemory, nullptr);
        m_indexBufferMemory = VK_NULL_HANDLE;
    }

    // 销毁描述符
    m_descriptorSet = VK_NULL_HANDLE;
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    // 销毁管线
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    m_initialized = false;
    spdlog::info("BreakProgressRenderer: Cleaned up");
}

// ============================================================================
// 更新
// ============================================================================

void BreakProgressRenderer::updateMesh(const Vector3& cameraPos) {
    // 获取可见的破坏进度
    auto& manager = BreakProgressManager::instance();
    m_progressEntries.clear();

    auto visibleProgress = manager.getVisibleProgress(cameraPos);
    // 转换 pair 格式到 ProgressEntry 格式
    m_progressEntries.reserve(visibleProgress.size());
    for (const auto& [pos, stage] : visibleProgress) {
        m_progressEntries.push_back({pos, stage});
    }

    // 如果没有进度，直接返回
    if (m_progressEntries.empty()) {
        return;
    }

    // 检查是否需要扩展缓冲区
    size_t requiredVertices = m_progressEntries.size() * VERTICES_PER_CUBE;
    size_t requiredIndices = m_progressEntries.size() * INDICES_PER_CUBE;

    // TODO: 动态调整缓冲区大小
    // 当前使用固定大小的缓冲区，超出部分会被裁剪

    // 生成顶点数据
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    vertices.reserve(requiredVertices);
    indices.reserve(requiredIndices);

    u32 vertexOffset = 0;
    for (const auto& entry : m_progressEntries) {
        generateCubeMesh(entry.position, vertices, indices, vertexOffset);
    }

    // 更新缓冲区
    updateVertexBuffer(vertices);
    updateIndexBuffer(indices);

    m_vertexCount = vertices.size();
    m_indexCount = indices.size();
}

// ============================================================================
// 渲染
// ============================================================================

void BreakProgressRenderer::render(VkCommandBuffer commandBuffer,
                                     VkDescriptorSet cameraDescriptorSet,
                                     VkDescriptorSet fogDescriptorSet) {
    if (!m_initialized || m_progressEntries.empty()) {
        return;
    }

    // 绑定管线
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // 绑定描述符集
    // Set 0: Camera UBO
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);

    // Set 1: 纹理图集
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 1, 1, &m_descriptorSet, 0, nullptr);

    // Set 2: Fog UBO
    if (fogDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_pipelineLayout, 2, 1, &fogDescriptorSet, 0, nullptr);
    }

    // 绑定顶点缓冲区
    VkBuffer vertexBuffers[] = { m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // 绑定索引缓冲区
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // 为每个破坏进度设置推送常量并绘制
    // TODO: 批量绘制，当前为简化实现逐个绘制

    // 绘制所有
    vkCmdDrawIndexed(commandBuffer, static_cast<u32>(m_indexCount), 1, 0, 0, 0);
}

// ============================================================================
// 私有方法 - 资源创建
// ============================================================================

bool BreakProgressRenderer::createPipeline() {
    // 加载着色器
    auto vertPath = resolveShaderPath("break_overlay.vert.spv");
    auto fragPath = resolveShaderPath("break_overlay.frag.spv");

    if (vertPath.empty() || fragPath.empty()) {
        spdlog::error("BreakProgressRenderer: Failed to resolve shader paths");
        return false;
    }

    // 读取着色器代码
    std::vector<char> vertCode, fragCode;

    std::ifstream vertFile(vertPath, std::ios::binary | std::ios::ate);
    if (!vertFile.is_open()) {
        spdlog::error("BreakProgressRenderer: Failed to open vertex shader: {}", vertPath.string());
        return false;
    }
    size_t vertSize = vertFile.tellg();
    vertFile.seekg(0);
    vertCode.resize(vertSize);
    vertFile.read(vertCode.data(), vertSize);
    vertFile.close();

    std::ifstream fragFile(fragPath, std::ios::binary | std::ios::ate);
    if (!fragFile.is_open()) {
        spdlog::error("BreakProgressRenderer: Failed to open fragment shader: {}", fragPath.string());
        return false;
    }
    size_t fragSize = fragFile.tellg();
    fragFile.seekg(0);
    fragCode.resize(fragSize);
    fragFile.read(fragCode.data(), fragSize);
    fragFile.close();

    // 创建着色器模块
    VkShaderModule vertModule, fragModule;

    VkShaderModuleCreateInfo vertCreateInfo{};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertCreateInfo.codeSize = vertCode.size();
    vertCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());

    if (vkCreateShaderModule(m_config.device, &vertCreateInfo, nullptr, &vertModule) != VK_SUCCESS) {
        spdlog::error("BreakProgressRenderer: Failed to create vertex shader module");
        return false;
    }

    VkShaderModuleCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragCreateInfo.codeSize = fragCode.size();
    fragCreateInfo.pCode = reinterpret_cast<const u32*>(fragCode.data());

    if (vkCreateShaderModule(m_config.device, &fragCreateInfo, nullptr, &fragModule) != VK_SUCCESS) {
        vkDestroyShaderModule(m_config.device, vertModule, nullptr);
        spdlog::error("BreakProgressRenderer: Failed to create fragment shader module");
        return false;
    }

    // 着色器阶段
    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

    // 顶点输入描述
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // 位置属性
    VkVertexInputAttributeDescription positionAttr{};
    positionAttr.binding = 0;
    positionAttr.location = 0;
    positionAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttr.offset = offsetof(Vertex, x);

    // UV属性
    VkVertexInputAttributeDescription texCoordAttr{};
    texCoordAttr.binding = 0;
    texCoordAttr.location = 1;
    texCoordAttr.format = VK_FORMAT_R32G32_SFLOAT;
    texCoordAttr.offset = offsetof(Vertex, u);

    VkVertexInputAttributeDescription attributeDescriptions[] = { positionAttr, texCoordAttr };

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions;

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪（动态设置）
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = -1.0f;
    rasterizer.depthBiasSlopeFactor = -1.0f;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 深度模板
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // 混合 - 使用 MC 的叠加混合模式：DST_COLOR * SRC_COLOR
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 推送常量
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(f32) * 4;  // vec3 blockPos + float damageStage

    // 创建纹理描述符布局
    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &textureBinding;

    if (vkCreateDescriptorSetLayout(m_config.device, &textureLayoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(m_config.device, vertModule, nullptr);
        vkDestroyShaderModule(m_config.device, fragModule, nullptr);
        spdlog::error("BreakProgressRenderer: Failed to create descriptor set layout");
        return false;
    }

    // 管线布局描述符集
    VkDescriptorSetLayout layouts[3] = {
        m_config.cameraLayout,
        m_descriptorSetLayout,
        m_config.fogLayout
    };

    // 管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 3;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_config.device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(m_config.device, m_descriptorSetLayout, nullptr);
        vkDestroyShaderModule(m_config.device, vertModule, nullptr);
        vkDestroyShaderModule(m_config.device, fragModule, nullptr);
        spdlog::error("BreakProgressRenderer: Failed to create pipeline layout");
        return false;
    }

    // 动态状态
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // 创建图形管线
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_config.renderPass;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(m_config.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    // 清理着色器模块
    vkDestroyShaderModule(m_config.device, vertModule, nullptr);
    vkDestroyShaderModule(m_config.device, fragModule, nullptr);

    if (result != VK_SUCCESS) {
        spdlog::error("BreakProgressRenderer: Failed to create graphics pipeline");
        return false;
    }

    spdlog::info("BreakProgressRenderer: Pipeline created successfully");
    return true;
}

bool BreakProgressRenderer::createBuffers() {
    m_maxVertices = DEFAULT_MAX_CUBES * VERTICES_PER_CUBE;
    m_maxIndices = DEFAULT_MAX_CUBES * INDICES_PER_CUBE;

    // 创建顶点缓冲区
    VkBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = m_maxVertices * sizeof(Vertex);
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_config.device, &vertexBufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS) {
        return false;
    }

    // 分配顶点缓冲区内存
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_config.device, m_vertexBuffer, &memRequirements);

    // 查找合适的内存类型
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_config.physicalDevice, &memProperties);

    u32 memoryTypeIndex = UINT32_MAX;
    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(m_config.device, &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(m_config.device, m_vertexBuffer, m_vertexBufferMemory, 0);

    // 创建索引缓冲区
    VkBufferCreateInfo indexBufferInfo{};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = m_maxIndices * sizeof(u32);
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_config.device, &indexBufferInfo, nullptr, &m_indexBuffer) != VK_SUCCESS) {
        return false;
    }

    // 分配索引缓冲区内存
    vkGetBufferMemoryRequirements(m_config.device, m_indexBuffer, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(m_config.device, &allocInfo, nullptr, &m_indexBufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(m_config.device, m_indexBuffer, m_indexBufferMemory, 0);

    spdlog::info("BreakProgressRenderer: Buffers created (vertices: {}, indices: {})",
                 m_maxVertices, m_maxIndices);
    return true;
}

bool BreakProgressRenderer::createDescriptorSets() {
    // 创建描述符池
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(m_config.device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        spdlog::error("BreakProgressRenderer: Failed to create descriptor pool");
        return false;
    }

    // 分配描述符集
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    if (vkAllocateDescriptorSets(m_config.device, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
        spdlog::error("BreakProgressRenderer: Failed to allocate descriptor set");
        return false;
    }

    spdlog::info("BreakProgressRenderer: Descriptor sets created");
    return true;
}

bool BreakProgressRenderer::uploadTextureAtlas() {
    auto& textures = DestroyStageTextures::instance();

    // 初始化纹理资源
    if (!textures.initialize()) {
        spdlog::error("BreakProgressRenderer: Failed to initialize destroy stage textures");
        return false;
    }

    const auto& atlasData = textures.getAtlasData();
    u32 width = textures.atlasWidth();
    u32 height = textures.atlasHeight();

    spdlog::info("BreakProgressRenderer: Uploading texture atlas ({}x{})", width, height);

    // 创建暂存缓冲区
    VkDeviceSize imageSize = width * height * 4;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_config.device, &stagingBufferInfo, nullptr, &m_stagingBuffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_config.device, m_stagingBuffer, &memRequirements);

    // 查找合适的内存类型
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_config.physicalDevice, &memProperties);

    u32 memoryTypeIndex = UINT32_MAX;
    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(m_config.device, &allocInfo, nullptr, &m_stagingBufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(m_config.device, m_stagingBuffer, m_stagingBufferMemory, 0);

    // 复制数据到暂存缓冲区
    void* data;
    vkMapMemory(m_config.device, m_stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, atlasData.data(), imageSize);
    vkUnmapMemory(m_config.device, m_stagingBufferMemory);

    // 创建图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_config.device, &imageInfo, nullptr, &m_textureImage) != VK_SUCCESS) {
        return false;
    }

    vkGetImageMemoryRequirements(m_config.device, m_textureImage, &memRequirements);

    // 查找设备本地内存
    memoryTypeIndex = UINT32_MAX;
    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(m_config.device, &allocInfo, nullptr, &m_textureImageMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(m_config.device, m_textureImage, m_textureImageMemory, 0);

    // 转换图像布局并复制
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = m_config.commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_config.device, &cmdAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_textureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

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

    vkCmdCopyBufferToImage(cmd, m_stagingBuffer, m_textureImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_config.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_config.graphicsQueue);

    vkFreeCommandBuffers(m_config.device, m_config.commandPool, 1, &cmd);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_config.device, &viewInfo, nullptr, &m_textureImageView) != VK_SUCCESS) {
        return false;
    }

    // 创建采样器
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_config.device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        return false;
    }

    // 更新描述符集
    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = m_textureImageView;
    descriptorImageInfo.sampler = m_textureSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &descriptorImageInfo;

    vkUpdateDescriptorSets(m_config.device, 1, &descriptorWrite, 0, nullptr);

    spdlog::info("BreakProgressRenderer: Texture atlas uploaded");
    return true;
}

// ============================================================================
// 私有方法 - 网格生成
// ============================================================================

void BreakProgressRenderer::generateCubeMesh(const BlockPos& pos,
                                               std::vector<Vertex>& vertices,
                                               std::vector<u32>& indices,
                                               u32& vertexOffset) {
    // 生成立方体顶点（相对于方块位置）
    // 立方体稍微放大以避免z-fighting

    f32 x0 = static_cast<f32>(pos.x) - 0.001f;
    f32 y0 = static_cast<f32>(pos.y) - 0.001f;
    f32 x1 = static_cast<f32>(pos.x) + 1.001f;
    f32 y1 = static_cast<f32>(pos.y) + 1.001f;
    f32 z0 = static_cast<f32>(pos.z) - 0.001f;
    f32 z1 = static_cast<f32>(pos.z) + 1.001f;

    // 6个面，每面4个顶点
    // 底面 (y = y0)
    vertices.push_back({x0, y0, z0, 0.0f, 0.0f});
    vertices.push_back({x1, y0, z0, 1.0f, 0.0f});
    vertices.push_back({x1, y0, z1, 1.0f, 1.0f});
    vertices.push_back({x0, y0, z1, 0.0f, 1.0f});

    // 顶面 (y = y1)
    vertices.push_back({x0, y1, z0, 0.0f, 0.0f});
    vertices.push_back({x0, y1, z1, 1.0f, 0.0f});
    vertices.push_back({x1, y1, z1, 1.0f, 1.0f});
    vertices.push_back({x1, y1, z0, 0.0f, 1.0f});

    // 前面 (z = z1)
    vertices.push_back({x0, y0, z1, 0.0f, 0.0f});
    vertices.push_back({x1, y0, z1, 1.0f, 0.0f});
    vertices.push_back({x1, y1, z1, 1.0f, 1.0f});
    vertices.push_back({x0, y1, z1, 0.0f, 1.0f});

    // 后面 (z = z0)
    vertices.push_back({x1, y0, z0, 0.0f, 0.0f});
    vertices.push_back({x0, y0, z0, 1.0f, 0.0f});
    vertices.push_back({x0, y1, z0, 1.0f, 1.0f});
    vertices.push_back({x1, y1, z0, 0.0f, 1.0f});

    // 右面 (x = x1)
    vertices.push_back({x1, y0, z1, 0.0f, 0.0f});
    vertices.push_back({x1, y0, z0, 1.0f, 0.0f});
    vertices.push_back({x1, y1, z0, 1.0f, 1.0f});
    vertices.push_back({x1, y1, z1, 0.0f, 1.0f});

    // 左面 (x = x0)
    vertices.push_back({x0, y0, z0, 0.0f, 0.0f});
    vertices.push_back({x0, y0, z1, 1.0f, 0.0f});
    vertices.push_back({x0, y1, z1, 1.0f, 1.0f});
    vertices.push_back({x0, y1, z0, 0.0f, 1.0f});

    // 每面6个索引（2个三角形）
    for (u32 i = 0; i < 6; ++i) {
        u32 base = vertexOffset + i * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    vertexOffset += VERTICES_PER_CUBE;
}

void BreakProgressRenderer::updateVertexBuffer(const std::vector<Vertex>& vertices) {
    if (vertices.empty() || m_vertexBuffer == VK_NULL_HANDLE) {
        return;
    }

    void* data;
    VkDeviceSize size = std::min(vertices.size(), m_maxVertices) * sizeof(Vertex);
    vkMapMemory(m_config.device, m_vertexBufferMemory, 0, size, 0, &data);
    memcpy(data, vertices.data(), size);
    vkUnmapMemory(m_config.device, m_vertexBufferMemory);
}

void BreakProgressRenderer::updateIndexBuffer(const std::vector<u32>& indices) {
    if (indices.empty() || m_indexBuffer == VK_NULL_HANDLE) {
        return;
    }

    void* data;
    VkDeviceSize size = std::min(indices.size(), m_maxIndices) * sizeof(u32);
    vkMapMemory(m_config.device, m_indexBufferMemory, 0, size, 0, &data);
    memcpy(data, indices.data(), size);
    vkUnmapMemory(m_config.device, m_indexBufferMemory);
}

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
