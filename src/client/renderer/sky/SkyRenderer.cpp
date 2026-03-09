#include "SkyRenderer.hpp"
#include "../ShaderPath.hpp"
#include "../VulkanBuffer.hpp"
#include "../../../common/core/Constants.hpp"
#include "../../../common/math/random/Random.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <array>

namespace mr::client {

namespace {

constexpr f32 SKY_CLIP_SCALE = 0.0075f;

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

struct SkyVertex {
    float x, y, z;
};

struct SkyPushConstants {
    glm::mat4 viewProjection;
};

// ============================================================================
// 构造/析构
// ============================================================================

SkyRenderer::SkyRenderer() = default;

SkyRenderer::~SkyRenderer() {
    destroy();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> SkyRenderer::initialize(VulkanContext* ctx, VkRenderPass renderPass, VkExtent2D extent) {
    if (m_initialized) {
        return Result<void>::ok();
    }

    m_ctx = ctx;
    m_renderPass = renderPass;
    m_extent = extent;

    // 创建顶点缓冲区
    auto result1 = createSkyDomeVBO();
    if (result1.failed()) {
        return result1;
    }

    auto result2 = createStarVBO();
    if (result2.failed()) {
        return result2;
    }

    auto result3 = createSunMoonVBO();
    if (result3.failed()) {
        return result3;
    }

    // 创建 Uniform 缓冲区
    auto result4 = createUniformBuffers();
    if (result4.failed()) {
        return result4;
    }

    // 创建描述符
    auto result5 = createDescriptorSetLayout();
    if (result5.failed()) {
        return result5;
    }

    auto result6 = createDescriptorSets();
    if (result6.failed()) {
        return result6;
    }

    // 创建管线
    auto result7 = createPipelineLayout();
    if (result7.failed()) {
        return result7;
    }

    auto result8 = createPipelines();
    if (result8.failed()) {
        return result8;
    }

    m_initialized = true;
    spdlog::info("SkyRenderer initialized");
    return Result<void>::ok();
}

void SkyRenderer::destroy() {
    if (m_ctx == nullptr) return;

    VkDevice device = m_ctx->device();

    // 销毁管线
    if (m_skyPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_skyPipeline, nullptr);
        m_skyPipeline = VK_NULL_HANDLE;
    }
    if (m_skyStarPipeline != VK_NULL_HANDLE && m_skyStarPipeline != m_skyPipeline) {
        vkDestroyPipeline(device, m_skyStarPipeline, nullptr);
        m_skyStarPipeline = VK_NULL_HANDLE;
    }
    if (m_sunPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_sunPipeline, nullptr);
        m_sunPipeline = VK_NULL_HANDLE;
    }
    if (m_moonPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_moonPipeline, nullptr);
        m_moonPipeline = VK_NULL_HANDLE;
    }
    if (m_starPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_starPipeline, nullptr);
        m_starPipeline = VK_NULL_HANDLE;
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
        m_uniformBuffers[i].reset();
    }

    // 销毁顶点缓冲区
    m_skyDomeVBO.reset();
    m_skyDomeIBO.reset();
    m_starVBO.reset();
    m_sunMoonVBO.reset();

    m_initialized = false;
    m_ctx = nullptr;
    spdlog::info("SkyRenderer destroyed");
}

Result<void> SkyRenderer::onResize(VkExtent2D extent) {
    m_extent = extent;
    // 不需要重建资源，因为天空渲染使用的是视图投影矩阵
    return Result<void>::ok();
}

// ============================================================================
// 更新
// ============================================================================

void SkyRenderer::update(i64 dayTime, i64 gameTime, f32 partialTick,
                         f32 rainStrength, f32 thunderStrength) {
    m_dayTime = dayTime;
    m_gameTime = gameTime;
    m_rainStrength = rainStrength;
    m_thunderStrength = thunderStrength;

    // 计算天体角度 (插值)
    m_celestialAngle = CelestialCalculations::calculateCelestialAngleInterpolated(dayTime, partialTick);

    // 计算月相
    m_moonPhase = CelestialCalculations::calculateMoonPhase(gameTime);

    // 计算天空颜色
    m_skyColor = CelestialCalculations::calculateSkyColor(m_celestialAngle, rainStrength, thunderStrength);
    m_fogColor = CelestialCalculations::calculateFogColor(m_celestialAngle, rainStrength, thunderStrength);

    // 计算太阳方向和强度
    m_sunDirection = CelestialCalculations::calculateSunDirection(m_celestialAngle);
    m_sunIntensity = CelestialCalculations::calculateSunIntensity(m_celestialAngle);

    // 计算星星亮度
    m_starBrightness = CelestialCalculations::calculateStarBrightness(m_celestialAngle);

}

// ============================================================================
// 渲染
// ============================================================================

void SkyRenderer::render(VkCommandBuffer cmd, const glm::mat4& viewProjection, const glm::vec3& cameraPos, u32 frameIndex) {
    (void)cameraPos;

    if (!m_initialized || m_pipelineLayout == VK_NULL_HANDLE) {
        return;
    }

    m_currentFrame = frameIndex % MAX_FRAMES_IN_FLIGHT;
    m_lastViewProjection = viewProjection;
    updateUniformBuffer(m_currentFrame);

    if (m_descriptorSets[m_currentFrame] == VK_NULL_HANDLE) {
        return;
    }

    // 绑定描述符集
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);

    // 现有 sky/sun/moon/star 着色器在顶点阶段使用“线性裁剪映射”而非标准透视除法，
    // 需要对传入矩阵做统一缩放，避免天体落在远超 [-1, 1] 的 NDC 外。
    SkyPushConstants pushConstants{};
    pushConstants.viewProjection = viewProjection * SKY_CLIP_SCALE;
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(SkyPushConstants), &pushConstants);

    // 渲染天空穹顶 (最优先，写入深度为远平面)
    renderSkyDome(cmd);

    // 渲染太阳
    renderSun(cmd);

    // 渲染月亮
    renderMoon(cmd);

    // 渲染星星 (夜晚可见)
    if (m_starBrightness > 0.005f) {
        renderStars(cmd);
    }
}

void SkyRenderer::renderSkyDome(VkCommandBuffer cmd) {
    if (m_skyPipeline == VK_NULL_HANDLE || !m_skyDomeVBO || !m_skyDomeIBO || m_skyDomeIndexCount == 0) {
        return;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyPipeline);

    VkDeviceSize offset = 0;
    VkBuffer vertexBuffer = m_skyDomeVBO->buffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_skyDomeIBO->buffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmd, m_skyDomeIndexCount, 1, 0, 0, 0);
}

void SkyRenderer::renderSun(VkCommandBuffer cmd) {
    // 太阳强度太低时不渲染
    const bool shouldLog = (++m_sunDebugLogCounter % 240) == 0;
    if (m_sunIntensity < 0.03f || m_sunPipeline == VK_NULL_HANDLE || !m_sunMoonVBO) {
        if (shouldLog) {
            spdlog::info("[Sky] Sun skipped: intensity={:.4f}, pipelineValid={}, vboValid={}",
                        m_sunIntensity,
                        m_sunPipeline != VK_NULL_HANDLE,
                        m_sunMoonVBO != nullptr);
        }
        return;
    }

    if (shouldLog) {
        const f32 angle = m_celestialAngle * mr::math::TAU_F;
        const f32 height = std::cos(angle);
        const f32 xz = std::sin(angle);
        const glm::vec3 sunDir = glm::normalize(glm::vec3(xz, height, 0.0f));

        const glm::vec3 sunCenter = sunDir * 100.0f;
        const glm::mat4 scaledViewProjection = m_lastViewProjection * SKY_CLIP_SCALE;
        const glm::mat4 viewWithoutTranslation = glm::mat4(glm::mat3(scaledViewProjection));
        const glm::vec4 pos = viewWithoutTranslation * glm::vec4(sunCenter, 1.0f);
        const f32 w = std::abs(pos.w) > 1e-6f ? pos.w : 1.0f;
        const glm::vec2 ndc = glm::vec2(pos.x / w, pos.y / w);
        const f32 crossLen = glm::length(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), sunDir));

        spdlog::info("[Sky] Sun draw: dayTime={}, gameTime={}, intensity={:.4f}, angle={:.6f}, sunDir=({:.3f},{:.3f},{:.3f}), crossLen={:.6f}, ndc=({:.3f},{:.3f})",
                m_dayTime, m_gameTime, m_sunIntensity, m_celestialAngle,
                    sunDir.x, sunDir.y, sunDir.z,
                    crossLen, ndc.x, ndc.y);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_sunPipeline);

    VkDeviceSize offset = 0;
    VkBuffer vertexBuffer = m_sunMoonVBO->buffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);

    // 太阳是一个四边形，4个顶点
    vkCmdDraw(cmd, 4, 1, 0, 0);
}

void SkyRenderer::renderMoon(VkCommandBuffer cmd) {
    // 白天不渲染月亮（晨昏允许短暂过渡）
    if (m_sunIntensity > 0.18f || m_moonPipeline == VK_NULL_HANDLE || !m_sunMoonVBO) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_moonPipeline);

    VkDeviceSize offset = 0;
    VkBuffer vertexBuffer = m_sunMoonVBO->buffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);

    // 月亮是一个四边形，4个顶点 (从索引4开始)
    vkCmdDraw(cmd, 4, 1, 4, 0);
}

void SkyRenderer::renderStars(VkCommandBuffer cmd) {
    if (m_starVertexCount == 0 || m_starPipeline == VK_NULL_HANDLE || !m_starVBO) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_starPipeline);

    VkDeviceSize offset = 0;
    VkBuffer vertexBuffer = m_starVBO->buffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);

    // 星星使用点渲染
    vkCmdDraw(cmd, m_starVertexCount, 1, 0, 0);
}

// ============================================================================
// 资源创建
// ============================================================================

Result<void> SkyRenderer::createSkyDomeVBO() {
    // 创建天空穹顶网格 (参考 MC WorldRenderer.renderSky)
    // Y=16 的平面网格，范围 -384 到 +384
    // 每格 64 单位，共 12x12 个四边形

    constexpr f32 SKY_HEIGHT = 16.0f;
    constexpr f32 SKY_RADIUS = 384.0f;
    constexpr i32 GRID_SIZE = 64;
    constexpr i32 GRID_COUNT = 12; // (384 * 2) / 64 = 12

    std::vector<SkyVertex> vertices;
    std::vector<u16> indices;

    // 生成顶点
    for (i32 z = -GRID_COUNT; z <= GRID_COUNT; ++z) {
        for (i32 x = -GRID_COUNT; x <= GRID_COUNT; ++x) {
            f32 px = static_cast<f32>(x * GRID_SIZE);
            f32 pz = static_cast<f32>(z * GRID_SIZE);
            vertices.push_back({px, SKY_HEIGHT, pz});
        }
    }

    // 生成索引 (三角形带)
    for (i32 z = 0; z < GRID_COUNT * 2; ++z) {
        i32 rowStart = z * (GRID_COUNT * 2 + 1);
        i32 nextRowStart = (z + 1) * (GRID_COUNT * 2 + 1);

        for (i32 x = 0; x < GRID_COUNT * 2; ++x) {
            indices.push_back(static_cast<u16>(rowStart + x));
            indices.push_back(static_cast<u16>(nextRowStart + x));
            indices.push_back(static_cast<u16>(rowStart + x + 1));

            indices.push_back(static_cast<u16>(rowStart + x + 1));
            indices.push_back(static_cast<u16>(nextRowStart + x));
            indices.push_back(static_cast<u16>(nextRowStart + x + 1));
        }
    }

    m_skyDomeIndexCount = static_cast<u32>(indices.size());

    // 创建 VBO
    VkDeviceSize vertexSize = vertices.size() * sizeof(SkyVertex);
    m_skyDomeVBO = std::make_unique<VulkanBuffer>();
    auto result1 = m_skyDomeVBO->create(
        m_ctx->device(),
        m_ctx->physicalDevice(),
        vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (result1.failed()) {
        return result1.error();
    }
    m_skyDomeVBO->upload(vertices.data(), vertexSize);

    // 创建 IBO
    VkDeviceSize indexSize = indices.size() * sizeof(u16);
    m_skyDomeIBO = std::make_unique<VulkanBuffer>();
    auto result2 = m_skyDomeIBO->create(
        m_ctx->device(),
        m_ctx->physicalDevice(),
        indexSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (result2.failed()) {
        return result2.error();
    }
    m_skyDomeIBO->upload(indices.data(), indexSize);

    return Result<void>::ok();
}

Result<void> SkyRenderer::createStarVBO() {
    // 使用固定种子生成星星位置 (MC 使用 10842L)
    math::Random rng(CelestialCalculations::getStarSeed());

    std::vector<SkyVertex> vertices;
    vertices.reserve(CelestialCalculations::getStarCount());

    // 在单位球面上均匀分布星星
    for (i32 i = 0; i < CelestialCalculations::getStarCount(); ++i) {
        // 使用球面均匀分布算法
        f64 theta = 2.0 * mr::math::PI_DOUBLE * rng.nextDouble();
        f64 phi = std::acos(2.0 * rng.nextDouble() - 1.0);
        f64 radius = 100.0; // 星星距离

        f32 x = static_cast<f32>(radius * std::sin(phi) * std::cos(theta));
        f32 y = static_cast<f32>(radius * std::sin(phi) * std::sin(theta));
        f32 z = static_cast<f32>(radius * std::cos(phi));

        // 只保留 y > 0 的星星 (地平线以上)
        if (y > 0.0f) {
            vertices.push_back({x, y, z});
        }
    }

    m_starVertexCount = static_cast<u32>(vertices.size());

    // 创建 VBO
    VkDeviceSize vertexSize = vertices.size() * sizeof(SkyVertex);
    m_starVBO = std::make_unique<VulkanBuffer>();
    auto result = m_starVBO->create(
        m_ctx->device(),
        m_ctx->physicalDevice(),
        vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (result.failed()) {
        return result.error();
    }
    m_starVBO->upload(vertices.data(), vertexSize);

    return Result<void>::ok();
}

Result<void> SkyRenderer::createSunMoonVBO() {
    // 太阳和月亮顶点使用“单位四边形”[-1, 1]。
    // 具体实际尺寸由顶点着色器中的常量控制（sun: 30, moon: 20）。
    // 注意：如果这里直接传入世界尺寸（例如 ±30），片元着色器会因为 UV 超范围
    // 导致圆盘被完全 discard，从而出现“白天看不到太阳”的问题。
    std::vector<SkyVertex> vertices = {
        // 太阳 (4 个顶点)
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},

        // 月亮 (4 个顶点)
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
    };

    // 创建 VBO
    VkDeviceSize vertexSize = vertices.size() * sizeof(SkyVertex);
    m_sunMoonVBO = std::make_unique<VulkanBuffer>();
    auto result = m_sunMoonVBO->create(
        m_ctx->device(),
        m_ctx->physicalDevice(),
        vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (result.failed()) {
        return result.error();
    }
    m_sunMoonVBO->upload(vertices.data(), vertexSize);

    return Result<void>::ok();
}

Result<void> SkyRenderer::createUniformBuffers() {
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_uniformBuffers[i] = std::make_unique<VulkanBuffer>();
        auto result = m_uniformBuffers[i]->create(
            m_ctx->device(),
            m_ctx->physicalDevice(),
            sizeof(SkyUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        if (result.failed()) {
            return result.error();
        }
    }
    return Result<void>::ok();
}

Result<void> SkyRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;

    VkResult result = vkCreateDescriptorSetLayout(
        m_ctx->device(), &layoutInfo, nullptr, &m_descriptorSetLayout);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sky descriptor set layout");
    }

    return Result<void>::ok();
}

Result<void> SkyRenderer::createDescriptorSets() {
    // 创建描述符池
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    VkResult result = vkCreateDescriptorPool(
        m_ctx->device(), &poolInfo, nullptr, &m_descriptorPool);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sky descriptor pool");
    }

    // 创建描述符集
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    result = vkAllocateDescriptorSets(m_ctx->device(), &allocInfo, m_descriptorSets);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate sky descriptor sets");
    }

    // 更新描述符集
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_uniformBuffers[i]->buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SkyUBO);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_ctx->device(), 1, &descriptorWrite, 0, nullptr);
    }

    return Result<void>::ok();
}

Result<void> SkyRenderer::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SkyPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(
        m_ctx->device(), &layoutInfo, nullptr, &m_pipelineLayout);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sky pipeline layout");
    }

    return Result<void>::ok();
}

Result<void> SkyRenderer::createPipelines() {
    const VkDevice device = m_ctx->device();

    const auto skyVertPath = resolveShaderPath("sky.vert.spv");
    const auto skyFragPath = resolveShaderPath("sky.frag.spv");
    const auto sunVertPath = resolveShaderPath("sun.vert.spv");
    const auto sunFragPath = resolveShaderPath("sun.frag.spv");
    const auto moonVertPath = resolveShaderPath("moon.vert.spv");
    const auto moonFragPath = resolveShaderPath("moon.frag.spv");
    const auto starVertPath = resolveShaderPath("star.vert.spv");
    const auto starFragPath = resolveShaderPath("star.frag.spv");

    if (skyVertPath.empty() || skyFragPath.empty() ||
        sunVertPath.empty() || sunFragPath.empty() ||
        moonVertPath.empty() || moonFragPath.empty() ||
        starVertPath.empty() || starFragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve one or more sky shader binaries");
    }

    auto createPipeline = [this, device](const std::filesystem::path& vertPath,
                                         const std::filesystem::path& fragPath,
                                         VkPrimitiveTopology topology,
                                         VkBool32 blendEnable,
                                         VkBool32 depthTestEnable,
                                         VkBool32 depthWriteEnable,
                                         VkPipeline* outPipeline) -> Result<void> {
        const auto vertPathString = vertPath.string();
        const auto fragPathString = fragPath.string();

        auto vertResult = createShaderModule(device, vertPathString.c_str());
        if (vertResult.failed()) {
            return vertResult.error();
        }

        auto fragResult = createShaderModule(device, fragPathString.c_str());
        if (fragResult.failed()) {
            vkDestroyShaderModule(device, vertResult.value(), nullptr);
            return fragResult.error();
        }

        const VkShaderModule vertModule = vertResult.value();
        const VkShaderModule fragModule = fragResult.value();

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertModule;
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragModule;
        shaderStages[1].pName = "main";

        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(SkyVertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attrDesc{};
        attrDesc.binding = 0;
        attrDesc.location = 0;
        attrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
        attrDesc.offset = 0;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = 1;
        vertexInputInfo.pVertexAttributeDescriptions = &attrDesc;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = depthTestEnable;
        depthStencil.depthWriteEnable = depthWriteEnable;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                              VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT |
                                              VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = blendEnable;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

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

        const VkResult pipelineResult = vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, outPipeline);

        vkDestroyShaderModule(device, fragModule, nullptr);
        vkDestroyShaderModule(device, vertModule, nullptr);

        if (pipelineResult != VK_SUCCESS) {
            return Error(ErrorCode::InitializationFailed,
                         std::string("Failed to create sky graphics pipeline for shaders: ") +
                         vertPathString + " / " + fragPathString);
        }

        return Result<void>::ok();
    };

    // 天空穹顶
    auto skyResult = createPipeline(skyVertPath,
                                    skyFragPath,
                                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE, VK_FALSE,
                                    VK_FALSE, &m_skyPipeline);
    if (skyResult.failed()) {
        return skyResult.error();
    }

    // 太阳
    auto sunResult = createPipeline(sunVertPath,
                                    sunFragPath, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                                    VK_TRUE, VK_FALSE, VK_FALSE, &m_sunPipeline);
    if (sunResult.failed()) {
        return sunResult.error();
    }

    // 月亮
    auto moonResult = createPipeline(
        moonVertPath,
        moonFragPath,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        VK_TRUE,
        VK_FALSE,
        VK_FALSE,
        &m_moonPipeline);
    if (moonResult.failed()) {
        return moonResult.error();
    }

    // 星星
    auto starResult = createPipeline(
        starVertPath,
        starFragPath,
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VK_TRUE,
        VK_FALSE,
        VK_FALSE,
        &m_starPipeline);
    if (starResult.failed()) {
        return starResult.error();
    }

    // 兼容保留字段：当前未使用独立夜晚天空管线
    m_skyStarPipeline = VK_NULL_HANDLE;

    spdlog::info("SkyRenderer pipelines created");
    return Result<void>::ok();
}

void SkyRenderer::updateUniformBuffer(u32 frameIndex) {
    SkyUBO ubo = {};
    ubo.skyColor = m_skyColor;
    ubo.fogColor = m_fogColor;

    // sun.vert 在太阳接近天顶/天底时会出现 right=normalize(cross(up, sunDir)) 退化。
    // 这里做极小偏移，避免精确零向量导致太阳四边形退化不可见。
    f32 adjustedAngle = m_celestialAngle;
    const f32 angleRad = adjustedAngle * mr::math::TAU_F;
    if (std::abs(std::sin(angleRad)) < 1e-4f) {
        adjustedAngle += 1e-4f;
        if (adjustedAngle >= 1.0f) {
            adjustedAngle -= 1.0f;
        }
    }

    ubo.celestialAngle = adjustedAngle;
    ubo.starBrightness = m_starBrightness;
    ubo.moonPhase = m_moonPhase;
    ubo.padding = 0.0f;

    m_uniformBuffers[frameIndex]->upload(&ubo, sizeof(SkyUBO));
}

} // namespace mr::client
