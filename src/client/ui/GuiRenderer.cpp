#include "GuiRenderer.hpp"
#include "../renderer/util/ShaderPath.hpp"
#include "../renderer/VulkanBuffer.hpp"
#include "../renderer/VulkanTexture.hpp"
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

namespace mc::client {

// 从文件加载SPIR-V着色器
static std::vector<u8> loadShaderFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::error("Failed to open shader file: {}", path.string());
        return {};
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<u8> code(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), fileSize);
    file.close();
    
    spdlog::info("Loaded GUI shader: {} ({} bytes)", path.string(), fileSize);
    return code;
}

static VkShaderModule createShaderModuleHelper(VkDevice device, const std::vector<u8>& code) {
    if (code.empty()) {
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        spdlog::error("Failed to create shader module");
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

GuiRenderer::GuiRenderer() = default;

GuiRenderer::~GuiRenderer() {
    destroy();
}

Result<void> GuiRenderer::initialize(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkRenderPass renderPass) {
    if (device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "VkDevice is null");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;

    // 创建描述符布局
    auto result = createDescriptors();
    if (!result.success()) {
        return result.error();
    }

    // 创建管线布局和图形管线
    result = createPipelineLayout();
    if (!result.success()) {
        return result.error();
    }

    result = createPipeline(renderPass);
    if (!result.success()) {
        return result.error();
    }

    // 创建缓冲区
    result = createBuffers();
    if (!result.success()) {
        return result.error();
    }

    // 创建字体纹理
    result = createFontTexture();
    if (!result.success()) {
        return result.error();
    }

    // 初始化字体渲染器
    if (m_font != nullptr) {
        result = m_fontRenderer.initialize(m_font);
        if (!result.success()) {
            return result.error();
        }
    }

    m_initialized = true;
    return {};
}

void GuiRenderer::destroy() {
    if (!m_initialized) return;

    VkDevice device = m_device;

    // 等待设备空闲
    vkDeviceWaitIdle(device);

    // 销毁纹理
    m_fontTexture.reset();
    m_itemPlaceholderTexture.reset();

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    // 销毁缓冲区
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
    m_fontStagingBuffer.reset();

    // 销毁管线
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    // 销毁描述符
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    m_fontRenderer.destroy();
    m_initialized = false;
}

void GuiRenderer::setFont(Font* font) {
    m_font = font;
    if (m_initialized && m_font != nullptr) {
        const auto initResult = m_fontRenderer.initialize(m_font);
        if (initResult.failed()) {
            spdlog::warn("Failed to reinitialize GUI font renderer: {}", initResult.error().toString());
        }
        m_needsTextureUpdate = true;
    }

}

void GuiRenderer::beginFrame(f32 screenW, f32 screenH) {
    m_screenWidth = screenW;
    m_screenHeight = screenH;
    m_vertices.clear();
    m_indices.clear();
    m_inFrame = true;
}

void GuiRenderer::prepareFrame(VkCommandBuffer commandBuffer) {
    // 首次使用时初始化纹理布局
    static bool texturesInitialized = false;
    if (!texturesInitialized) {
        initializeTextureLayouts(commandBuffer);
        texturesInitialized = true;
    }

    // 在渲染通道外更新字体纹理
    if (m_needsTextureUpdate && m_font != nullptr) {
        updateFontTexture(commandBuffer);
        m_needsTextureUpdate = false;
    }
}

void GuiRenderer::render(VkCommandBuffer commandBuffer) {
    if (m_vertices.empty() || m_indices.empty()) return;

    // 上传顶点和索引数据（使用HOST_VISIBLE内存，可以在任何地方调用）
    uploadBufferData(commandBuffer);

    // 绑定管线
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    // 设置推送常量（屏幕尺寸）
    struct PushConstants {
        f32 screenWidth;
        f32 screenHeight;
        f32 padding[2];
    } pc;
    pc.screenWidth = m_screenWidth;
    pc.screenHeight = m_screenHeight;
    pc.padding[0] = 0.0f;
    pc.padding[1] = 0.0f;

    vkCmdPushConstants(commandBuffer, m_pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

    // 绑定描述符集（字体纹理）
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

    // 绑定顶点缓冲
    VkBuffer vertexBuffer = m_vertexBuffer->buffer();
    VkDeviceSize vertexOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vertexOffset);

    // 绑定索引缓冲
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT32);

    // 绘制
    vkCmdDrawIndexed(commandBuffer, static_cast<u32>(m_indices.size()), 1, 0, 0, 0);
}

f32 GuiRenderer::drawText(const std::string& text, f32 x, f32 y, u32 color, bool shadow) {
    if (m_font == nullptr) return 0.0f;

    m_fontRenderer.beginBatch();
    f32 width;
    if (shadow) {
        width = m_fontRenderer.addTextWithShadow(text, x, y, color);
    } else {
        TextStyle style;
        style.color = color;
        style.shadow = false;
        width = m_fontRenderer.addText(text, x, y, style);
    }
    m_fontRenderer.endBatch();

    // 复制顶点和索引数据
    const auto& textVertices = m_fontRenderer.vertices();
    const auto& textIndices = m_fontRenderer.indices();

    u32 baseIndex = static_cast<u32>(m_vertices.size());
    m_vertices.insert(m_vertices.end(), textVertices.begin(), textVertices.end());

    for (u32 idx : textIndices) {
        m_indices.push_back(baseIndex + idx);
    }

    return width;
}

f32 GuiRenderer::drawTextCentered(const std::string& text, f32 x, f32 y, u32 color) {
    f32 width = getTextWidth(text);
    return drawText(text, x - width * 0.5f, y, color, true);
}

f32 GuiRenderer::getTextWidth(const std::string& text) {
    if (m_font == nullptr) return 0.0f;
    return m_fontRenderer.getTextWidth(text);
}

u32 GuiRenderer::getFontHeight() const {
    return m_fontRenderer.getFontHeight();
}

void GuiRenderer::setFontScale(f32 scale) {
    m_fontRenderer.setScale(scale);
}

f32 GuiRenderer::getFontScale() const {
    return m_fontRenderer.scale();
}

void GuiRenderer::fillRect(f32 x, f32 y, f32 width, f32 height, u32 color) {
    u32 baseIndex = static_cast<u32>(m_vertices.size());

    // 四个顶点
    // 注意：使用负UV作为”纯色矩形”标记，片段着色器将跳过字体纹理采样。
    // 否则会错误地使用字体纹理alpha，导致准星/背景矩形不可见。
    constexpr f32 SOLID_RECT_UV = -1.0f;
    m_vertices.emplace_back(x, y, SOLID_RECT_UV, SOLID_RECT_UV, color);                  // 左上
    m_vertices.emplace_back(x + width, y, SOLID_RECT_UV, SOLID_RECT_UV, color);          // 右上
    m_vertices.emplace_back(x + width, y + height, SOLID_RECT_UV, SOLID_RECT_UV, color); // 右下
    m_vertices.emplace_back(x, y + height, SOLID_RECT_UV, SOLID_RECT_UV, color);         // 左下

    // 两个三角形
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
}

void GuiRenderer::drawTexturedRect(f32 x, f32 y, f32 width, f32 height,
                                     f32 u0, f32 v0, f32 u1, f32 v1, u32 color) {
    u32 baseIndex = static_cast<u32>(m_vertices.size());

    // 物品纹理模式：alpha < 255 表示使用物品纹理采样（alpha=255是字体模式）
    // 默认 ITEM_TEXTURE_COLOR = 0xFEFFFFFF，确保走物品分支且可见

    // 四个顶点，设置纹理坐标
    m_vertices.emplace_back(x, y, u0, v0, color);                  // 左上
    m_vertices.emplace_back(x + width, y, u1, v0, color);          // 右上
    m_vertices.emplace_back(x + width, y + height, u1, v1, color); // 右下
    m_vertices.emplace_back(x, y + height, u0, v1, color);         // 左下

    // 两个三角形
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
}

void GuiRenderer::fillGradientRect(f32 x, f32 y, f32 width, f32 height,
                                    u32 colorTop, u32 colorBottom) {
    u32 baseIndex = static_cast<u32>(m_vertices.size());

    // 四个顶点，顶部和底部不同颜色
    constexpr f32 SOLID_RECT_UV = -1.0f;
    m_vertices.emplace_back(x, y, SOLID_RECT_UV, SOLID_RECT_UV, colorTop);                  // 左上
    m_vertices.emplace_back(x + width, y, SOLID_RECT_UV, SOLID_RECT_UV, colorTop);          // 右上
    m_vertices.emplace_back(x + width, y + height, SOLID_RECT_UV, SOLID_RECT_UV, colorBottom); // 右下
    m_vertices.emplace_back(x, y + height, SOLID_RECT_UV, SOLID_RECT_UV, colorBottom);      // 左下

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
}

void GuiRenderer::drawRect(f32 x, f32 y, f32 width, f32 height, u32 color) {
    // 上边
    fillRect(x, y, width, 1.0f, color);
    // 下边
    fillRect(x, y + height - 1.0f, width, 1.0f, color);
    // 左边
    fillRect(x, y, 1.0f, height, color);
    // 右边
    fillRect(x + width - 1.0f, y, 1.0f, height, color);
}

Result<void> GuiRenderer::createPipelineLayout() {
    VkDevice device = m_device;

    // 推送常量范围
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(f32) * 4; // screenWidth, screenHeight, padding

    // 管线布局创建信息
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create pipeline layout");
    }

    return {};
}

Result<void> GuiRenderer::createPipeline(VkRenderPass renderPass) {
    VkDevice device = m_device;

    const auto vertPath = resolveShaderPath("gui.vert.spv");
    const auto fragPath = resolveShaderPath("gui.frag.spv");

    if (vertPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve GUI vertex shader: gui.vert.spv");
    }
    if (fragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve GUI fragment shader: gui.frag.spv");
    }

    // 加载SPIR-V着色器
    auto vertCode = loadShaderFile(vertPath);
    auto fragCode = loadShaderFile(fragPath);

    if (vertCode.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to load GUI vertex shader");
    }
    if (fragCode.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to load GUI fragment shader");
    }

    // 创建着色器模块
    VkShaderModule vertShaderModule = createShaderModuleHelper(m_device, vertCode);
    VkShaderModule fragShaderModule = createShaderModuleHelper(m_device, fragCode);
    
    if (vertShaderModule == VK_NULL_HANDLE) {
        return Error(ErrorCode::InitializationFailed, "Failed to create vertex shader module");
    }
    if (fragShaderModule == VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to create fragment shader module");
    }

    // 着色器阶段
    VkPipelineShaderStageCreateInfo vertStageInfo = {};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo = {};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    // 顶点输入描述
    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(GuiVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // 位置属性
    VkVertexInputAttributeDescription positionAttr = {};
    positionAttr.binding = 0;
    positionAttr.location = 0;
    positionAttr.format = VK_FORMAT_R32G32_SFLOAT;
    positionAttr.offset = offsetof(GuiVertex, x);

    // 纹理坐标属性
    VkVertexInputAttributeDescription texCoordAttr = {};
    texCoordAttr.binding = 0;
    texCoordAttr.location = 1;
    texCoordAttr.format = VK_FORMAT_R32G32_SFLOAT;
    texCoordAttr.offset = offsetof(GuiVertex, u);

    // 颜色属性
    VkVertexInputAttributeDescription colorAttr = {};
    colorAttr.binding = 0;
    colorAttr.location = 2;
    colorAttr.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttr.offset = offsetof(GuiVertex, color);

    VkVertexInputAttributeDescription attributeDescs[] = {positionAttr, texCoordAttr, colorAttr};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 3;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪（动态设置）
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // GUI不剔除
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 深度测试（GUI禁用）
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    // 颜色混合（启用alpha混合）
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 动态状态（视口和裁剪）
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // 图形管线创建
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    // 清理着色器模块
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create graphics pipeline");
    }

    return {};
}
Result<void> GuiRenderer::createDescriptors() {
    VkDevice device = m_device;

    // 描述符集布局（两个采样器：字体纹理和物品纹理）
    VkDescriptorSetLayoutBinding bindings[2] = {};
    // Binding 0: 字体纹理 (R8)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Binding 1: 物品纹理图集 (RGBA)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor set layout");
    }

    // 描述符池
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 2;  // 两个采样器

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor pool");
    }

    // 分配描述符集
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate descriptor set");
    }

    return {};
}

Result<void> GuiRenderer::createBuffers() {
    VkDevice device = m_device;
    VkPhysicalDevice physicalDevice = m_physicalDevice;

    // 创建顶点缓冲（使用HOST_VISIBLE内存，以便在render pass内直接更新数据）
    m_vertexBuffer = std::make_unique<VulkanBuffer>();
    auto result = m_vertexBuffer->create(device, physicalDevice, 64 * 1024,
                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!result.success()) {
        return result.error();
    }

    // 创建索引缓冲（使用HOST_VISIBLE内存）
    m_indexBuffer = std::make_unique<VulkanBuffer>();
    result = m_indexBuffer->create(device, physicalDevice, 128 * 1024,
                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!result.success()) {
        return result.error();
    }

    // 不再需要暂存缓冲
    return {};
}

Result<void> GuiRenderer::createFontTexture() {
    VkDevice device = m_device;
    VkPhysicalDevice physicalDevice = m_physicalDevice;

    // 创建字体纹理（256x256，单通道）
    m_fontTexture = std::make_unique<VulkanTexture>();

    TextureConfig config;
    config.width = 256;
    config.height = 256;
    config.format = VK_FORMAT_R8_UNORM; // 单通道灰度
    config.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    config.mipLevels = 1;

    auto result = m_fontTexture->create(device, physicalDevice, config);
    if (!result.success()) {
        return result.error();
    }

    // 创建图像视图
    result = m_fontTexture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
    if (!result.success()) {
        return result.error();
    }

    // 创建采样器
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sampler");
    }

    // 创建字体纹理暂存缓冲
    m_fontStagingBuffer = std::make_unique<VulkanBuffer>();
    result = m_fontStagingBuffer->create(device, physicalDevice, 256 * 256,
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!result.success()) {
        return result.error();
    }

    // 初始清空纹理
    auto mapResult1 = m_fontStagingBuffer->map();
    void* mapped1 = mapResult1.success() ? mapResult1.value() : nullptr;
    if (mapped1 != nullptr) {
        std::memset(mapped1, 0, 256 * 256);
        m_fontStagingBuffer->unmap();
    }

    // 立即上传初始数据并转换布局
    // 由于这里没有commandBuffer，我们将在首次prepareFrame时上传
    // 先创建一个空的RGBA纹理作为物品纹理占位符
    m_itemPlaceholderTexture = std::make_unique<VulkanTexture>();
    TextureConfig placeholderConfig;
    placeholderConfig.width = 1;
    placeholderConfig.height = 1;
    placeholderConfig.format = VK_FORMAT_R8G8B8A8_SRGB;
    placeholderConfig.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    placeholderConfig.mipLevels = 1;

    result = m_itemPlaceholderTexture->create(device, physicalDevice, placeholderConfig);
    if (!result.success()) {
        return result.error();
    }

    result = m_itemPlaceholderTexture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
    if (!result.success()) {
        return result.error();
    }

    // 更新描述符集（字体纹理 = binding 0）
    VkDescriptorImageInfo fontImageInfo = {};
    fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    fontImageInfo.imageView = m_fontTexture->imageView();
    fontImageInfo.sampler = m_sampler;

    VkWriteDescriptorSet descriptorWrites[2] = {};
    // Binding 0: 字体纹理
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &fontImageInfo;

    // Binding 1: 物品纹理占位符（使用占位纹理）
    VkDescriptorImageInfo placeholderImageInfo = {};
    placeholderImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    placeholderImageInfo.imageView = m_itemPlaceholderTexture->imageView();
    placeholderImageInfo.sampler = m_sampler;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &placeholderImageInfo;

    vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);

    m_needsTextureUpdate = true;
    return {};
}

void GuiRenderer::updateFontTexture(VkCommandBuffer commandBuffer) {
    if (m_font == nullptr || !m_font->isValid()) return;

    const u8* pixels = m_font->atlasPixels();
    u32 size = m_font->atlasSize();

    if (pixels == nullptr || size == 0) return;

    // 上传到暂存缓冲
    auto mapResult2 = m_fontStagingBuffer->map();
    void* mapped = mapResult2.success() ? mapResult2.value() : nullptr;
    if (mapped != nullptr) {
        std::memcpy(mapped, pixels, size * size);
        m_fontStagingBuffer->unmap();
    }

    // 转换图像布局
    m_fontTexture->transitionLayout(commandBuffer,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲到图像
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {size, size, 1};

    vkCmdCopyBufferToImage(commandBuffer, m_fontStagingBuffer->buffer(),
                           m_fontTexture->image(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换到着色器读取布局
    m_fontTexture->transitionLayout(commandBuffer,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void GuiRenderer::initializeTextureLayouts(VkCommandBuffer commandBuffer) {
    // 初始化字体纹理布局
    // 注意：从 UNDEFINED 布局转换时，源阶段使用 TOP_OF_PIPE，但访问掩码为0
    if (m_fontTexture && m_fontTexture->image() != VK_NULL_HANDLE) {
        m_fontTexture->transitionLayout(commandBuffer,
                                        VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    // 初始化物品占位纹理布局
    if (m_itemPlaceholderTexture && m_itemPlaceholderTexture->image() != VK_NULL_HANDLE) {
        m_itemPlaceholderTexture->transitionLayout(commandBuffer,
                                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
}

void GuiRenderer::uploadBufferData(VkCommandBuffer commandBuffer) {
    if (m_vertices.empty() && m_indices.empty()) return;

    VkDeviceSize vertexSize = m_vertices.size() * sizeof(GuiVertex);
    VkDeviceSize indexSize = m_indices.size() * sizeof(u32);

    VkDevice device = m_device;
    VkPhysicalDevice physicalDevice = m_physicalDevice;

    // 若顶点数据超出缓冲容量，则以2倍所需大小重建缓冲
    if (vertexSize > m_vertexBuffer->size()) {
        m_vertexBuffer->destroy();
        auto result = m_vertexBuffer->create(device, physicalDevice,
                                              vertexSize * 2,
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!result.success()) {
            spdlog::error("GuiRenderer: failed to reallocate vertex buffer ({}B)", vertexSize);
            return;
        }
    }

    // 若索引数据超出缓冲容量，则以2倍所需大小重建缓冲
    if (indexSize > m_indexBuffer->size()) {
        m_indexBuffer->destroy();
        auto result = m_indexBuffer->create(device, physicalDevice,
                                             indexSize * 2,
                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!result.success()) {
            spdlog::error("GuiRenderer: failed to reallocate index buffer ({}B)", indexSize);
            return;
        }
    }

    // 直接映射顶点缓冲并复制数据
    auto vertexMapResult = m_vertexBuffer->map();
    if (vertexMapResult.success()) {
        void* vertexMapped = vertexMapResult.value();
        std::memcpy(vertexMapped, m_vertices.data(), vertexSize);
        m_vertexBuffer->unmap();
    }

    // 直接映射索引缓冲并复制数据
    auto indexMapResult = m_indexBuffer->map();
    if (indexMapResult.success()) {
        void* indexMapped = indexMapResult.value();
        std::memcpy(indexMapped, m_indices.data(), indexSize);
        m_indexBuffer->unmap();
    }

    // 由于使用HOST_COHERENT内存，不需要手动flush
    // 数据会自动对GPU可见
    (void)commandBuffer; // 不再需要commandBuffer
}

void GuiRenderer::setItemTextureAtlas(VkImageView itemView, VkSampler itemSampler) {
    if (!m_initialized || itemView == VK_NULL_HANDLE || itemSampler == VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_device;

    // 更新 binding 1 的描述符
    VkDescriptorImageInfo itemImageInfo = {};
    itemImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    itemImageInfo.imageView = itemView;
    itemImageInfo.sampler = itemSampler;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 1;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &itemImageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

} // namespace mc::client
