#include "CloudRenderer.hpp"
#include "../util/VulkanUtils.hpp"
#include "../../util/ShaderPath.hpp"
#include "../../../resource/ResourceManager.hpp"
#include "../../../../common/math/MathUtils.hpp"
#include "../../../../common/math/random/Random.hpp"
#include "../../../../common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <array>
#include <vector>

namespace mc::client::renderer::trident::cloud {

namespace {

// 云纹理尺寸
constexpr u32 CLOUD_TEXTURE_SIZE = 256;

// 云动画速度 (MC 1.16.5: 0.03F per tick)
constexpr f32 CLOUD_SPEED = 0.03f;

// 云网格范围 (-3 到 +4 区块，每区块 8 格)
constexpr i32 CLOUD_RANGE_MIN = -3;
constexpr i32 CLOUD_RANGE_MAX = 4;
constexpr i32 CLOUD_CHUNK_SIZE = 8;

// 云纹理缩放
constexpr f32 CLOUD_TEXTURE_SCALE = 0.00390625f; // 1/256

// 云层厚度
constexpr f32 CLOUD_THICKNESS = 4.0f;

// 云透明度
constexpr f32 CLOUD_ALPHA = 0.8f;

// 云顶偏移量（避免 z-fighting）
constexpr f32 CLOUD_TOP_OFFSET = 0.0009765625f; // 约 1/1024

Result<std::vector<u8>> readBinaryFile(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, "Failed to open shader file: " + std::string(path));
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        return Error(ErrorCode::InvalidData, std::string("Shader file is empty: ") + path);
    }

    std::vector<u8> data(static_cast<size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    if (!file.good()) {
        return Error(ErrorCode::Unknown, "Failed to read shader file: " + std::string(path));
    }

    return data;
}

Result<VkShaderModule> createShaderModule(VkDevice device, const char* path) {
    auto codeResult = readBinaryFile(path);
    if (codeResult.failed()) {
        return codeResult.error();
    }

    const auto& code = codeResult.value();
    if (code.size() % 4 != 0) {
        return Error(ErrorCode::InvalidData, "Invalid SPIR-V file size: " + std::string(path));
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    const VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to create shader module: " + std::string(path));
    }

    return shaderModule;
}

} // namespace

// ============================================================================
// 顶点结构
// ============================================================================

/**
 * @brief 云顶点结构
 */
struct CloudVertex {
    f32 x, y, z;       // 位置
    f32 u, v;          // 纹理坐标
    f32 nx, ny, nz;    // 法线
};

// ============================================================================
// 构造/析构
// ============================================================================

CloudRenderer::CloudRenderer() = default;

CloudRenderer::~CloudRenderer() {
    destroy();
}

CloudRenderer::CloudRenderer(CloudRenderer&& other) noexcept
    : m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_commandPool(other.m_commandPool)
    , m_graphicsQueue(other.m_graphicsQueue)
    , m_renderPass(other.m_renderPass)
    , m_extent(other.m_extent)
    , m_initialized(other.m_initialized)
    , m_descriptorSetLayout(other.m_descriptorSetLayout)
    , m_descriptorPool(other.m_descriptorPool)
    , m_pipelineLayout(other.m_pipelineLayout)
    , m_fastPipeline(other.m_fastPipeline)
    , m_fancyPipeline(other.m_fancyPipeline)
    , m_fastVBO(other.m_fastVBO)
    , m_fastVBOMemory(other.m_fastVBOMemory)
    , m_fastVertexCount(other.m_fastVertexCount)
    , m_fancyVBO(other.m_fancyVBO)
    , m_fancyVBOMemory(other.m_fancyVBOMemory)
    , m_fancyVertexCount(other.m_fancyVertexCount)
    , m_textureImage(other.m_textureImage)
    , m_textureImageMemory(other.m_textureImageMemory)
    , m_textureImageView(other.m_textureImageView)
    , m_textureSampler(other.m_textureSampler)
    , m_cloudMode(other.m_cloudMode)
    , m_cloudHeight(other.m_cloudHeight)
    , m_cloudColor(other.m_cloudColor)
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_uniformBuffers[i] = other.m_uniformBuffers[i];
        m_uniformBuffersMemory[i] = other.m_uniformBuffersMemory[i];
        m_uniformBuffersMapped[i] = other.m_uniformBuffersMapped[i];
        m_descriptorSets[i] = other.m_descriptorSets[i];
    }

    // 清除源对象
    other.m_device = VK_NULL_HANDLE;
    other.m_initialized = false;
}

CloudRenderer& CloudRenderer::operator=(CloudRenderer&& other) noexcept {
    if (this != &other) {
        destroy();

        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_commandPool = other.m_commandPool;
        m_graphicsQueue = other.m_graphicsQueue;
        m_renderPass = other.m_renderPass;
        m_extent = other.m_extent;
        m_initialized = other.m_initialized;
        m_descriptorSetLayout = other.m_descriptorSetLayout;
        m_descriptorPool = other.m_descriptorPool;
        m_pipelineLayout = other.m_pipelineLayout;
        m_fastPipeline = other.m_fastPipeline;
        m_fancyPipeline = other.m_fancyPipeline;
        m_fastVBO = other.m_fastVBO;
        m_fastVBOMemory = other.m_fastVBOMemory;
        m_fastVertexCount = other.m_fastVertexCount;
        m_fancyVBO = other.m_fancyVBO;
        m_fancyVBOMemory = other.m_fancyVBOMemory;
        m_fancyVertexCount = other.m_fancyVertexCount;
        m_textureImage = other.m_textureImage;
        m_textureImageMemory = other.m_textureImageMemory;
        m_textureImageView = other.m_textureImageView;
        m_textureSampler = other.m_textureSampler;
        m_cloudMode = other.m_cloudMode;
        m_cloudHeight = other.m_cloudHeight;
        m_cloudColor = other.m_cloudColor;

        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_uniformBuffers[i] = other.m_uniformBuffers[i];
            m_uniformBuffersMemory[i] = other.m_uniformBuffersMemory[i];
            m_uniformBuffersMapped[i] = other.m_uniformBuffersMapped[i];
            m_descriptorSets[i] = other.m_descriptorSets[i];
        }

        other.m_device = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> CloudRenderer::initialize(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    VkExtent2D extent,
    const ResourceManager* resourceManager) {
    if (m_initialized) {
        return Result<void>::ok();
    }

    MC_TRACE_EVENT("render", "CloudRenderer::initialize");

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    m_extent = extent;

    // 创建顶点缓冲区
    auto result1 = createCloudVBO();
    if (result1.failed()) {
        return result1;
    }

    // 创建纹理
    auto result2 = createTexture(resourceManager);
    if (result2.failed()) {
        return result2;
    }

    // 创建 Uniform 缓冲区
    auto result3 = createUniformBuffers();
    if (result3.failed()) {
        return result3;
    }

    // 创建描述符
    auto result4 = createDescriptorSetLayout();
    if (result4.failed()) {
        return result4;
    }

    auto result5 = createDescriptorSets();
    if (result5.failed()) {
        return result5;
    }

    // 创建管线
    auto result6 = createPipelineLayout();
    if (result6.failed()) {
        return result6;
    }

    auto result7 = createPipelines();
    if (result7.failed()) {
        return result7;
    }

    m_initialized = true;
    spdlog::info("CloudRenderer initialized");
    return Result<void>::ok();
}

void CloudRenderer::destroy() {
    if (m_device == VK_NULL_HANDLE) return;

    VkDevice device = m_device;

    // 等待设备空闲
    vkDeviceWaitIdle(device);

    // 销毁管线
    if (m_fastPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_fastPipeline, nullptr);
        m_fastPipeline = VK_NULL_HANDLE;
    }
    if (m_fancyPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_fancyPipeline, nullptr);
        m_fancyPipeline = VK_NULL_HANDLE;
    }

    // 销毁管线布局
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    // 销毁描述符池
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    // 销毁描述符集布局
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    // 销毁 Uniform 缓冲区
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (m_uniformBuffers[i] != VK_NULL_HANDLE) {
            vkUnmapMemory(device, m_uniformBuffersMemory[i]);
            vkDestroyBuffer(device, m_uniformBuffers[i], nullptr);
            vkFreeMemory(device, m_uniformBuffersMemory[i], nullptr);
            m_uniformBuffers[i] = VK_NULL_HANDLE;
            m_uniformBuffersMemory[i] = VK_NULL_HANDLE;
            m_uniformBuffersMapped[i] = nullptr;
        }
    }

    // 销毁顶点缓冲区
    if (m_fastVBO != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_fastVBO, nullptr);
        m_fastVBO = VK_NULL_HANDLE;
    }
    if (m_fastVBOMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_fastVBOMemory, nullptr);
        m_fastVBOMemory = VK_NULL_HANDLE;
    }
    if (m_fancyVBO != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_fancyVBO, nullptr);
        m_fancyVBO = VK_NULL_HANDLE;
    }
    if (m_fancyVBOMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_fancyVBOMemory, nullptr);
        m_fancyVBOMemory = VK_NULL_HANDLE;
    }

    // 销毁纹理
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

    m_initialized = false;
    spdlog::debug("CloudRenderer destroyed");
}

Result<void> CloudRenderer::onResize(VkExtent2D extent) {
    m_extent = extent;
    return Result<void>::ok();
}

Result<void> CloudRenderer::reloadTexture(const ResourceManager* resourceManager) {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "CloudRenderer is not initialized");
    }

    vkDeviceWaitIdle(m_device);

    auto textureResult = createTexture(resourceManager);
    if (textureResult.failed()) {
        return textureResult;
    }

    updateTextureDescriptors();
    return Result<void>::ok();
}

// ============================================================================
// 更新与渲染
// ============================================================================

void CloudRenderer::update(i64 dayTime, i64 gameTime, f32 partialTick,
                           f32 cloudHeight, const glm::vec4& cloudColor) {
    m_dayTime = dayTime;
    m_gameTime = gameTime;
    m_partialTick = partialTick;
    m_cloudHeight = cloudHeight;
    m_cloudColor = cloudColor;

    // 计算云偏移（云随时间移动）
    // 参考 MC 1.16.5: d1 = (ticks + partialTick) * 0.03
    f32 totalTicks = static_cast<f32>(gameTime) + partialTick;
    m_cloudOffsetX = totalTicks * CLOUD_SPEED;
    m_cloudOffsetZ = 0.0f; // 云只沿 X 轴移动
}

void CloudRenderer::render(VkCommandBuffer cmd,
                           const glm::mat4& projection,
                           const glm::mat4& view,
                           const glm::vec3& cameraPos,
                           CloudMode mode,
                           u32 frameIndex) {
    if (!m_initialized || mode == CloudMode::Off || std::isnan(m_cloudHeight)) {
        return;
    }

    MC_TRACE_EVENT("render", "CloudRenderer::render");

    m_cameraPos = cameraPos;

    // 更新 Uniform 缓冲区
    updateUniformBuffer(frameIndex);

    // 选择管线
    VkPipeline pipeline = (mode == CloudMode::Fast) ? m_fastPipeline : m_fancyPipeline;
    VkBuffer vbo = (mode == CloudMode::Fast) ? m_fastVBO : m_fancyVBO;
    u32 vertexCount = (mode == CloudMode::Fast) ? m_fastVertexCount : m_fancyVertexCount;

    if (pipeline == VK_NULL_HANDLE || vbo == VK_NULL_HANDLE || vertexCount == 0) {
        return;
    }

    // 设置视口和裁剪区域（管线使用动态状态）
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(m_extent.width);
    viewport.height = static_cast<f32>(m_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // 绑定管线
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // 绑定描述符集
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 1,
                            &m_descriptorSets[frameIndex], 0, nullptr);

    // 绑定顶点缓冲区
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &vbo, offsets);

    // 推送常量：视图-投影矩阵
    // 云层应当围绕相机渲染（无限远平铺效果），因此仅保留视图旋转，移除平移分量。
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
    glm::mat4 viewProjection = projection * viewNoTranslation;

    // 云的变换矩阵
    // 参考 MC 1.16.5 WorldRenderer.renderClouds():
    // d1 = (ticks + partialTicks) * 0.03F  (动画偏移)
    // d2 = (viewEntityX + d1) / 12.0D      (X 坐标，包含动画)
    // d3 = cloudHeight - viewEntityY + 0.33F (Y 坐标，相对相机)
    // d4 = viewEntityZ / 12.0D + 0.33D     (Z 坐标)
    // 然后取模 2048 防止浮点精度问题
    // 最后 scale(12, 1, 12) 并 translate(-fracX, fracY, -fracZ)

    // 计算动画偏移
    f32 animOffset = (static_cast<f32>(m_gameTime) + m_partialTick) * CLOUD_SPEED;

    // 计算 X/Z 坐标（包含相机位置和动画）
    f32 cloudX = (cameraPos.x + animOffset) / 12.0f;
    f32 cloudZ = cameraPos.z / 12.0f + 0.33f;

    // 取模防止浮点精度问题
    cloudX = std::fmod(cloudX, 2048.0f);
    cloudZ = std::fmod(cloudZ, 2048.0f);

    // 分数部分
    f32 fracX = cloudX - std::floor(cloudX);
    f32 fracZ = cloudZ - std::floor(cloudZ);

    // Y 坐标（相对相机）
    f32 cloudY = m_cloudHeight - cameraPos.y + 0.33f;

    // 变换：先缩放，再平移（保持与 MC 云网格坐标一致）
    // 注意：X/Z 的平移在缩放后生效（对应每格 12 世界单位）。
    // Y 必须使用完整相对高度，否则云层会错误地贴近世界原点附近。
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(12.0f, 1.0f, 12.0f));
    model = glm::translate(model, glm::vec3(-fracX, cloudY, -fracZ));

    glm::mat4 mvp = viewProjection * model;
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(glm::mat4), &mvp);

    // 绘制
    vkCmdDraw(cmd, vertexCount, 1, 0, 0);
}

// ============================================================================
// 资源创建
// ============================================================================

Result<void> CloudRenderer::createCloudVBO() {
    // 生成 Fast 模式顶点（单层平面）
    {
        std::vector<CloudVertex> vertices;

        // Fast 模式：只渲染底面
        // 参考 MC 1.16.5 drawClouds() Fast 分支
        // 范围：-32 到 +32，步长 32
        for (i32 x = -32; x < 32; x += 32) {
            for (i32 z = -32; z < 32; z += 32) {
                // 四个顶点
                CloudVertex v0, v1, v2, v3;

                // 位置
                v0.x = static_cast<f32>(x);
                v0.y = 0.0f;
                v0.z = static_cast<f32>(z + 32);

                v1.x = static_cast<f32>(x + 32);
                v1.y = 0.0f;
                v1.z = static_cast<f32>(z + 32);

                v2.x = static_cast<f32>(x + 32);
                v2.y = 0.0f;
                v2.z = static_cast<f32>(z);

                v3.x = static_cast<f32>(x);
                v3.y = 0.0f;
                v3.z = static_cast<f32>(z);

                // 纹理坐标（使用纹理缩放）
                v0.u = static_cast<f32>(x) * CLOUD_TEXTURE_SCALE;
                v0.v = static_cast<f32>(z + 32) * CLOUD_TEXTURE_SCALE;

                v1.u = static_cast<f32>(x + 32) * CLOUD_TEXTURE_SCALE;
                v1.v = static_cast<f32>(z + 32) * CLOUD_TEXTURE_SCALE;

                v2.u = static_cast<f32>(x + 32) * CLOUD_TEXTURE_SCALE;
                v2.v = static_cast<f32>(z) * CLOUD_TEXTURE_SCALE;

                v3.u = static_cast<f32>(x) * CLOUD_TEXTURE_SCALE;
                v3.v = static_cast<f32>(z) * CLOUD_TEXTURE_SCALE;

                // 法线（底面朝下）
                v0.nx = v1.nx = v2.nx = v3.nx = 0.0f;
                v0.ny = v1.ny = v2.ny = v3.ny = -1.0f;
                v0.nz = v1.nz = v2.nz = v3.nz = 0.0f;

                // 两个三角形
                vertices.push_back(v0);
                vertices.push_back(v1);
                vertices.push_back(v2);

                vertices.push_back(v0);
                vertices.push_back(v2);
                vertices.push_back(v3);
            }
        }

        m_fastVertexCount = static_cast<u32>(vertices.size());

        // 创建缓冲区
        VkDeviceSize bufferSize = sizeof(CloudVertex) * vertices.size();
        auto result = createBuffer(bufferSize,
                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    m_fastVBO, m_fastVBOMemory);
        if (result.failed()) {
            return result;
        }

        // 上传数据
        void* data;
        vkMapMemory(m_device, m_fastVBOMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices.data(), bufferSize);
        vkUnmapMemory(m_device, m_fastVBOMemory);
    }

    // 生成 Fancy 模式顶点（3D 立方体）
    {
        std::vector<CloudVertex> vertices;

        // Fancy 模式：渲染完整 3D 云体
        // 参考 MC 1.16.5 drawClouds() Fancy 分支
        // 范围：-3 到 +4 区块，每区块 8 格
        for (i32 chunkX = CLOUD_RANGE_MIN; chunkX <= CLOUD_RANGE_MAX; ++chunkX) {
            for (i32 chunkZ = CLOUD_RANGE_MIN; chunkZ <= CLOUD_RANGE_MAX; ++chunkZ) {
                f32 baseX = static_cast<f32>(chunkX * CLOUD_CHUNK_SIZE);
                f32 baseZ = static_cast<f32>(chunkZ * CLOUD_CHUNK_SIZE);

                // 云的颜色变体（模拟云层厚度）
                // 参考 MC: 顶部颜色、底部颜色、侧面颜色

                // 底面（朝下）
                {
                    CloudVertex v0, v1, v2, v3;
                    v0.x = baseX;
                    v0.y = 0.0f;
                    v0.z = baseZ + CLOUD_CHUNK_SIZE;

                    v1.x = baseX + CLOUD_CHUNK_SIZE;
                    v1.y = 0.0f;
                    v1.z = baseZ + CLOUD_CHUNK_SIZE;

                    v2.x = baseX + CLOUD_CHUNK_SIZE;
                    v2.y = 0.0f;
                    v2.z = baseZ;

                    v3.x = baseX;
                    v3.y = 0.0f;
                    v3.z = baseZ;

                    // 纹理坐标
                    v0.u = baseX * CLOUD_TEXTURE_SCALE;
                    v0.v = (baseZ + CLOUD_CHUNK_SIZE) * CLOUD_TEXTURE_SCALE;

                    v1.u = (baseX + CLOUD_CHUNK_SIZE) * CLOUD_TEXTURE_SCALE;
                    v1.v = (baseZ + CLOUD_CHUNK_SIZE) * CLOUD_TEXTURE_SCALE;

                    v2.u = (baseX + CLOUD_CHUNK_SIZE) * CLOUD_TEXTURE_SCALE;
                    v2.v = baseZ * CLOUD_TEXTURE_SCALE;

                    v3.u = baseX * CLOUD_TEXTURE_SCALE;
                    v3.v = baseZ * CLOUD_TEXTURE_SCALE;

                    // 法线（底面朝下）
                    v0.nx = v1.nx = v2.nx = v3.nx = 0.0f;
                    v0.ny = v1.ny = v2.ny = v3.ny = -1.0f;
                    v0.nz = v1.nz = v2.nz = v3.nz = 0.0f;

                    vertices.push_back(v0);
                    vertices.push_back(v1);
                    vertices.push_back(v2);
                    vertices.push_back(v0);
                    vertices.push_back(v2);
                    vertices.push_back(v3);
                }

                // 顶面（朝上）
                {
                    CloudVertex v0, v1, v2, v3;
                    v0.x = baseX;
                    v0.y = CLOUD_THICKNESS - CLOUD_TOP_OFFSET;
                    v0.z = baseZ + CLOUD_CHUNK_SIZE;

                    v1.x = baseX + CLOUD_CHUNK_SIZE;
                    v1.y = CLOUD_THICKNESS - CLOUD_TOP_OFFSET;
                    v1.z = baseZ + CLOUD_CHUNK_SIZE;

                    v2.x = baseX + CLOUD_CHUNK_SIZE;
                    v2.y = CLOUD_THICKNESS - CLOUD_TOP_OFFSET;
                    v2.z = baseZ;

                    v3.x = baseX;
                    v3.y = CLOUD_THICKNESS - CLOUD_TOP_OFFSET;
                    v3.z = baseZ;

                    // 纹理坐标
                    v0.u = baseX * CLOUD_TEXTURE_SCALE;
                    v0.v = (baseZ + CLOUD_CHUNK_SIZE) * CLOUD_TEXTURE_SCALE;

                    v1.u = (baseX + CLOUD_CHUNK_SIZE) * CLOUD_TEXTURE_SCALE;
                    v1.v = (baseZ + CLOUD_CHUNK_SIZE) * CLOUD_TEXTURE_SCALE;

                    v2.u = (baseX + CLOUD_CHUNK_SIZE) * CLOUD_TEXTURE_SCALE;
                    v2.v = baseZ * CLOUD_TEXTURE_SCALE;

                    v3.u = baseX * CLOUD_TEXTURE_SCALE;
                    v3.v = baseZ * CLOUD_TEXTURE_SCALE;

                    // 法线（顶面朝上）
                    v0.nx = v1.nx = v2.nx = v3.nx = 0.0f;
                    v0.ny = v1.ny = v2.ny = v3.ny = 1.0f;
                    v0.nz = v1.nz = v2.nz = v3.nz = 0.0f;

                    // 注意：顶面三角形顺序相反
                    vertices.push_back(v0);
                    vertices.push_back(v2);
                    vertices.push_back(v1);
                    vertices.push_back(v0);
                    vertices.push_back(v3);
                    vertices.push_back(v2);
                }

                // 四个侧面（仅在区块边缘渲染）
                // 西面 (X = chunkX * 8)
                if (chunkX > CLOUD_RANGE_MIN) {
                    for (i32 i = 0; i < CLOUD_CHUNK_SIZE; ++i) {
                        CloudVertex v0, v1, v2, v3;
                        v0.x = baseX;
                        v0.y = 0.0f;
                        v0.z = baseZ + static_cast<f32>(i);

                        v1.x = baseX;
                        v1.y = CLOUD_THICKNESS;
                        v1.z = baseZ + static_cast<f32>(i);

                        v2.x = baseX;
                        v2.y = CLOUD_THICKNESS;
                        v2.z = baseZ + static_cast<f32>(i + 1);

                        v3.x = baseX;
                        v3.y = 0.0f;
                        v3.z = baseZ + static_cast<f32>(i + 1);

                        // 纹理坐标
                        v0.u = (baseX + 0.5f) * CLOUD_TEXTURE_SCALE;
                        v0.v = (baseZ + static_cast<f32>(i) + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v1.u = (baseX + 0.5f) * CLOUD_TEXTURE_SCALE;
                        v1.v = (baseZ + static_cast<f32>(i) + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v2.u = (baseX + 0.5f) * CLOUD_TEXTURE_SCALE;
                        v2.v = (baseZ + static_cast<f32>(i + 1) + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v3.u = (baseX + 0.5f) * CLOUD_TEXTURE_SCALE;
                        v3.v = (baseZ + static_cast<f32>(i + 1) + 0.5f) * CLOUD_TEXTURE_SCALE;

                        // 法线（朝西）
                        v0.nx = v1.nx = v2.nx = v3.nx = -1.0f;
                        v0.ny = v1.ny = v2.ny = v3.ny = 0.0f;
                        v0.nz = v1.nz = v2.nz = v3.nz = 0.0f;

                        vertices.push_back(v0);
                        vertices.push_back(v1);
                        vertices.push_back(v2);
                        vertices.push_back(v0);
                        vertices.push_back(v2);
                        vertices.push_back(v3);
                    }
                }

                // 东面 (X = chunkX * 8 + 8)
                if (chunkX < CLOUD_RANGE_MAX) {
                    for (i32 i = 0; i < CLOUD_CHUNK_SIZE; ++i) {
                        CloudVertex v0, v1, v2, v3;
                        f32 x = baseX + CLOUD_CHUNK_SIZE - CLOUD_TOP_OFFSET;

                        v0.x = x;
                        v0.y = 0.0f;
                        v0.z = baseZ + static_cast<f32>(i);

                        v1.x = x;
                        v1.y = CLOUD_THICKNESS;
                        v1.z = baseZ + static_cast<f32>(i);

                        v2.x = x;
                        v2.y = CLOUD_THICKNESS;
                        v2.z = baseZ + static_cast<f32>(i + 1);

                        v3.x = x;
                        v3.y = 0.0f;
                        v3.z = baseZ + static_cast<f32>(i + 1);

                        // 纹理坐标
                        v0.u = (baseX + CLOUD_CHUNK_SIZE - 0.5f) * CLOUD_TEXTURE_SCALE;
                        v0.v = (baseZ + static_cast<f32>(i) + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v1.u = (baseX + CLOUD_CHUNK_SIZE - 0.5f) * CLOUD_TEXTURE_SCALE;
                        v1.v = (baseZ + static_cast<f32>(i) + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v2.u = (baseX + CLOUD_CHUNK_SIZE - 0.5f) * CLOUD_TEXTURE_SCALE;
                        v2.v = (baseZ + static_cast<f32>(i + 1) + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v3.u = (baseX + CLOUD_CHUNK_SIZE - 0.5f) * CLOUD_TEXTURE_SCALE;
                        v3.v = (baseZ + static_cast<f32>(i + 1) + 0.5f) * CLOUD_TEXTURE_SCALE;

                        // 法线（朝东）
                        v0.nx = v1.nx = v2.nx = v3.nx = 1.0f;
                        v0.ny = v1.ny = v2.ny = v3.ny = 0.0f;
                        v0.nz = v1.nz = v2.nz = v3.nz = 0.0f;

                        vertices.push_back(v0);
                        vertices.push_back(v2);
                        vertices.push_back(v1);
                        vertices.push_back(v0);
                        vertices.push_back(v3);
                        vertices.push_back(v2);
                    }
                }

                // 北面 (Z = chunkZ * 8)
                if (chunkZ > CLOUD_RANGE_MIN) {
                    for (i32 i = 0; i < CLOUD_CHUNK_SIZE; ++i) {
                        CloudVertex v0, v1, v2, v3;
                        v0.x = baseX + static_cast<f32>(i);
                        v0.y = CLOUD_THICKNESS;
                        v0.z = baseZ;

                        v1.x = baseX + static_cast<f32>(i + 1);
                        v1.y = CLOUD_THICKNESS;
                        v1.z = baseZ;

                        v2.x = baseX + static_cast<f32>(i + 1);
                        v2.y = 0.0f;
                        v2.z = baseZ;

                        v3.x = baseX + static_cast<f32>(i);
                        v3.y = 0.0f;
                        v3.z = baseZ;

                        // 纹理坐标
                        v0.u = (baseX + static_cast<f32>(i)) * CLOUD_TEXTURE_SCALE;
                        v0.v = (baseZ + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v1.u = (baseX + static_cast<f32>(i + 1)) * CLOUD_TEXTURE_SCALE;
                        v1.v = (baseZ + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v2.u = (baseX + static_cast<f32>(i + 1)) * CLOUD_TEXTURE_SCALE;
                        v2.v = (baseZ + 0.5f) * CLOUD_TEXTURE_SCALE;

                        v3.u = (baseX + static_cast<f32>(i)) * CLOUD_TEXTURE_SCALE;
                        v3.v = (baseZ + 0.5f) * CLOUD_TEXTURE_SCALE;

                        // 法线（朝北）
                        v0.nx = v1.nx = v2.nx = v3.nx = 0.0f;
                        v0.ny = v1.ny = v2.ny = v3.ny = 0.0f;
                        v0.nz = v1.nz = v2.nz = v3.nz = -1.0f;

                        vertices.push_back(v0);
                        vertices.push_back(v1);
                        vertices.push_back(v2);
                        vertices.push_back(v0);
                        vertices.push_back(v2);
                        vertices.push_back(v3);
                    }
                }

                // 南面 (Z = chunkZ * 8 + 8)
                if (chunkZ < CLOUD_RANGE_MAX) {
                    for (i32 i = 0; i < CLOUD_CHUNK_SIZE; ++i) {
                        CloudVertex v0, v1, v2, v3;
                        f32 z = baseZ + CLOUD_CHUNK_SIZE - CLOUD_TOP_OFFSET;

                        v0.x = baseX + static_cast<f32>(i);
                        v0.y = CLOUD_THICKNESS;
                        v0.z = z;

                        v1.x = baseX + static_cast<f32>(i + 1);
                        v1.y = CLOUD_THICKNESS;
                        v1.z = z;

                        v2.x = baseX + static_cast<f32>(i + 1);
                        v2.y = 0.0f;
                        v2.z = z;

                        v3.x = baseX + static_cast<f32>(i);
                        v3.y = 0.0f;
                        v3.z = z;

                        // 纹理坐标
                        v0.u = (baseX + static_cast<f32>(i)) * CLOUD_TEXTURE_SCALE;
                        v0.v = (baseZ + CLOUD_CHUNK_SIZE - 0.5f) * CLOUD_TEXTURE_SCALE;

                        v1.u = (baseX + static_cast<f32>(i + 1)) * CLOUD_TEXTURE_SCALE;
                        v1.v = (baseZ + CLOUD_CHUNK_SIZE - 0.5f) * CLOUD_TEXTURE_SCALE;

                        v2.u = (baseX + static_cast<f32>(i + 1)) * CLOUD_TEXTURE_SCALE;
                        v2.v = (baseZ + CLOUD_CHUNK_SIZE - 0.5f) * CLOUD_TEXTURE_SCALE;

                        v3.u = (baseX + static_cast<f32>(i)) * CLOUD_TEXTURE_SCALE;
                        v3.v = (baseZ + CLOUD_CHUNK_SIZE - 0.5f) * CLOUD_TEXTURE_SCALE;

                        // 法线（朝南）
                        v0.nx = v1.nx = v2.nx = v3.nx = 0.0f;
                        v0.ny = v1.ny = v2.ny = v3.ny = 0.0f;
                        v0.nz = v1.nz = v2.nz = v3.nz = 1.0f;

                        vertices.push_back(v0);
                        vertices.push_back(v2);
                        vertices.push_back(v1);
                        vertices.push_back(v0);
                        vertices.push_back(v3);
                        vertices.push_back(v2);
                    }
                }
            }
        }

        m_fancyVertexCount = static_cast<u32>(vertices.size());

        // 创建缓冲区
        VkDeviceSize bufferSize = sizeof(CloudVertex) * vertices.size();
        auto result = createBuffer(bufferSize,
                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    m_fancyVBO, m_fancyVBOMemory);
        if (result.failed()) {
            return result;
        }

        // 上传数据
        void* data;
        vkMapMemory(m_device, m_fancyVBOMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices.data(), bufferSize);
        vkUnmapMemory(m_device, m_fancyVBOMemory);
    }

    spdlog::debug("Cloud VBO created: Fast={} vertices, Fancy={} vertices",
                  m_fastVertexCount, m_fancyVertexCount);
    return Result<void>::ok();
}

std::vector<u8> CloudRenderer::generateCloudTexture(u32 width, u32 height) {
    std::vector<u8> data(width * height * 4); // RGBA

    // 使用 Perlin 噪声生成云图案
    // 参考 MC 1.16.5 的云纹理特征
    math::Random rng(12345L); // 固定种子保证一致性

    // 简化的云纹理生成
    // 云纹理是灰度透明度图
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            f32 fx = static_cast<f32>(x) / static_cast<f32>(width);
            f32 fy = static_cast<f32>(y) / static_cast<f32>(height);

            // 使用多层噪声叠加
            f32 noise = 0.0f;
            f32 amplitude = 1.0f;
            f32 frequency = 4.0f;

            for (int octave = 0; octave < 4; ++octave) {
                // 简单的伪随机噪声
                f32 sampleX = fx * frequency;
                f32 sampleY = fy * frequency;

                f32 n = std::sin(sampleX * 12.9898f + sampleY * 78.233f + octave * 43.758f) * 43758.5453f;
                n = n - std::floor(n);

                noise += amplitude * n;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            // 归一化到 0-1
            noise = noise / 2.0f;

            // 阈值化：创建明显的云边界
            f32 alpha = 0.0f;
            if (noise > 0.4f) {
                alpha = std::min(1.0f, (noise - 0.4f) * 2.5f);
            }

            // 应用抖动以避免明显的边界
            f32 dither = (rng.nextFloat() - 0.5f) * 0.1f;
            alpha = std::clamp(alpha + dither, 0.0f, 1.0f);

            u32 idx = (y * width + x) * 4;
            data[idx + 0] = 255; // R
            data[idx + 1] = 255; // G
            data[idx + 2] = 255; // B
            data[idx + 3] = static_cast<u8>(alpha * 255.0f); // A
        }
    }

    return data;
}

Result<void> CloudRenderer::createTexture(const ResourceManager* resourceManager) {
    // 若已有纹理资源，先释放，避免重建时泄漏。
    if (m_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_textureSampler, nullptr);
        m_textureSampler = VK_NULL_HANDLE;
    }
    if (m_textureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_textureImageView, nullptr);
        m_textureImageView = VK_NULL_HANDLE;
    }
    if (m_textureImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_textureImage, nullptr);
        m_textureImage = VK_NULL_HANDLE;
    }
    if (m_textureImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_textureImageMemory, nullptr);
        m_textureImageMemory = VK_NULL_HANDLE;
    }

    std::vector<u8> textureData;
    u32 textureWidth = CLOUD_TEXTURE_SIZE;
    u32 textureHeight = CLOUD_TEXTURE_SIZE;

    if (resourceManager != nullptr) {
        const ResourceLocation cloudTextureLocation("minecraft:textures/environment/clouds");
        auto loadResult = resourceManager->loadTextureRGBA(cloudTextureLocation);
        if (loadResult.success()) {
            textureData = std::move(loadResult.value().pixels);
            textureWidth = loadResult.value().width;
            textureHeight = loadResult.value().height;
            spdlog::info("Cloud texture loaded from resource pack: {} ({}x{})",
                         cloudTextureLocation.toString(), textureWidth, textureHeight);
        } else {
            spdlog::warn("Failed to load cloud texture from resource packs: {}. Falling back to procedural texture.",
                         loadResult.error().toString());
        }
    }

    if (textureData.empty()) {
        textureData = generateCloudTexture(CLOUD_TEXTURE_SIZE, CLOUD_TEXTURE_SIZE);
        textureWidth = CLOUD_TEXTURE_SIZE;
        textureHeight = CLOUD_TEXTURE_SIZE;
    }

    // 创建暂存缓冲区
    VkDeviceSize imageSize = textureData.size();

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    auto result = createBuffer(imageSize,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                stagingBuffer, stagingMemory);
    if (result.failed()) {
        return result;
    }

    // 上传数据
    void* data;
    vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &data);
    std::memcpy(data, textureData.data(), imageSize);
    vkUnmapMemory(m_device, stagingMemory);

    // 创建图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = textureWidth;
    imageInfo.extent.height = textureHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VkResult vkResult = vkCreateImage(m_device, &imageInfo, nullptr, &m_textureImage);
    if (vkResult != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to create cloud texture image");
    }

    // 分配内存
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(m_device, m_textureImage, &memReqs);

    auto memTypeResult = findMemoryType(memReqs.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memTypeResult.failed()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return memTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    vkResult = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_textureImageMemory);
    if (vkResult != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to allocate cloud texture memory");
    }

    vkBindImageMemory(m_device, m_textureImage, m_textureImageMemory, 0);

    // 转换图像布局并复制
    VkCommandBuffer cmd = beginSingleTimeCommands();

    // 转换为传输目标
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

    // 复制缓冲区到图像
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {textureWidth, textureHeight, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_textureImage,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换为着色器只读
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(cmd);

    // 销毁暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

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

    vkResult = vkCreateImageView(m_device, &viewInfo, nullptr, &m_textureImageView);
    if (vkResult != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create cloud texture image view");
    }

    // 创建采样器
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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

    vkResult = vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler);
    if (vkResult != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create cloud texture sampler");
    }

    spdlog::debug("Cloud texture created: {}x{}", textureWidth, textureHeight);
    return Result<void>::ok();
}

void CloudRenderer::updateTextureDescriptors() {
    if (m_descriptorPool == VK_NULL_HANDLE) {
        return;
    }

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }
}

Result<void> CloudRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(CloudUBO);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto result = createBuffer(bufferSize,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    m_uniformBuffers[i], m_uniformBuffersMemory[i]);
        if (result.failed()) {
            return result;
        }

        vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, bufferSize, 0,
                    &m_uniformBuffersMapped[i]);
    }

    return Result<void>::ok();
}

Result<void> CloudRenderer::createDescriptorSetLayout() {
    // 绑定 0: 纹理采样器
    // 绑定 1: Uniform 缓冲区
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

    // 纹理采样器
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    // Uniform 缓冲区
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<u32>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr,
                                                   &m_descriptorSetLayout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create cloud descriptor set layout");
    }

    return Result<void>::ok();
}

Result<void> CloudRenderer::createDescriptorSets() {
    // 创建描述符池
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create cloud descriptor pool");
    }

    // 分配描述符集
    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{
        m_descriptorSetLayout, m_descriptorSetLayout
    };

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    result = vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate cloud descriptor sets");
    }

    // 更新描述符集
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        // 纹理
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        // Uniform 缓冲区
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CloudUBO);

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_device, static_cast<u32>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }

    return Result<void>::ok();
}

Result<void> CloudRenderer::createPipelineLayout() {
    // 推送常量：视图-投影矩阵
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create cloud pipeline layout");
    }

    return Result<void>::ok();
}

Result<void> CloudRenderer::createPipelines() {
    // 加载着色器
    const auto vertPath = resolveShaderPath("cloud.vert.spv");
    const auto fragPath = resolveShaderPath("cloud.frag.spv");

    if (vertPath.empty() || fragPath.empty()) {
        // 如果编译后的着色器不存在，尝试从源文件路径
        spdlog::warn("Cloud shaders not found, using fallback paths");
    }

    auto vertModuleResult = createShaderModule(m_device, vertPath.string().c_str());
    if (vertModuleResult.failed()) {
        return vertModuleResult.error();
    }

    auto fragModuleResult = createShaderModule(m_device, fragPath.string().c_str());
    if (fragModuleResult.failed()) {
        vkDestroyShaderModule(m_device, vertModuleResult.value(), nullptr);
        return fragModuleResult.error();
    }

    VkShaderModule vertShader = vertModuleResult.value();
    VkShaderModule fragShader = fragModuleResult.value();

    // 着色器阶段
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShader;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShader;
    shaderStages[1].pName = "main";

    // 顶点输入
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(CloudVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescs{};
    // 位置
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[0].offset = offsetof(CloudVertex, x);

    // 纹理坐标
    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[1].offset = offsetof(CloudVertex, u);

    // 法线
    attributeDescs[2].binding = 0;
    attributeDescs[2].location = 2;
    attributeDescs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[2].offset = offsetof(CloudVertex, nx);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪
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
    // 云为半透明体，且面向组合较多，关闭剔除可避免由绕序差异导致的整片云不可见。
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // MC 使用顺时针
    rasterizer.depthBiasEnable = VK_FALSE;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 深度模板
    // 云是半透明物体，需要：
    // - 启用深度测试：与天空球比较深度
    // - 禁用深度写入：不阻挡后面的地形
    // 参考 MC 1.16.5 云渲染深度设置
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;  // 禁用深度写入，避免阻挡地形
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // 颜色混合（Alpha 混合）
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                           VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT |
                                           VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 动态状态
    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 创建 Fast 模式管线
    {
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
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;

        VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1,
                                                     &pipelineInfo, nullptr, &m_fastPipeline);
        if (result != VK_SUCCESS) {
            vkDestroyShaderModule(m_device, vertShader, nullptr);
            vkDestroyShaderModule(m_device, fragShader, nullptr);
            return Error(ErrorCode::InitializationFailed, "Failed to create cloud fast pipeline");
        }
    }

    // Fancy 模式管线（目前与 Fast 相同，可以后续优化）
    {
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
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;

        VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1,
                                                     &pipelineInfo, nullptr, &m_fancyPipeline);
        if (result != VK_SUCCESS) {
            vkDestroyShaderModule(m_device, vertShader, nullptr);
            vkDestroyShaderModule(m_device, fragShader, nullptr);
            return Error(ErrorCode::InitializationFailed, "Failed to create cloud fancy pipeline");
        }
    }

    // 清理着色器模块
    vkDestroyShaderModule(m_device, vertShader, nullptr);
    vkDestroyShaderModule(m_device, fragShader, nullptr);

    spdlog::debug("Cloud pipelines created");
    return Result<void>::ok();
}

void CloudRenderer::updateUniformBuffer(u32 frameIndex) {
    CloudUBO ubo{};
    ubo.cloudColor = m_cloudColor;
    ubo.cloudHeight = m_cloudHeight;
    ubo.time = static_cast<f32>(m_gameTime) + m_partialTick;
    ubo.textureScale = CLOUD_TEXTURE_SCALE;
    ubo.cameraY = m_cameraPos.y;

    std::memcpy(m_uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
}

void CloudRenderer::updateCloudMesh(CloudMode mode) {
    // 云网格在初始化时已创建，这里可以用于动态更新
    // 目前不需要，因为云网格是静态的
    m_cloudMeshDirty = false;
}

// ============================================================================
// Vulkan 辅助函数
// ============================================================================

Result<u32> CloudRenderer::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::OutOfMemory, "Failed to find suitable memory type");
}

Result<void> CloudRenderer::createBuffer(VkDeviceSize size,
                                          VkBufferUsageFlags usage,
                                          VkMemoryPropertyFlags properties,
                                          VkBuffer& buffer,
                                          VkDeviceMemory& memory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create buffer");
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, buffer, &memReqs);

    auto memTypeResult = findMemoryType(memReqs.memoryTypeBits, properties);
    if (memTypeResult.failed()) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        return memTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    result = vkAllocateMemory(m_device, &allocInfo, nullptr, &memory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to allocate buffer memory");
    }

    vkBindBufferMemory(m_device, buffer, memory, 0);
    return Result<void>::ok();
}

VkCommandBuffer CloudRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void CloudRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

} // namespace mc::client::renderer::trident::cloud
