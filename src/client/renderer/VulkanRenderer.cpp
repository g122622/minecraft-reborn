#include "VulkanRenderer.hpp"
#include "ShaderPath.hpp"
#include "VulkanBuffer.hpp"
#include "DefaultTextureAtlas.hpp"
#include "sky/CelestialCalculations.hpp"
#include "../ui/DefaultAsciiFont.hpp"
#include "../resource/EntityTextureLoader.hpp"
#include "MeshTypes.hpp"
#include "../../common/world/WorldConstants.hpp"
#include "../../common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>
#include <array>
#include <cstring>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

namespace mc::client {

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() {
    destroy();
}

Result<void> VulkanRenderer::initialize(GLFWwindow* window, const RendererConfig& config) {
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Renderer already initialized");
    }

    m_config = config;

    // 创建Vulkan上下文
    m_context = std::make_unique<VulkanContext>();

    // 第一步：创建instance
    auto instanceResult = m_context->createInstanceOnly(config.vulkanConfig);
    if (instanceResult.failed()) {
        return instanceResult.error();
    }

    // 第二步：创建Surface（在instance创建后）
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(m_context->instance(), window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create window surface: " + std::to_string(result));
    }

    // 设置surface到context
    m_context->setSurface(surface);

    // 第三步：创建设备（物理设备+逻辑设备）
    auto deviceResult = m_context->createDevice();
    if (deviceResult.failed()) {
        return deviceResult.error();
    }

    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // 创建交换链
    m_swapchain = std::make_unique<VulkanSwapchain>();
    SwapChainConfig swapChainConfig = config.swapChainConfig;
    swapChainConfig.width = static_cast<u32>(width);
    swapChainConfig.height = static_cast<u32>(height);
    swapChainConfig.vsync = config.enableVSync;

    auto swapchainResult = m_swapchain->initialize(m_context.get(), swapChainConfig);
    if (swapchainResult.failed()) {
        return swapchainResult.error();
    }

    // 创建渲染通道
    auto renderPassResult = createRenderPass();
    if (renderPassResult.failed()) {
        return renderPassResult.error();
    }

    // 创建深度缓冲区
    auto depthResult = createDepthResources();
    if (depthResult.failed()) {
        return depthResult.error();
    }

    // 创建描述符集布局
    auto descriptorResult = createDescriptorSetLayouts();
    if (descriptorResult.failed()) {
        return descriptorResult.error();
    }

    // 创建管线布局
    auto pipelineLayoutResult = createPipelineLayout();
    if (pipelineLayoutResult.failed()) {
        return pipelineLayoutResult.error();
    }

    // 创建命令池
    auto commandPoolResult = createCommandPool();
    if (commandPoolResult.failed()) {
        return commandPoolResult.error();
    }

    // 创建命令缓冲区
    auto commandBufferResult = createCommandBuffers();
    if (commandBufferResult.failed()) {
        return commandBufferResult.error();
    }

    // 创建帧缓冲区
    auto framebufferResult = createFramebuffers();
    if (framebufferResult.failed()) {
        return framebufferResult.error();
    }

    // 创建描述符池
    auto descriptorPoolResult = createDescriptorPool();
    if (descriptorPoolResult.failed()) {
        return descriptorPoolResult.error();
    }

    // 创建Uniform缓冲区
    auto uniformResult = createUniformBuffers();
    if (uniformResult.failed()) {
        return uniformResult.error();
    }

    // 创建同步对象
    auto syncResult = createSyncObjects();
    if (syncResult.failed()) {
        return syncResult.error();
    }

    // 创建区块管线
    auto chunkPipelineResult = createChunkPipeline();
    if (chunkPipelineResult.failed()) {
        spdlog::warn("Failed to create chunk pipeline: {}", chunkPipelineResult.error().toString());
    }

    // 创建区块纹理图集
    auto chunkAtlasResult = createChunkTextureAtlas();
    if (chunkAtlasResult.failed()) {
        spdlog::warn("Failed to create chunk texture atlas: {}", chunkAtlasResult.error().toString());
    }

    // 创建天空渲染器
    auto skyResult = m_skyRenderer.initialize(m_context.get(), m_renderPass, m_swapchain->extent());
    if (skyResult.failed()) {
        spdlog::warn("Failed to create sky renderer: {}", skyResult.error().toString());
    } else {
        m_skyRendererInitialized = true;
    }

    // 创建GUI渲染器
    auto guiResult = createGuiRenderer();
    if (guiResult.failed()) {
        spdlog::warn("Failed to create GUI renderer: {}", guiResult.error().toString());
    }

    // 创建实体渲染管线
    auto entityPipelineResult = createEntityPipeline();
    if (entityPipelineResult.failed()) {
        spdlog::warn("Failed to create entity pipeline: {}", entityPipelineResult.error().toString());
    }

    // 注意：区块由 ClientWorld 管理，不再自动创建测试区块

    m_initialized = true;
    spdlog::info("Vulkan renderer initialized successfully");
    return Result<void>::ok();
}

void VulkanRenderer::destroy() {
    if (!m_initialized) {
        return;
    }

    m_context->waitIdle();

    // 销毁实体渲染资源
    destroyEntityResources();

    // 销毁天空渲染器
    m_skyRenderer.destroy();
    m_skyRendererInitialized = false;

    destroyGuiResources();
    destroyChunkResources();

    m_chunkPipeline.reset();
    m_cameraUBO.destroy();
    m_lightingUBO.destroy();

    destroySyncObjects();
    destroyDescriptors();
    destroyFramebuffers();
    destroyDepthResources();
    destroyCommandBuffers();
    destroyCommandPool();
    destroyRenderPass();

    m_swapchain.reset();
    m_context.reset();

    m_initialized = false;
    spdlog::info("Vulkan renderer destroyed");
}

Result<void> VulkanRenderer::beginFrame() {
    MC_TRACE_BEGIN_FRAME("BeginFrame");

    if (!m_initialized || m_minimized) {
        return Error(ErrorCode::InvalidState, "Renderer not ready");
    }

    if (m_frameStarted) {
        return Error(ErrorCode::InvalidState, "Frame already started");
    }

    // 等待当前帧的fence（限制同时进行的帧数）
    {
        MC_TRACE_BEGIN_FRAME("WaitForFence");
        vkWaitForFences(m_context->device(), 1, &m_inFlightFences[m_currentFrame],
                        VK_TRUE, UINT64_MAX);
    }

    // 获取下一帧图像，使用当前帧索引对应的信号量
    {
        MC_TRACE_BEGIN_FRAME("AcquireNextImage");
        auto acquireResult = m_swapchain->acquireNextImage(
            m_imageAvailableSemaphores[m_currentFrame]);

        if (acquireResult.failed()) {
            if (acquireResult.error().code() == ErrorCode::InvalidState) {
                auto recreateResult = recreateSwapchain();
                if (recreateResult.failed()) {
                    return recreateResult.error();
                }
                return beginFrame();
            }
            return acquireResult.error();
        }

        m_imageIndex = acquireResult.value();
    }

    // 检查是否有其他帧正在使用此图像
    {
        MC_TRACE_BEGIN_FRAME("WaitForImageFence");
        if (m_imageFences[m_imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(m_context->device(), 1, &m_imageFences[m_imageIndex], VK_TRUE, UINT64_MAX);
        }
        // 标记此图像正在被当前帧使用
        m_imageFences[m_imageIndex] = m_inFlightFences[m_currentFrame];
    }

    // 重置fence
    {
        MC_TRACE_BEGIN_FRAME("ResetFence");
        vkResetFences(m_context->device(), 1, &m_inFlightFences[m_currentFrame]);
    }

    // 处理 ChunkRenderer 的延迟销毁队列
    // 此时上一帧的 GPU 命令已完成，可以安全销毁缓冲区
    if (m_chunkRendererInitialized) {
        MC_TRACE_BEGIN_FRAME("ProcessPendingDestroys");
        m_chunkRenderer.processPendingDestroys();
    }

    // 开始命令缓冲区
    {
        MC_TRACE_CMD_BUFFER("BeginCommandBuffer");
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        VkCommandBuffer cmd = m_commandBuffers[m_imageIndex];
        vkResetCommandBuffer(cmd, 0);

        VkResult result = vkBeginCommandBuffer(cmd, &beginInfo);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to begin command buffer: " + std::to_string(result));
        }
    }

    // 开始渲染通道
    {
        MC_TRACE_CMD_BUFFER("BeginRenderPass");
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_framebuffers[m_imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapchain->extent();

        // 清除值：颜色 + 深度
        std::array<VkClearValue, 2> clearValues{};
        if (m_skyRendererInitialized) {
            const glm::vec4& skyColor = m_skyRenderer.skyColor();
            clearValues[0].color = {{skyColor.r, skyColor.g, skyColor.b, skyColor.a}};
        } else {
            clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};
        }
        clearValues[1].depthStencil.depth = 1.0f;
        clearValues[1].depthStencil.stencil = 0;
        renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // 在渲染通道开始前准备GUI帧数据（更新字体纹理等）
        if (m_guiRendererInitialized && m_guiRenderer) {
            MC_TRACE_GUI("PrepareFrame");
            m_guiRenderer->prepareFrame(m_commandBuffers[m_imageIndex]);
        }

        vkCmdBeginRenderPass(m_commandBuffers[m_imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    m_frameStarted = true;
    return Result<void>::ok();
}

Result<void> VulkanRenderer::endFrame() {
    MC_TRACE_END_FRAME("EndFrame");

    if (!m_frameStarted) {
        return Error(ErrorCode::InvalidState, "Frame not started");
    }

    VkCommandBuffer cmd = m_commandBuffers[m_imageIndex];

    // 结束渲染通道
    {
        MC_TRACE_CMD_BUFFER("EndRenderPass");
        vkCmdEndRenderPass(cmd);
    }

    // 结束命令缓冲区
    {
        MC_TRACE_CMD_BUFFER("EndCommandBuffer");
        VkResult result = vkEndCommandBuffer(cmd);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to end command buffer: " + std::to_string(result));
        }
    }

    // 提交命令缓冲区
    {
        MC_TRACE_END_FRAME("QueueSubmit");
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // 使用当前帧的imageAvailableSemaphore等待
        VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        // 使用当前图像的renderFinishedSemaphore发信号
        VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_imageIndex]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult result = vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo,
                               m_inFlightFences[m_currentFrame]);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to submit draw command buffer: " + std::to_string(result));
        }
    }

    // 呈现，使用当前图像的renderFinishedSemaphore
    {
        MC_TRACE_END_FRAME("Present");
        auto presentResult = m_swapchain->present(m_imageIndex,
                                                   m_renderFinishedSemaphores[m_imageIndex]);

        if (presentResult.failed()) {
            if (presentResult.error().code() == ErrorCode::InvalidState) {
                auto recreateResult = recreateSwapchain();
                if (recreateResult.failed()) {
                    return recreateResult.error();
                }
            } else {
                return presentResult.error();
            }
        }
    }

    // 下一帧（循环使用MAX_FRAMES_IN_FLIGHT个帧资源）
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    m_frameStarted = false;

    return Result<void>::ok();
}

Result<void> VulkanRenderer::render() {
    auto beginResult = beginFrame();
    if (beginResult.failed()) {
        return beginResult.error();
    }

    // 更新Uniform缓冲区
    {
        MC_TRACE_UNIFORM_UPDATE("UpdateUniformBuffers");
        updateUniformBuffers();
    }

    VkCommandBuffer cmd = m_commandBuffers[m_imageIndex];
    assert(cmd != VK_NULL_HANDLE && "Command buffer must be valid");

    // 设置视口和裁剪
    {
        MC_TRACE_VIEWPORT("SetViewportScissor");
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapchain->extent().width);
        viewport.height = static_cast<float>(m_swapchain->extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_swapchain->extent();
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    // 渲染天空（先绘制背景）
    if (m_skyRendererInitialized) {
        MC_TRACE_SKY("SkyRender");
        glm::mat4 viewProjection(1.0f);
        glm::vec3 cameraPos(0.0f);
        glm::vec3 cameraForward(0.0f, 0.0f, -1.0f);

        if (m_camera) {
            glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(m_camera->viewMatrix()));
            viewProjection = m_camera->projectionMatrix() * viewNoTranslation;
            cameraPos = m_camera->position();
            cameraForward = m_camera->forward();
        }

        m_skyRenderer.render(cmd, viewProjection, cameraPos, cameraForward, m_currentFrame);
    }

    // 渲染区块
    if (m_chunkRendererInitialized && m_chunkRenderer.chunkCount() > 0) {
        MC_TRACE_CHUNK_DRAW("ChunkRender");
        renderChunks(cmd);
    }

    // 渲染实体
    if (m_entityRendererInitialized && m_entityRenderCallback) {
        renderEntities(cmd);
    }

    // 渲染GUI
    if (m_guiRendererInitialized) {
        MC_TRACE_GUI("GuiRender");
        renderGui(cmd);
    }

    return endFrame();
}

void VulkanRenderer::renderChunks(VkCommandBuffer cmd) {
    if (!m_chunkPipeline || !m_chunkPipeline->pipeline()) {
        return;
    }

    if (m_chunkRenderer.chunkCount() == 0) {
        return;
    }

    // 绑定区块管线
    {
        MC_TRACE_CHUNK_DRAW("BindPipeline");
        m_chunkPipeline->bind(cmd);
    }

    // 绑定相机描述符集 (set = 0)
    {
        MC_TRACE_DESCRIPTOR_BIND("BindCameraDescriptor");
        if (!m_chunkDescriptorSets.empty() && m_chunkDescriptorSets[m_currentFrame] != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_chunkPipeline->pipelineLayout(), 0, 1,
                                    &m_chunkDescriptorSets[m_currentFrame], 0, nullptr);
        }
    }

    // 绑定纹理描述符集 (set = 1)
    {
        MC_TRACE_DESCRIPTOR_BIND("BindTextureDescriptor");
        if (m_chunkTextureDescriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_chunkPipeline->pipelineLayout(), 1, 1,
                                    &m_chunkTextureDescriptorSet, 0, nullptr);
        }
    }

    // 渲染所有区块，为每个区块设置正确的世界偏移
    {
        MC_TRACE_CHUNK_DRAW("DrawChunks");
        m_chunkRenderer.render(cmd, m_chunkPipeline->pipelineLayout(),
            [this, cmd](const ChunkId& chunkId) {
                // 设置推送常量 - 区块世界偏移
                // TODO: 下面一行代码必须注释才能编译通过，否则会导致编译器死循环。疑似MSVC编译器bug。
                // MC_TRACE_PUSH_CONSTANTS("ChunkOffset");
                ModelPushConstants pushConstants{};
                std::memset(&pushConstants, 0, sizeof(pushConstants));
                // 单位矩阵
                pushConstants.model[0] = 1.0f;
                pushConstants.model[5] = 1.0f;
                pushConstants.model[10] = 1.0f;
                pushConstants.model[15] = 1.0f;
                // 区块世界偏移（区块坐标 * 区块大小）
                pushConstants.chunkOffset[0] = static_cast<f32>(chunkId.x * world::CHUNK_WIDTH);
                pushConstants.chunkOffset[1] = 0.0f;
                pushConstants.chunkOffset[2] = static_cast<f32>(chunkId.z * world::CHUNK_WIDTH);
                pushConstants.padding = 0.0f;

                vkCmdPushConstants(cmd, m_chunkPipeline->pipelineLayout(),
                                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
            });
    }
}

void VulkanRenderer::renderEntities(VkCommandBuffer cmd) {
    if (!m_entityPipeline || !m_entityPipeline->isInitialized() || !m_entityRendererManager) {
        return;
    }

    // 绑定相机描述符集 (set = 0)
    if (!m_chunkDescriptorSets.empty() && m_chunkDescriptorSets[m_currentFrame] != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_entityPipeline->pipelineLayout(), 0, 1,
                                &m_chunkDescriptorSets[m_currentFrame], 0, nullptr);
    }

    // 调用外部回调来渲染实体
    if (m_entityRenderCallback) {
        m_entityRenderCallback(cmd, m_partialTick);
    }
}

Result<void> VulkanRenderer::onResize(u32 width, u32 height) {
    if (width == 0 || height == 0) {
        m_minimized = true;
        return Result<void>::ok();
    }

    m_minimized = false;

    if (!m_initialized) {
        return Result<void>::ok();
    }

    return recreateSwapchain();
}

VkCommandBuffer VulkanRenderer::currentCommandBuffer() const {
    if (m_imageIndex < m_commandBuffers.size()) {
        return m_commandBuffers[m_imageIndex];
    }
    return VK_NULL_HANDLE;
}

Result<void> VulkanRenderer::createRenderPass() {
    // 颜色附件
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchain->imageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 深度附件
    VkAttachmentDescription depthAttachment{};
    auto depthFormatResult = m_context->findDepthFormat();
    VkFormat depthFormat = depthFormatResult.success() ? depthFormatResult.value() : VK_FORMAT_D32_SFLOAT;
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 附件数组
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // 深度附件需要在早期片段测试阶段可用
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(m_context->device(), &renderPassInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create render pass: " + std::to_string(result));
    }

    spdlog::info("Render pass created with depth attachment");
    return Result<void>::ok();
}

Result<void> VulkanRenderer::createDepthResources() {
    auto depthFormatResult = m_context->findDepthFormat();
    VkFormat depthFormat = depthFormatResult.success() ? depthFormatResult.value() : VK_FORMAT_D32_SFLOAT;
    VkExtent2D extent = m_swapchain->extent();

    // 创建深度图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice device = m_context->device();
    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &m_depthImage);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create depth image: " + std::to_string(result));
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_depthImage, &memRequirements);

    auto memoryTypeResult = m_context->findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    if (memoryTypeResult.failed()) {
        return memoryTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_depthImageMemory);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to allocate depth image memory: " + std::to_string(result));
    }

    vkBindImageMemory(device, m_depthImage, m_depthImageMemory, 0);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device, &viewInfo, nullptr, &m_depthImageView);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create depth image view: " + std::to_string(result));
    }

    spdlog::info("Depth buffer created: {}x{}, format: {}", extent.width, extent.height, static_cast<int>(depthFormat));
    return Result<void>::ok();
}

Result<void> VulkanRenderer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = m_context->queueFamilies();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    VkResult result = vkCreateCommandPool(m_context->device(), &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create command pool: " + std::to_string(result));
    }

    spdlog::info("Command pool created");
    return Result<void>::ok();
}

Result<void> VulkanRenderer::createCommandBuffers() {
    m_commandBuffers.resize(m_swapchain->imageCount());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<u32>(m_commandBuffers.size());

    VkResult result = vkAllocateCommandBuffers(m_context->device(), &allocInfo, m_commandBuffers.data());
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to allocate command buffers: " + std::to_string(result));
    }

    spdlog::info("Command buffers created: {}", m_commandBuffers.size());
    return Result<void>::ok();
}

Result<void> VulkanRenderer::createFramebuffers() {
    m_framebuffers.resize(m_swapchain->imageCount());

    for (size_t i = 0; i < m_swapchain->imageCount(); ++i) {
        VkImageView attachments[] = {m_swapchain->imageViews()[i], m_depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchain->extent().width;
        framebufferInfo.height = m_swapchain->extent().height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(m_context->device(), &framebufferInfo,
                                              nullptr, &m_framebuffers[i]);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to create framebuffer: " + std::to_string(result));
        }
    }

    spdlog::info("Framebuffers created: {}", m_framebuffers.size());
    return Result<void>::ok();
}

Result<void> VulkanRenderer::createSyncObjects() {
    // 关键同步策略（参考Vulkan文档 swapchain_semaphore_reuse）:
    // 1. imageAvailableSemaphores: 用于vkAcquireNextImageKHR，按MAX_FRAMES_IN_FLIGHT分配
    // 2. renderFinishedSemaphores: 用于vkQueueSubmit信号和vkQueuePresentKHR等待，按交换链图像数量分配
    // 3. inFlightFences: 用于CPU等待帧完成，按MAX_FRAMES_IN_FLIGHT分配
    // 4. imageFences: 追踪每个图像正在使用的fence

    u32 imageCount = m_swapchain->imageCount();

    // 每帧一个imageAvailableSemaphore
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    // 每个交换链图像一个renderFinishedSemaphore
    m_renderFinishedSemaphores.resize(imageCount);
    // 每帧一个fence
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    // 每个交换链图像追踪其当前使用的fence
    m_imageFences.resize(imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // 创建imageAvailableSemaphores（每帧一个）
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkResult result = vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr,
                                            &m_imageAvailableSemaphores[i]);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to create image available semaphore: " + std::to_string(result));
        }
    }

    // 创建renderFinishedSemaphores（每个交换链图像一个）
    for (u32 i = 0; i < imageCount; ++i) {
        VkResult result = vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr,
                                   &m_renderFinishedSemaphores[i]);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to create render finished semaphore: " + std::to_string(result));
        }
    }

    // 创建帧fence
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkResult result = vkCreateFence(m_context->device(), &fenceInfo, nullptr,
                               &m_inFlightFences[i]);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to create fence: " + std::to_string(result));
        }
    }

    spdlog::info("Sync objects created ({} swapchain images, {} frames in flight)", imageCount, MAX_FRAMES_IN_FLIGHT);
    return Result<void>::ok();
}

void VulkanRenderer::destroyRenderPass() {
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_context->device(), m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroyDepthResources() {
    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE) return;

    if (m_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }
    if (m_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
    }
    if (m_depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_depthImageMemory, nullptr);
        m_depthImageMemory = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroyCommandPool() {
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_context->device(), m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroyCommandBuffers() {
    if (!m_commandBuffers.empty() && m_commandPool != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(m_context->device(), m_commandPool,
                             static_cast<u32>(m_commandBuffers.size()), m_commandBuffers.data());
        m_commandBuffers.clear();
    }
}

void VulkanRenderer::destroyFramebuffers() {
    for (auto framebuffer : m_framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_context->device(), framebuffer, nullptr);
        }
    }
    m_framebuffers.clear();
}

void VulkanRenderer::destroySyncObjects() {
    // 销毁imageAvailableSemaphores（每帧一个）
    for (size_t i = 0; i < m_imageAvailableSemaphores.size(); ++i) {
        if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_context->device(), m_imageAvailableSemaphores[i], nullptr);
        }
    }
    // 销毁renderFinishedSemaphores（每个交换链图像一个）
    for (size_t i = 0; i < m_renderFinishedSemaphores.size(); ++i) {
        if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_context->device(), m_renderFinishedSemaphores[i], nullptr);
        }
    }
    // 销毁帧fence
    for (size_t i = 0; i < m_inFlightFences.size(); ++i) {
        if (m_inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(m_context->device(), m_inFlightFences[i], nullptr);
        }
    }
    m_imageAvailableSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_inFlightFences.clear();
    m_imageFences.clear();
}

Result<void> VulkanRenderer::recreateSwapchain() {
    m_context->waitIdle();

    // 交换链重建会重建 RenderPass，天空管线依赖 RenderPass，需要先销毁。
    if (m_skyRendererInitialized) {
        m_skyRenderer.destroy();
        m_skyRendererInitialized = false;
    }

    destroyFramebuffers();
    destroyDepthResources();
    destroyCommandBuffers();
    destroyRenderPass();

    auto result = m_swapchain->recreate(
        m_swapchain->extent().width,
        m_swapchain->extent().height);

    if (result.failed()) {
        return result;
    }

    // 重新创建同步对象（交换链图像数量可能变化）
    destroySyncObjects();
    auto syncResult = createSyncObjects();
    if (syncResult.failed()) {
        return syncResult.error();
    }

    auto renderPassResult = createRenderPass();
    if (renderPassResult.failed()) {
        return renderPassResult.error();
    }

    auto depthResult = createDepthResources();
    if (depthResult.failed()) {
        return depthResult.error();
    }

    auto commandBufferResult = createCommandBuffers();
    if (commandBufferResult.failed()) {
        return commandBufferResult.error();
    }

    auto framebufferResult = createFramebuffers();
    if (framebufferResult.failed()) {
        return framebufferResult.error();
    }

    // 重建天空渲染器
    auto skyResult = m_skyRenderer.initialize(m_context.get(), m_renderPass, m_swapchain->extent());
    if (skyResult.failed()) {
        spdlog::warn("Failed to recreate sky renderer: {}", skyResult.error().toString());
        m_skyRendererInitialized = false;
    } else {
        m_skyRendererInitialized = true;
        m_skyRenderer.update(m_dayTime, m_gameTime, m_partialTick);
    }

    return Result<void>::ok();
}

Result<void> VulkanRenderer::createDescriptorSetLayouts() {
    VkDevice device = m_context->device();

    // 相机描述符集布局 (set 0)
    // binding 0: CameraUBO
    // binding 1: LightingUBO
    std::array<VkDescriptorSetLayoutBinding, 2> cameraBindings{};
    cameraBindings[0].binding = 0;
    cameraBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraBindings[0].descriptorCount = 1;
    cameraBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    cameraBindings[0].pImmutableSamplers = nullptr;

    cameraBindings[1].binding = 1;
    cameraBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraBindings[1].descriptorCount = 1;
    cameraBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // 实体顶点着色器也需要lighting
    cameraBindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
    cameraLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cameraLayoutInfo.bindingCount = static_cast<u32>(cameraBindings.size());
    cameraLayoutInfo.pBindings = cameraBindings.data();

    if (vkCreateDescriptorSetLayout(device, &cameraLayoutInfo, nullptr, &m_cameraDescriptorLayout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create camera descriptor set layout");
    }

    // 纹理描述符集布局 (set 1)
    // binding 0: combined image sampler
    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &textureBinding;

    if (vkCreateDescriptorSetLayout(device, &textureLayoutInfo, nullptr, &m_textureDescriptorLayout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create texture descriptor set layout");
    }

    spdlog::info("Descriptor set layouts created");
    return {};
}

Result<void> VulkanRenderer::createPipelineLayout() {
    VkDevice device = m_context->device();

    // 描述符集布局
    std::array<VkDescriptorSetLayout, 2> layouts = {
        m_cameraDescriptorLayout,
        m_textureDescriptorLayout
    };

    // 推送常量
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ModelPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<u32>(layouts.size());
    layoutInfo.pSetLayouts = layouts.data();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create pipeline layout");
    }

    spdlog::info("Pipeline layout created");
    return {};
}

Result<void> VulkanRenderer::createDescriptorPool() {
    VkDevice device = m_context->device();

    // 描述符池大小
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<u32>(MAX_FRAMES_IN_FLIGHT) * 2; // Camera + Lighting

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 16; // 支持最多16个纹理

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = static_cast<u32>(MAX_FRAMES_IN_FLIGHT) * 2 + 16;
    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor pool");
    }

    spdlog::info("Descriptor pool created");
    return {};
}

Result<void> VulkanRenderer::createUniformBuffers() {
    VkDevice device = m_context->device();
    VkPhysicalDevice physicalDevice = m_context->physicalDevice();

    // 创建相机UBO
    auto result = m_cameraUBO.create(device, physicalDevice, sizeof(CameraUBO), MAX_FRAMES_IN_FLIGHT);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create camera UBO");
    }

    // 创建光照UBO
    result = m_lightingUBO.create(device, physicalDevice, sizeof(LightingUBO), MAX_FRAMES_IN_FLIGHT);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create lighting UBO");
    }

    // 初始化默认光照
    LightingUBO lighting{};
    lighting.sunDirection[0] = 0.5f;
    lighting.sunDirection[1] = 0.8f;
    lighting.sunDirection[2] = 0.3f;
    lighting.sunIntensity = 1.0f;
    lighting.ambientColor[0] = 0.4f;
    lighting.ambientColor[1] = 0.4f;
    lighting.ambientColor[2] = 0.5f;
    lighting.ambientIntensity = 0.4f;
    lighting.cameraPosition[0] = 0.0f;
    lighting.cameraPosition[1] = 0.0f;
    lighting.cameraPosition[2] = 0.0f;
    lighting.fogColor[0] = 0.6f;
    lighting.fogColor[1] = 0.7f;
    lighting.fogColor[2] = 0.9f;
    lighting.fogStart = 100.0f;
    lighting.fogEnd = 500.0f;
    lighting.fogDensity = 0.002f;
    lighting.fogMode = 1; // 线性雾

    m_lightingUBO.update(&lighting, sizeof(lighting));

    spdlog::info("Uniform buffers created");
    return {};
}

void VulkanRenderer::destroyDescriptors() {
    VkDevice device = m_context->device();

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_cameraDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_cameraDescriptorLayout, nullptr);
        m_cameraDescriptorLayout = VK_NULL_HANDLE;
    }

    if (m_textureDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_textureDescriptorLayout, nullptr);
        m_textureDescriptorLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::updateUniformBuffers() {
    // 更新相机 UBO
    {
        MC_TRACE_UNIFORM_UPDATE("CameraUBO");
        CameraUBO cameraUBO{};

        if (m_camera) {
            // 使用相机的矩阵
            const auto& view = m_camera->viewMatrix();
            const auto& proj = m_camera->projectionMatrix();
            const auto& viewProj = m_camera->viewProjectionMatrix();

            std::memcpy(cameraUBO.view, glm::value_ptr(view), sizeof(glm::mat4));
            std::memcpy(cameraUBO.projection, glm::value_ptr(proj), sizeof(glm::mat4));
            std::memcpy(cameraUBO.viewProjection, glm::value_ptr(viewProj), sizeof(glm::mat4));
        } else {
            // 默认矩阵（用于测试）
            // 相机在较高位置，俯视区块
            // 区块范围大约是 (0,0,0) 到 (16, 50, 16)
            // 将相机放在 (8, 80, 80) 看向区块中心
            glm::mat4 view = glm::lookAt(
                glm::vec3(8.0f, 50.0f, 64.0f),   // 相机位置
                glm::vec3(8.0f, 20.0f, 8.0f),    // 看向区块中心
                glm::vec3(0.0f, 1.0f, 0.0f)      // 上方向
            );
            // 投影矩阵：透视投影
            glm::mat4 proj = glm::perspective(glm::radians(70.0f),
                                              static_cast<float>(m_swapchain->extent().width) / m_swapchain->extent().height,
                                              0.1f, 500.0f);
            // 翻转Y轴（GLM默认用于OpenGL，Vulkan需要翻转）
            proj[1][1] *= -1.0f;

            glm::mat4 viewProj = proj * view;

            std::memcpy(cameraUBO.view, glm::value_ptr(view), sizeof(glm::mat4));
            std::memcpy(cameraUBO.projection, glm::value_ptr(proj), sizeof(glm::mat4));
            std::memcpy(cameraUBO.viewProjection, glm::value_ptr(viewProj), sizeof(glm::mat4));
        }

        m_cameraUBO.update(&cameraUBO, sizeof(cameraUBO), m_currentFrame);
    }

    // 更新光照 UBO (根据时间动态更新)
    {
        MC_TRACE_UNIFORM_UPDATE("LightingUBO");
        LightingUBO lighting{};

        // 计算天体角度和太阳方向
        f32 celestialAngle = CelestialCalculations::calculateCelestialAngle(m_dayTime);
        glm::vec3 sunDir = CelestialCalculations::calculateSunDirection(celestialAngle);
        f32 sunIntensity = CelestialCalculations::calculateSunIntensity(celestialAngle);

        lighting.sunDirection[0] = sunDir.x;
        lighting.sunDirection[1] = sunDir.y;
        lighting.sunDirection[2] = sunDir.z;
        lighting.sunIntensity = sunIntensity;

        // 环境光 (夜晚较暗)
        f32 ambientBase = 0.3f + sunIntensity * 0.4f;
        lighting.ambientColor[0] = 0.4f;
        lighting.ambientColor[1] = 0.4f;
        lighting.ambientColor[2] = 0.5f;
        lighting.ambientIntensity = ambientBase;

        if (m_camera) {
            const auto& camPos = m_camera->position();
            lighting.cameraPosition[0] = camPos.x;
            lighting.cameraPosition[1] = camPos.y;
            lighting.cameraPosition[2] = camPos.z;
        } else {
            lighting.cameraPosition[0] = 0.0f;
            lighting.cameraPosition[1] = 0.0f;
            lighting.cameraPosition[2] = 2.0f;
        }

        // 根据时间计算天空/雾颜色
        glm::vec4 skyColor = CelestialCalculations::calculateSkyColor(celestialAngle);
        glm::vec4 fogColor = CelestialCalculations::calculateFogColor(celestialAngle);

        lighting.fogColor[0] = fogColor.r;
        lighting.fogColor[1] = fogColor.g;
        lighting.fogColor[2] = fogColor.b;
        lighting.fogStart = 100.0f;
        lighting.fogEnd = 500.0f;
        lighting.fogDensity = 0.002f;
        lighting.fogMode = 1;

        // 时间相关字段
        lighting.celestialAngle = celestialAngle;
        lighting.skyBrightness = sunIntensity;
        lighting.moonPhase = CelestialCalculations::calculateMoonPhase(m_gameTime);
        lighting.starBrightness = CelestialCalculations::calculateStarBrightness(celestialAngle);

        m_lightingUBO.update(&lighting, sizeof(lighting), m_currentFrame);
    }
}

void VulkanRenderer::updateTime(i64 dayTime, i64 gameTime, f32 partialTick) {
    m_dayTime = dayTime;
    m_gameTime = gameTime;
    m_partialTick = partialTick;

    // 更新天空渲染器
    if (m_skyRendererInitialized) {
        m_skyRenderer.update(dayTime, gameTime, partialTick);
    }
}

Result<void> VulkanRenderer::createChunkPipeline() {
    assert(m_context && m_context->device() != VK_NULL_HANDLE && "Context must be valid");
    assert(m_renderPass != VK_NULL_HANDLE && "Render pass must be valid");

    m_chunkPipeline = std::make_unique<VulkanPipeline>();

    PipelineConfig config{};

    const auto chunkVertPath = resolveShaderPath("chunk.vert.spv");
    const auto chunkFragPath = resolveShaderPath("chunk.frag.spv");
    if (chunkVertPath.empty() || chunkFragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve chunk shader binaries");
    }

    // 着色器路径 - 优先使用最新构建产物
    config.vertexShaderPath = chunkVertPath.string();
    config.fragmentShaderPath = chunkFragPath.string();

    // 顶点输入描述 - 与Vertex结构匹配
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    config.vertexBindings.push_back(bindingDesc);

    // position (location = 0)
    VkVertexInputAttributeDescription attr0{};
    attr0.binding = 0;
    attr0.location = 0;
    attr0.format = VK_FORMAT_R32G32B32_SFLOAT;
    attr0.offset = offsetof(Vertex, x);
    config.vertexAttributes.push_back(attr0);

    // normal (location = 1)
    VkVertexInputAttributeDescription attr1{};
    attr1.binding = 0;
    attr1.location = 1;
    attr1.format = VK_FORMAT_R32G32B32_SFLOAT;
    attr1.offset = offsetof(Vertex, nx);
    config.vertexAttributes.push_back(attr1);

    // texCoord (location = 2)
    VkVertexInputAttributeDescription attr2{};
    attr2.binding = 0;
    attr2.location = 2;
    attr2.format = VK_FORMAT_R32G32_SFLOAT;
    attr2.offset = offsetof(Vertex, u);
    config.vertexAttributes.push_back(attr2);

    // color (location = 3) - RGBA8
    VkVertexInputAttributeDescription attr3{};
    attr3.binding = 0;
    attr3.location = 3;
    attr3.format = VK_FORMAT_R8G8B8A8_UNORM;
    attr3.offset = offsetof(Vertex, color);
    config.vertexAttributes.push_back(attr3);

    // light (location = 4)
    VkVertexInputAttributeDescription attr4{};
    attr4.binding = 0;
    attr4.location = 4;
    attr4.format = VK_FORMAT_R8_UNORM;
    attr4.offset = offsetof(Vertex, light);
    config.vertexAttributes.push_back(attr4);

    // 光栅化配置
    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.polygonMode = VK_POLYGON_MODE_FILL;
    config.cullMode = VK_CULL_MODE_BACK_BIT;  // 启用背面剔除
    config.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // 由于投影矩阵翻转Y轴，屏幕空间缠绕顺序反转

    // 深度测试
    config.depthTestEnable = VK_TRUE;   // 启用深度测试
    config.depthWriteEnable = VK_TRUE;  // 启用深度写入

    // 渲染通道
    config.renderPass = m_renderPass;
    config.subpass = 0;

    // 描述符集布局 - camera (set 0) 和 texture (set 1)
    config.descriptorSetLayouts.push_back(m_cameraDescriptorLayout);
    config.descriptorSetLayouts.push_back(m_textureDescriptorLayout);

    // 推送常量
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ModelPushConstants);
    config.pushConstantRanges.push_back(pushConstantRange);

    auto result = m_chunkPipeline->initialize(m_context.get(), config);
    if (result.failed()) {
        return result.error();
    }

    // 为每帧创建描述符集（set 0: camera）
    m_chunkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_cameraDescriptorLayout;

        VkResult vkResult = vkAllocateDescriptorSets(m_context->device(), &allocInfo, &m_chunkDescriptorSets[i]);
        if (vkResult != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to allocate chunk descriptor set: " + std::to_string(vkResult));
        }

        // 更新描述符集 - binding 0: CameraUBO, binding 1: LightingUBO
        std::array<VkDescriptorBufferInfo, 2> bufferInfos{};
        bufferInfos[0].buffer = m_cameraUBO.buffer(i).buffer();
        bufferInfos[0].offset = 0;
        bufferInfos[0].range = sizeof(CameraUBO);

        bufferInfos[1].buffer = m_lightingUBO.buffer(i).buffer();
        bufferInfos[1].offset = 0;
        bufferInfos[1].range = sizeof(LightingUBO);

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        // CameraUBO
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_chunkDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfos[0];

        // LightingUBO
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_chunkDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &bufferInfos[1];

        vkUpdateDescriptorSets(m_context->device(), static_cast<u32>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    spdlog::info("Chunk pipeline created");
    return Result<void>::ok();
}

Result<void> VulkanRenderer::createChunkTextureAtlas() {
    assert(m_context && m_context->device() != VK_NULL_HANDLE && "Context must be valid");

    VkDevice device = m_context->device();
    VkPhysicalDevice physicalDevice = m_context->physicalDevice();

    // 生成默认纹理图集
    auto pixels = DefaultTextureAtlas::generate();

    // 创建纹理图集 (正方形)
    u32 atlasSize = DefaultTextureAtlas::atlasSize();
    auto result = m_chunkTextureAtlas.create(device, physicalDevice,
                                              atlasSize, atlasSize);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create chunk texture atlas: " + result.error().toString());
    }

    // 创建暂存缓冲区
    VulkanBuffer stagingBuffer;
    VkDeviceSize imageSize = pixels.size();
    result = stagingBuffer.create(device, physicalDevice, imageSize,
                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create staging buffer: " + result.error().toString());
    }

    // 复制数据到暂存缓冲区
    auto mapResult = stagingBuffer.map();
    if (mapResult.failed() || !mapResult.value()) {
        stagingBuffer.destroy();
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }
    void* data = mapResult.value();
    std::memcpy(data, pixels.data(), pixels.size());
    stagingBuffer.unmap();

    // 分配命令缓冲区用于上传
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // 开始录制命令
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // 上传纹理 (数据已在 staging buffer 中)
    auto uploadResult = m_chunkTextureAtlas.upload(commandBuffer, stagingBuffer);
    if (!uploadResult.success()) {
        vkEndCommandBuffer(commandBuffer);
        vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
        stagingBuffer.destroy();
        return Error(ErrorCode::OperationFailed, "Failed to upload texture atlas: " + uploadResult.error().toString());
    }

    vkEndCommandBuffer(commandBuffer);

    // 提交命令
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_context->graphicsQueue());

    // 清理
    vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
    stagingBuffer.destroy();

    // 创建纹理描述符集
    VkDescriptorSetAllocateInfo descAllocInfo{};
    descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAllocInfo.descriptorPool = m_descriptorPool;
    descAllocInfo.descriptorSetCount = 1;
    descAllocInfo.pSetLayouts = &m_textureDescriptorLayout;

    VkResult vkResult = vkAllocateDescriptorSets(device, &descAllocInfo, &m_chunkTextureDescriptorSet);
    if (vkResult != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to allocate chunk texture descriptor set: " + std::to_string(vkResult));
    }

    // 更新纹理描述符集
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_chunkTextureAtlas.texture().imageView();
    imageInfo.sampler = m_chunkTextureAtlas.texture().sampler();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_chunkTextureDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    // 初始化ChunkRenderer
    result = m_chunkRenderer.initialize(device, physicalDevice, m_commandPool, m_context->graphicsQueue());
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to initialize ChunkRenderer: " + result.error().toString());
    }
    m_chunkRendererInitialized = true;

    // 注意：BlockModelRegistry 已移除，方块模型现在通过 BlockModelCache 从资源包加载
    // ChunkMesher 需要在初始化时设置 BlockModelCache

    spdlog::info("Chunk texture atlas created ({}x{}, tile: {})",
                 DefaultTextureAtlas::atlasSize(), DefaultTextureAtlas::atlasSize(), DefaultTextureAtlas::tileSize());
    return Result<void>::ok();
}

void VulkanRenderer::destroyChunkResources() {
    m_chunkRenderer.destroy();
    m_chunkRendererInitialized = false;
    m_chunkTextureAtlas.destroy();

    if (!m_chunkDescriptorSets.empty() && m_descriptorPool != VK_NULL_HANDLE && m_context) {
        for (auto& ds : m_chunkDescriptorSets) {
            if (ds != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(m_context->device(), m_descriptorPool, 1, &ds);
                ds = VK_NULL_HANDLE;
            }
        }
        m_chunkDescriptorSets.clear();
    }

    if (m_chunkTextureDescriptorSet != VK_NULL_HANDLE && m_descriptorPool != VK_NULL_HANDLE && m_context) {
        vkFreeDescriptorSets(m_context->device(), m_descriptorPool, 1, &m_chunkTextureDescriptorSet);
        m_chunkTextureDescriptorSet = VK_NULL_HANDLE;
    }

    m_chunkPipeline.reset();
}

// ============================================================================
// GUI渲染
// ============================================================================

Result<void> VulkanRenderer::createGuiRenderer() {
    spdlog::info("Creating GUI renderer...");

    // 创建GUI渲染器
    m_guiRenderer = std::make_unique<GuiRenderer>();

    // 初始化GUI渲染器
    auto result = m_guiRenderer->initialize(m_context.get(), m_renderPass);
    if (result.failed()) {
        m_guiRenderer.reset();
        return result.error();
    }

    // 初始化默认字体
    auto fontResult = DefaultAsciiFont::create(m_font);
    if (fontResult.failed()) {
        spdlog::warn("Failed to create default font: {}", fontResult.error().toString());
    } else {
        m_guiRenderer->setFont(&m_font);
        // 设置字体缩放为1.5倍，让调试屏幕文字更清晰可读
        m_guiRenderer->setFontScale(1.5f);
        spdlog::info("Default ASCII font loaded");
    }

    m_guiRendererInitialized = true;
    spdlog::info("GUI renderer created successfully");
    return Result<void>::ok();
}
void VulkanRenderer::destroyGuiResources() {
    if (!m_guiRendererInitialized) {
        return;
    }

    m_itemRenderer.reset();
    m_itemTextureAtlas.destroy();
    m_itemTextureAtlasInitialized = false;
    m_guiRenderer.reset();
    m_font.destroy();
    m_guiRendererInitialized = false;
    spdlog::info("GUI renderer destroyed");
}

void VulkanRenderer::renderGui(VkCommandBuffer cmd) {
    if (!m_guiRenderer || !m_guiRendererInitialized) {
        return;
    }

    // 获取屏幕尺寸
    f32 screenW = static_cast<f32>(m_swapchain->extent().width);
    f32 screenH = static_cast<f32>(m_swapchain->extent().height);

    // 开始GUI帧
    {
        MC_TRACE_GUI("BeginFrame");
        m_guiRenderer->beginFrame(screenW, screenH);
    }

    // 调用GUI渲染回调，让外部绘制GUI内容
    {
        MC_TRACE_GUI("RenderCallback");
        if (m_guiRenderCallback) {
            m_guiRenderCallback();
        }
    }

    // 渲染GUI（顶点数据使用HOST_VISIBLE内存直接上传）
    {
        MC_TRACE_GUI("DrawCall");
        m_guiRenderer->render(cmd);
    }
}

Result<void> VulkanRenderer::updateTextureAtlas(const AtlasBuildResult& atlasResult) {
    if (!m_context || m_context->device() == VK_NULL_HANDLE) {
        return Error(ErrorCode::InitializationFailed, "Context not initialized");
    }

    if (atlasResult.pixels.empty()) {
        return Error(ErrorCode::InvalidArgument, "Atlas result has no pixel data");
    }

    VkDevice device = m_context->device();
    VkPhysicalDevice physicalDevice = m_context->physicalDevice();

    // 如果纹理图集已存在，先销毁
    if (m_chunkTextureAtlas.isValid()) {
        // 等待设备空闲
        vkDeviceWaitIdle(device);
        m_chunkTextureAtlas.destroy();
    }

    // 创建新的纹理图集
    u32 atlasWidth = atlasResult.width;
    u32 atlasHeight = atlasResult.height;

    auto result = m_chunkTextureAtlas.create(device, physicalDevice,
                                              atlasWidth, atlasHeight);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create texture atlas: " + result.error().toString());
    }

    // 创建暂存缓冲区
    VulkanBuffer stagingBuffer;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(atlasResult.pixels.size());
    result = stagingBuffer.create(device, physicalDevice, imageSize,
                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create staging buffer: " + result.error().toString());
    }

    // 复制数据到暂存缓冲区
    auto mapResult = stagingBuffer.map();
    if (mapResult.failed() || !mapResult.value()) {
        stagingBuffer.destroy();
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }
    void* data = mapResult.value();
    std::memcpy(data, atlasResult.pixels.data(), atlasResult.pixels.size());
    stagingBuffer.unmap();

    // 分配命令缓冲区用于上传
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // 开始录制命令
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // 上传纹理 (数据已在 staging buffer 中)
    auto uploadResult = m_chunkTextureAtlas.upload(commandBuffer, stagingBuffer);
    if (!uploadResult.success()) {
        vkEndCommandBuffer(commandBuffer);
        vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
        stagingBuffer.destroy();
        return Error(ErrorCode::OperationFailed, "Failed to upload texture atlas: " + uploadResult.error().toString());
    }

    vkEndCommandBuffer(commandBuffer);

    // 提交命令
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_context->graphicsQueue());

    // 清理
    vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
    stagingBuffer.destroy();

    // 更新纹理描述符集
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_chunkTextureAtlas.texture().imageView();
    imageInfo.sampler = m_chunkTextureAtlas.texture().sampler();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_chunkTextureDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    // 更新实体管线的纹理描述符
    if (m_entityPipeline && m_entityPipeline->isInitialized()) {
        m_entityPipeline->setTextureAtlas(
            m_chunkTextureAtlas.texture().imageView(),
            m_chunkTextureAtlas.texture().sampler());
    }

    // 保存纹理区域映射
    m_textureRegions = atlasResult.regions;

    spdlog::info("Renderer texture atlas updated: {}x{}, {} textures",
                 atlasWidth, atlasHeight, atlasResult.regions.size());

    return Result<void>::ok();
}

const TextureRegion* VulkanRenderer::getTextureRegion(const ResourceLocation& location) const {
    auto it = m_textureRegions.find(location);
    if (it != m_textureRegions.end()) {
        return &it->second;
    }
    return nullptr;
}

Result<void> VulkanRenderer::initializeItemRenderer(ResourceManager* resourceManager) {
    if (resourceManager == nullptr) {
        return Error(ErrorCode::NullPointer, "ResourceManager is null");
    }

    spdlog::info("Initializing item renderer...");

    VkDevice device = m_context->device();
    VkPhysicalDevice physicalDevice = m_context->physicalDevice();

    // 创建物品纹理图集
    auto result = m_itemTextureAtlas.create(device, physicalDevice, 256, 256);
    if (!result.success()) {
        return result.error();
    }

    // 加载物品纹理
    // TODO: 需要传入资源包列表
    // auto loadResult = m_itemTextureAtlas.loadFromResourcePacks(resourcePacks);
    // if (!loadResult.success()) {
    //     spdlog::warn("Failed to load item textures: {}", loadResult.error().toString());
    // }

    // 创建暂存缓冲区用于上传纹理
    VulkanBuffer stagingBuffer;
    auto stagingResult = stagingBuffer.create(device, physicalDevice, 256 * 256 * 4,
                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!stagingResult.success()) {
        m_itemTextureAtlas.destroy();
        return stagingResult.error();
    }

    // 分配命令缓冲区用于上传
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // 开始录制命令
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // 上传纹理
    auto uploadResult = m_itemTextureAtlas.upload(commandBuffer, stagingBuffer);
    if (!uploadResult.success()) {
        vkEndCommandBuffer(commandBuffer);
        vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
        stagingBuffer.destroy();
        m_itemTextureAtlas.destroy();
        return uploadResult.error();
    }

    vkEndCommandBuffer(commandBuffer);

    // 提交命令
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_context->graphicsQueue());

    // 清理
    vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
    stagingBuffer.destroy();

    // 设置GUI渲染器的物品纹理
    if (m_guiRenderer && m_itemTextureAtlas.isValid()) {
        m_guiRenderer->setItemTextureAtlas(
            m_itemTextureAtlas.texture().imageView(),
            m_itemTextureAtlas.sampler());
    }

    // 创建物品渲染器
    m_itemRenderer = std::make_unique<ItemRenderer>();
    auto rendererResult = m_itemRenderer->initialize(resourceManager, &m_itemTextureAtlas);
    if (!rendererResult.success()) {
        m_itemTextureAtlas.destroy();
        m_itemRenderer.reset();
        return rendererResult.error();
    }

    m_itemTextureAtlasInitialized = true;
    spdlog::info("Item renderer initialized successfully");
    return Result<void>::ok();
}

// ============================================================================
// 实体渲染
// ============================================================================

Result<void> VulkanRenderer::createEntityPipeline() {
    spdlog::info("Creating entity pipeline...");

    m_entityPipeline = std::make_unique<EntityPipeline>();

    auto result = m_entityPipeline->initialize(
        m_context.get(),
        m_renderPass,
        m_cameraDescriptorLayout,
        m_descriptorPool,
        m_commandPool);

    if (result.failed()) {
        m_entityPipeline.reset();
        spdlog::warn("Failed to create entity pipeline: {}", result.error().toString());
        return result.error();
    }

    // 如果实体纹理图集已经初始化，设置到管线
    // 否则在 initializeEntityTextureAtlas 中设置
    if (m_entityTextureAtlasInitialized && m_entityTextureAtlas.isBuilt()) {
        m_entityPipeline->setTextureAtlas(
            m_entityTextureAtlas.imageView(),
            m_entityTextureAtlas.sampler());
    }

    // 初始化实体渲染器管理器
    m_entityRendererManager = std::make_unique<renderer::EntityRendererManager>();
    m_entityRendererManager->setPipeline(m_entityPipeline.get());
    if (m_entityTextureAtlasInitialized && m_entityTextureAtlas.isBuilt()) {
        m_entityRendererManager->setTextureAtlas(&m_entityTextureAtlas);
    }
    m_entityRendererManager->initializeDefaults();

    m_entityRendererInitialized = true;
    spdlog::info("Entity pipeline created successfully");
    return Result<void>::ok();
}

void VulkanRenderer::destroyEntityResources() {
    if (!m_entityRendererInitialized) {
        return;
    }

    m_entityRendererManager.reset();
    m_entityPipeline.reset();
    m_entityTextureAtlas.destroy();
    m_entityRendererInitialized = false;
    m_entityTextureAtlasInitialized = false;
    spdlog::info("Entity resources destroyed");
}

Result<void> VulkanRenderer::initializeEntityTextureAtlas(ResourceManager* resourceManager) {
    if (m_entityTextureAtlasInitialized) {
        return Result<void>::ok();
    }

    if (!resourceManager) {
        return Error(ErrorCode::NullPointer, "ResourceManager is null");
    }

    spdlog::info("Initializing entity texture atlas...");

    // 初始化实体纹理图集
    auto initResult = m_entityTextureAtlas.initialize(
        m_context.get(),
        m_commandPool,
        256,   // 最大纹理数量
        64     // 默认纹理尺寸
    );
    if (initResult.failed()) {
        return initResult.error();
    }

    // 遍历所有资源包加载实体纹理（从后往前，优先使用后添加的资源包）
    u32 loadedCount = 0;
    size_t packCount = resourceManager->resourcePackCount();

    for (size_t i = packCount; i > 0; --i) {
        auto* pack = resourceManager->getResourcePack(i - 1);
        if (!pack) continue;

        EntityTextureLoader textureLoader;
        auto loaderResult = textureLoader.loadDefaultTextures(*pack, m_entityTextureAtlas);
        if (loaderResult.success() && loaderResult.value() > 0) {
            loadedCount += loaderResult.value();
            spdlog::info("Loaded {} entity textures from resource pack", loaderResult.value());
        }
    }

    // 构建纹理图集
    if (loadedCount > 0) {
        auto buildResult = m_entityTextureAtlas.build();
        if (buildResult.failed()) {
            spdlog::error("Failed to build entity texture atlas: {}", buildResult.error().toString());
            return buildResult.error();
        }

        // 设置纹理到管线
        if (m_entityPipeline && m_entityPipeline->isInitialized()) {
            m_entityPipeline->setTextureAtlas(
                m_entityTextureAtlas.imageView(),
                m_entityTextureAtlas.sampler()
            );
            spdlog::info("Entity texture atlas set to pipeline");
        }

        if (m_entityRendererManager) {
            m_entityRendererManager->setTextureAtlas(&m_entityTextureAtlas);
        }
    } else {
        spdlog::warn("No entity textures loaded from any resource pack");
    }

    m_entityTextureAtlasInitialized = true;
    spdlog::info("Entity texture atlas initialized with {} total textures", loadedCount);
    return Result<void>::ok();
}

} // namespace mc::client
