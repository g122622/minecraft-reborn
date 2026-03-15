#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace mc::client {

// 着色器类型
enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    Geometry,
    TessellationControl,
    TessellationEvaluation
};

// 着色器模块
struct ShaderModule {
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderStageFlagBits stage;
    String entryPoint = "main";
};

// 管线配置
struct PipelineConfig {
    // 着色器
    String vertexShaderPath;
    String fragmentShaderPath;

    // 顶点输入
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

    // 输入装配
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkBool32 primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪 (动态设置)
    bool dynamicViewport = true;
    bool dynamicScissor = true;

    // 光栅化
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
    float lineWidth = 1.0f;

    // 多重采样
    VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkBool32 sampleShadingEnable = VK_FALSE;
    float minSampleShading = 1.0f;

    // 深度/模板
    VkBool32 depthTestEnable = VK_TRUE;
    VkBool32 depthWriteEnable = VK_TRUE;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    VkBool32 stencilTestEnable = VK_FALSE;

    // 颜色混合
    VkBool32 blendEnable = VK_FALSE;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
    VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;

    // 渲染通道
    VkRenderPass renderPass = VK_NULL_HANDLE;
    u32 subpass = 0;

    // 管线布局
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
};

// 图形管线
class VulkanPipeline {
public:
    VulkanPipeline();
    ~VulkanPipeline();

    // 禁止拷贝
    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    // 初始化
    [[nodiscard]] Result<void> initialize(VkDevice device, const PipelineConfig& config);
    void destroy();

    // 着色器加载
    [[nodiscard]] Result<ShaderModule> loadShader(const String& path, ShaderStage stage);
    void destroyShader(ShaderModule& shader);

    // 管线绑定
    void bind(VkCommandBuffer commandBuffer);

    // 信息
    VkPipeline pipeline() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_layout; }
    VkPipelineLayout pipelineLayout() const { return m_layout; }

    // 静态创建函数
    [[nodiscard]] static Result<VkShaderModule> createShaderModule(VkDevice device, const std::vector<u8>& code);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;

    std::vector<ShaderModule> m_shaders;
    bool m_initialized = false;
};

// 管线缓存 (用于加速管线创建)
class VulkanPipelineCache {
public:
    VulkanPipelineCache();
    ~VulkanPipelineCache();

    [[nodiscard]] Result<void> initialize(VkDevice device, const String& cachePath = "");
    void destroy();

    VkPipelineCache cache() const { return m_cache; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipelineCache m_cache = VK_NULL_HANDLE;
    String m_cachePath;
    bool m_initialized = false;
};

} // namespace mc::client
