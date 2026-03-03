#include "VulkanPipeline.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>

namespace mr::client {

VulkanPipeline::VulkanPipeline() = default;

VulkanPipeline::~VulkanPipeline() {
    destroy();
}

Result<void> VulkanPipeline::initialize(VulkanContext* context, const PipelineConfig& config) {
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Pipeline already initialized");
    }

    m_context = context;

    // 创建管线布局
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<u32>(config.descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = config.descriptorSetLayouts.empty() ? nullptr : config.descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = static_cast<u32>(config.pushConstantRanges.size());
    layoutInfo.pPushConstantRanges = config.pushConstantRanges.empty() ? nullptr : config.pushConstantRanges.data();

    VkResult result = vkCreatePipelineLayout(m_context->device(), &layoutInfo, nullptr, &m_layout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create pipeline layout: " + std::to_string(result));
    }

    // 加载着色器
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    if (!config.vertexShaderPath.empty()) {
        auto shaderResult = loadShader(config.vertexShaderPath, ShaderStage::Vertex);
        if (shaderResult.failed()) {
            destroy();
            return shaderResult.error();
        }
        m_shaders.push_back(shaderResult.value());
    }

    if (!config.fragmentShaderPath.empty()) {
        auto shaderResult = loadShader(config.fragmentShaderPath, ShaderStage::Fragment);
        if (shaderResult.failed()) {
            destroy();
            return shaderResult.error();
        }
        m_shaders.push_back(shaderResult.value());
    }

    // 着色器阶段
    for (const auto& shader : m_shaders) {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = shader.stage;
        stageInfo.module = shader.module;
        stageInfo.pName = shader.entryPoint.c_str();
        shaderStages.push_back(stageInfo);
    }

    // 顶点输入
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<u32>(config.vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = config.vertexBindings.empty() ? nullptr : config.vertexBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(config.vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = config.vertexAttributes.empty() ? nullptr : config.vertexAttributes.data();

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = config.topology;
    inputAssembly.primitiveRestartEnable = config.primitiveRestartEnable;

    // 视口和裁剪 (动态)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // 动态
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr; // 动态

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = config.polygonMode;
    rasterizer.lineWidth = config.lineWidth;
    rasterizer.cullMode = config.cullMode;
    rasterizer.frontFace = config.frontFace;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = config.sampleShadingEnable;
    multisampling.rasterizationSamples = config.rasterizationSamples;
    multisampling.minSampleShading = config.minSampleShading;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // 深度/模板
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = config.depthTestEnable;
    depthStencil.depthWriteEnable = config.depthWriteEnable;
    depthStencil.depthCompareOp = config.depthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = config.stencilTestEnable;
    depthStencil.front = {};
    depthStencil.back = {};

    // 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = config.colorWriteMask;
    colorBlendAttachment.blendEnable = config.blendEnable;
    colorBlendAttachment.srcColorBlendFactor = config.srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = config.dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = config.colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = config.srcAlphaBlendFactor;
    colorBlendAttachment.dstAlphaBlendFactor = config.dstAlphaBlendFactor;
    colorBlendAttachment.alphaBlendOp = config.alphaBlendOp;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // 动态状态
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 管线创建
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<u32>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_layout;
    pipelineInfo.renderPass = config.renderPass;
    pipelineInfo.subpass = config.subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(m_context->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        destroy();
        return Error(ErrorCode::Unknown, "Failed to create graphics pipeline: " + std::to_string(result));
    }

    // 销毁着色器模块 (管线已创建，不再需要)
    for (auto& shader : m_shaders) {
        destroyShader(shader);
    }
    m_shaders.clear();

    m_initialized = true;
    spdlog::info("Graphics pipeline created");
    return Result<void>::ok();
}

void VulkanPipeline::destroy() {
    if (!m_initialized) {
        return;
    }

    for (auto& shader : m_shaders) {
        destroyShader(shader);
    }
    m_shaders.clear();

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_context->device(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_context->device(), m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }

    m_context = nullptr;
    m_initialized = false;
}

Result<ShaderModule> VulkanPipeline::loadShader(const String& path, ShaderStage stage) {
    // 读取SPIR-V文件
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, "Failed to open shader file: " + path);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<u8> code(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), fileSize);
    file.close();

    auto moduleResult = createShaderModule(m_context, code);
    if (moduleResult.failed()) {
        return moduleResult.error();
    }

    ShaderModule shader;
    shader.module = moduleResult.value();
    shader.stage = stage == ShaderStage::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
    shader.entryPoint = "main";

    spdlog::info("Loaded shader: {} ({} bytes)", path, fileSize);
    return shader;
}

void VulkanPipeline::destroyShader(ShaderModule& shader) {
    if (shader.module != VK_NULL_HANDLE && m_context != nullptr) {
        vkDestroyShaderModule(m_context->device(), shader.module, nullptr);
        shader.module = VK_NULL_HANDLE;
    }
}

void VulkanPipeline::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

Result<VkShaderModule> VulkanPipeline::createShaderModule(VulkanContext* context, const std::vector<u8>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(context->device(), &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create shader module: " + std::to_string(result));
    }

    return shaderModule;
}

// ============================================================================
// VulkanPipelineCache 实现
// ============================================================================

VulkanPipelineCache::VulkanPipelineCache() = default;

VulkanPipelineCache::~VulkanPipelineCache() {
    destroy();
}

Result<void> VulkanPipelineCache::initialize(VulkanContext* context, const String& cachePath) {
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Pipeline cache already initialized");
    }

    m_context = context;
    m_cachePath = cachePath;

    // 尝试从文件加载缓存
    std::vector<u8> cacheData;
    if (!cachePath.empty()) {
        std::ifstream file(cachePath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            size_t fileSize = static_cast<size_t>(file.tellg());
            cacheData.resize(fileSize);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(cacheData.data()), fileSize);
            spdlog::info("Loaded pipeline cache from: {} ({} bytes)", cachePath, fileSize);
        }
    }

    VkPipelineCacheCreateInfo cacheInfo{};
    cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cacheInfo.initialDataSize = cacheData.size();
    cacheInfo.pInitialData = cacheData.empty() ? nullptr : cacheData.data();

    VkResult result = vkCreatePipelineCache(context->device(), &cacheInfo, nullptr, &m_cache);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create pipeline cache: " + std::to_string(result));
    }

    m_initialized = true;
    return Result<void>::ok();
}

void VulkanPipelineCache::destroy() {
    if (!m_initialized) {
        return;
    }

    // 保存缓存到文件
    if (!m_cachePath.empty() && m_cache != VK_NULL_HANDLE) {
        size_t dataSize = 0;
        vkGetPipelineCacheData(m_context->device(), m_cache, &dataSize, nullptr);

        if (dataSize > 0) {
            std::vector<u8> cacheData(dataSize);
            vkGetPipelineCacheData(m_context->device(), m_cache, &dataSize, cacheData.data());

            std::ofstream file(m_cachePath, std::ios::binary);
            if (file.is_open()) {
                file.write(reinterpret_cast<const char*>(cacheData.data()), cacheData.size());
                spdlog::info("Saved pipeline cache to: {} ({} bytes)", m_cachePath, cacheData.size());
            }
        }
    }

    if (m_cache != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(m_context->device(), m_cache, nullptr);
        m_cache = VK_NULL_HANDLE;
    }

    m_context = nullptr;
    m_initialized = false;
}

} // namespace mr::client
