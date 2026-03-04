#include "VulkanRenderer.hpp"
#include "VulkanBuffer.hpp"
#include "DefaultTextureAtlas.hpp"
#include "../../common/renderer/ChunkMesher.hpp"
#include "../../common/renderer/MeshTypes.hpp"
#include "../../common/world/WorldConstants.hpp"
#include <spdlog/spdlog.h>
#include <array>
#include <cstring>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace mr::client {

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

    // 创建测试管线和三角形
    auto pipelineResult = createTestPipeline();
    if (pipelineResult.failed()) {
        return pipelineResult.error();
    }

    auto triangleResult = createTestTriangle();
    if (triangleResult.failed()) {
        return triangleResult.error();
    }

    auto textureResult = createTestTexture();
    if (textureResult.failed()) {
        spdlog::warn("Failed to create test texture: {}", textureResult.error().toString());
        // 继续运行，只是没有纹理
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

    destroyChunkResources();
    destroyTestResources();

    m_testPipeline.reset();
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
    if (!m_initialized || m_minimized) {
        return Error(ErrorCode::InvalidState, "Renderer not ready");
    }

    if (m_frameStarted) {
        return Error(ErrorCode::InvalidState, "Frame already started");
    }

    // 等待当前帧的fence（限制同时进行的帧数）
    vkWaitForFences(m_context->device(), 1, &m_inFlightFences[m_currentFrame],
                    VK_TRUE, UINT64_MAX);

    // 获取下一帧图像，使用当前帧索引对应的信号量
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

    // 检查是否有其他帧正在使用此图像
    if (m_imageFences[m_imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_context->device(), 1, &m_imageFences[m_imageIndex], VK_TRUE, UINT64_MAX);
    }
    // 标记此图像正在被当前帧使用
    m_imageFences[m_imageIndex] = m_inFlightFences[m_currentFrame];

    // 重置fence
    vkResetFences(m_context->device(), 1, &m_inFlightFences[m_currentFrame]);

    // 开始命令缓冲区
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

    // 开始渲染通道
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[m_imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain->extent();

    // 清除值：颜色 + 深度
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;
    clearValues[1].depthStencil.stencil = 0;
    renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    m_frameStarted = true;
    return Result<void>::ok();
}

Result<void> VulkanRenderer::endFrame() {
    if (!m_frameStarted) {
        return Error(ErrorCode::InvalidState, "Frame not started");
    }

    VkCommandBuffer cmd = m_commandBuffers[m_imageIndex];

    // 结束渲染通道
    vkCmdEndRenderPass(cmd);

    // 结束命令缓冲区
    VkResult result = vkEndCommandBuffer(cmd);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to end command buffer: " + std::to_string(result));
    }

    // 提交命令缓冲区
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

    result = vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo,
                           m_inFlightFences[m_currentFrame]);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to submit draw command buffer: " + std::to_string(result));
    }

    // 呈现，使用当前图像的renderFinishedSemaphore
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
    updateUniformBuffers();

    VkCommandBuffer cmd = m_commandBuffers[m_imageIndex];
    assert(cmd != VK_NULL_HANDLE && "Command buffer must be valid");

    // 设置视口和裁剪
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

    // 渲染区块
    if (m_chunkRendererInitialized && m_chunkRenderer.chunkCount() > 0) {
        renderChunks(cmd);
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
    m_chunkPipeline->bind(cmd);

    // 绑定相机描述符集 (set = 0)
    if (!m_chunkDescriptorSets.empty() && m_chunkDescriptorSets[m_currentFrame] != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_chunkPipeline->pipelineLayout(), 0, 1,
                                &m_chunkDescriptorSets[m_currentFrame], 0, nullptr);
    }

    // 绑定纹理描述符集 (set = 1)
    if (m_chunkTextureDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_chunkPipeline->pipelineLayout(), 1, 1,
                                &m_chunkTextureDescriptorSet, 0, nullptr);
    }

    // 渲染所有区块，为每个区块设置正确的世界偏移
    m_chunkRenderer.render(cmd, m_chunkPipeline->pipelineLayout(),
        [this, cmd](const ChunkId& chunkId) {
            // 设置推送常量 - 区块世界偏移
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

void VulkanRenderer::renderTestTriangle(VkCommandBuffer cmd) {
    if (!m_testPipeline || m_testPipeline->pipeline() == VK_NULL_HANDLE) {
        return;
    }

    // 绑定管线
    m_testPipeline->bind(cmd);

    // 绑定描述符集（使用当前帧的描述符集）
    if (!m_testDescriptorSets.empty() && m_testDescriptorSets[m_currentFrame] != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_testPipeline->pipelineLayout(), 0, 1, &m_testDescriptorSets[m_currentFrame], 0, nullptr);
    } else {
        spdlog::warn("No descriptor set available for frame {}", m_currentFrame);
    }

    // 绑定纹理描述符集 (set = 1)
    if (m_testTextureDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_testPipeline->pipelineLayout(), 1, 1, &m_testTextureDescriptorSet, 0, nullptr);
    }

    // 设置推送常量 - 单位矩阵
    ModelPushConstants pushConstants{};
    // 初始化为0
    std::memset(&pushConstants, 0, sizeof(pushConstants));
    // 设置单位矩阵 (列主序)
    pushConstants.model[0] = 1.0f;  // [0][0]
    pushConstants.model[5] = 1.0f;  // [1][1]
    pushConstants.model[10] = 1.0f; // [2][2]
    pushConstants.model[15] = 1.0f; // [3][3]
    pushConstants.chunkOffset[0] = 0.0f;
    pushConstants.chunkOffset[1] = 0.0f;
    pushConstants.chunkOffset[2] = 0.0f;
    pushConstants.padding = 0.0f;

    vkCmdPushConstants(cmd, m_testPipeline->pipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

    // 绑定顶点缓冲区
    if (m_testVertexBuffer.isValid()) {
        VkBuffer vertexBuffers[] = {m_testVertexBuffer.buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    } else {
        spdlog::error("Test vertex buffer is not valid!");
    }

    // 绑定索引缓冲区并绘制
    if (m_testIndexBuffer.isValid()) {
        vkCmdBindIndexBuffer(cmd, m_testIndexBuffer.buffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, m_testIndexCount, 1, 0, 0, 0);
    } else if (m_testVertexBuffer.isValid()) {
        // 没有索引缓冲区，直接绘制顶点
        vkCmdDraw(cmd, 3, 1, 0, 0);
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
    cameraBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
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

    // 更新光照UBO
    LightingUBO lighting{};
    lighting.sunDirection[0] = 0.5f;
    lighting.sunDirection[1] = 0.8f;
    lighting.sunDirection[2] = 0.3f;
    lighting.sunIntensity = 1.0f;
    lighting.ambientColor[0] = 0.4f;
    lighting.ambientColor[1] = 0.4f;
    lighting.ambientColor[2] = 0.5f;
    lighting.ambientIntensity = 0.4f;

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

    lighting.fogColor[0] = 0.6f;
    lighting.fogColor[1] = 0.7f;
    lighting.fogColor[2] = 0.9f;
    lighting.fogStart = 100.0f;
    lighting.fogEnd = 500.0f;
    lighting.fogDensity = 0.002f;
    lighting.fogMode = 1;

    m_lightingUBO.update(&lighting, sizeof(lighting), m_currentFrame);
}

Result<void> VulkanRenderer::createTestPipeline() {
    assert(m_context && m_context->device() != VK_NULL_HANDLE && "Context must be valid");
    assert(m_renderPass != VK_NULL_HANDLE && "Render pass must be valid");
    assert(m_pipelineLayout != VK_NULL_HANDLE && "Pipeline layout must be valid");

    m_testPipeline = std::make_unique<VulkanPipeline>();

    PipelineConfig config{};

    // 着色器路径 - 使用带纹理的着色器
    config.vertexShaderPath = "shaders/textured.vert.spv";
    config.fragmentShaderPath = "shaders/textured.frag.spv";

    // 顶点输入描述 - 与TestVertex结构匹配
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(TestVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    config.vertexBindings.push_back(bindingDesc);

    // position (location = 0)
    VkVertexInputAttributeDescription attr0{};
    attr0.binding = 0;
    attr0.location = 0;
    attr0.format = VK_FORMAT_R32G32B32_SFLOAT;
    attr0.offset = offsetof(TestVertex, pos);
    config.vertexAttributes.push_back(attr0);

    // normal (location = 1)
    VkVertexInputAttributeDescription attr1{};
    attr1.binding = 0;
    attr1.location = 1;
    attr1.format = VK_FORMAT_R32G32B32_SFLOAT;
    attr1.offset = offsetof(TestVertex, normal);
    config.vertexAttributes.push_back(attr1);

    // texCoord (location = 2)
    VkVertexInputAttributeDescription attr2{};
    attr2.binding = 0;
    attr2.location = 2;
    attr2.format = VK_FORMAT_R32G32_SFLOAT;
    attr2.offset = offsetof(TestVertex, texCoord);
    config.vertexAttributes.push_back(attr2);

    // color (location = 3)
    VkVertexInputAttributeDescription attr3{};
    attr3.binding = 0;
    attr3.location = 3;
    attr3.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attr3.offset = offsetof(TestVertex, color);
    config.vertexAttributes.push_back(attr3);

    // light (location = 4)
    VkVertexInputAttributeDescription attr4{};
    attr4.binding = 0;
    attr4.location = 4;
    attr4.format = VK_FORMAT_R32_SFLOAT;
    attr4.offset = offsetof(TestVertex, light);
    config.vertexAttributes.push_back(attr4);

    // 光栅化配置
    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.polygonMode = VK_POLYGON_MODE_FILL;
    config.cullMode = VK_CULL_MODE_NONE;  // 暂时禁用剔除以便调试
    config.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // 深度测试
    config.depthTestEnable = VK_FALSE;  // 测试三角形不需要深度测试
    config.depthWriteEnable = VK_FALSE;

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

    // 使用已创建的管线布局
    // 注意：VulkanPipeline会创建自己的管线布局，但我们想用现有的
    // 所以需要修改VulkanPipeline来支持外部布局，或者使用现有布局
    // 这里我们先让VulkanPipeline创建新的布局

    auto result = m_testPipeline->initialize(m_context.get(), config);
    if (result.failed()) {
        return result.error();
    }

    spdlog::info("Test pipeline created");
    return Result<void>::ok();
}

Result<void> VulkanRenderer::createTestTriangle() {
    assert(m_context && m_context->device() != VK_NULL_HANDLE && "Context must be valid");

    // 定义彩色三角形顶点
    TestVertex vertices[] = {
        // 位置                法线              纹理坐标      颜色               光照
        {{ 0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, 1.0f},  // 顶部 - 红色
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, 1.0f},  // 右下 - 绿色
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, 1.0f},  // 左下 - 蓝色
    };

    u32 indices[] = {0, 1, 2};
    m_testIndexCount = 3;

    VkDevice device = m_context->device();
    VkPhysicalDevice physicalDevice = m_context->physicalDevice();

    // 创建顶点缓冲区
    auto result = m_testVertexBuffer.create(
        device,
        physicalDevice,
        sizeof(vertices),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (result.failed()) {
        return Error(ErrorCode::OutOfMemory, "Failed to create test vertex buffer: " + result.error().toString());
    }

    // 上传顶点数据
    void* data = nullptr;
    VkResult vkResult = vkMapMemory(device, m_testVertexBuffer.memory(), 0, sizeof(vertices), 0, &data);
    if (vkResult != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to map vertex buffer memory");
    }
    std::memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(device, m_testVertexBuffer.memory());

    // 创建索引缓冲区
    result = m_testIndexBuffer.create(
        device,
        physicalDevice,
        sizeof(indices),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (result.failed()) {
        return Error(ErrorCode::OutOfMemory, "Failed to create test index buffer: " + result.error().toString());
    }

    // 上传索引数据
    vkResult = vkMapMemory(device, m_testIndexBuffer.memory(), 0, sizeof(indices), 0, &data);
    if (vkResult != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to map index buffer memory");
    }
    std::memcpy(data, indices, sizeof(indices));
    vkUnmapMemory(device, m_testIndexBuffer.memory());

    // 为每一帧创建描述符集
    m_testDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_cameraDescriptorLayout;

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkResult = vkAllocateDescriptorSets(device, &allocInfo, &m_testDescriptorSets[i]);
        if (vkResult != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to allocate descriptor set: " + std::to_string(vkResult));
        }

        // 更新描述符集
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_cameraUBO.buffer(i).buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraUBO);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_testDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    spdlog::info("Test triangle created with {} vertices, {} indices", 3, m_testIndexCount);
    return Result<void>::ok();
}

void VulkanRenderer::destroyTestResources() {
    m_testVertexBuffer.destroy();
    m_testIndexBuffer.destroy();

    if (!m_testDescriptorSets.empty() && m_descriptorPool != VK_NULL_HANDLE && m_context) {
        for (auto& ds : m_testDescriptorSets) {
            if (ds != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(m_context->device(), m_descriptorPool, 1, &ds);
                ds = VK_NULL_HANDLE;
            }
        }
        m_testDescriptorSets.clear();
    }

    // 销毁纹理描述符集
    if (m_testTextureDescriptorSet != VK_NULL_HANDLE && m_descriptorPool != VK_NULL_HANDLE && m_context) {
        vkFreeDescriptorSets(m_context->device(), m_descriptorPool, 1, &m_testTextureDescriptorSet);
        m_testTextureDescriptorSet = VK_NULL_HANDLE;
    }

    m_testTexture.destroy();
}

Result<void> VulkanRenderer::createTestTexture() {
    assert(m_context && m_context->device() != VK_NULL_HANDLE && "Context must be valid");

    VkDevice device = m_context->device();
    VkPhysicalDevice physicalDevice = m_context->physicalDevice();

    // 创建一个简单的棋盘格测试纹理 (64x64 RGBA)
    constexpr u32 texSize = 64;
    std::vector<u8> pixels(texSize * texSize * 4);

    for (u32 y = 0; y < texSize; ++y) {
        for (u32 x = 0; x < texSize; ++x) {
            u32 index = (y * texSize + x) * 4;
            // 棋盘格图案
            bool isWhite = ((x / 8) + (y / 8)) % 2 == 0;
            if (isWhite) {
                pixels[index + 0] = 255; // R
                pixels[index + 1] = 255; // G
                pixels[index + 2] = 255; // B
            } else {
                pixels[index + 0] = 64;  // R
                pixels[index + 1] = 64;  // G
                pixels[index + 2] = 128; // B (蓝灰色)
            }
            pixels[index + 3] = 255; // A
        }
    }

    // 创建纹理
    TextureConfig texConfig{};
    texConfig.width = texSize;
    texConfig.height = texSize;
    texConfig.format = VK_FORMAT_R8G8B8A8_UNORM;  // 使用UNORM避免SRGB转换
    texConfig.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    texConfig.mipLevels = 1;

    auto result = m_testTexture.create(device, physicalDevice, texConfig);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create test texture: " + result.error().toString());
    }

    // 创建image view
    result = m_testTexture.createImageView();
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create test texture view: " + result.error().toString());
    }

    // 创建sampler
    result = m_testTexture.createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create test texture sampler: " + result.error().toString());
    }

    // 上传纹理数据 - 需要暂存缓冲区和命令缓冲区
    // 创建暂存缓冲区
    VulkanBuffer stagingBuffer;
    VkDeviceSize imageSize = texSize * texSize * 4;
    result = stagingBuffer.create(device, physicalDevice, imageSize,
                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!result.success()) {
        return Error(ErrorCode::InitializationFailed, "Failed to create staging buffer: " + result.error().toString());
    }

    // 复制数据到暂存缓冲区
    void* data = stagingBuffer.map().value();
    if (!data) {
        stagingBuffer.destroy();
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }
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

    // 上传纹理
    auto uploadResult = m_testTexture.upload(commandBuffer, stagingBuffer, pixels.data());
    if (uploadResult.failed()) {
        vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
        stagingBuffer.destroy();
        return Error(ErrorCode::OperationFailed, "Failed to upload texture: " + uploadResult.error().toString());
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

    VkResult vkResult = vkAllocateDescriptorSets(device, &descAllocInfo, &m_testTextureDescriptorSet);
    if (vkResult != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to allocate texture descriptor set: " + std::to_string(vkResult));
    }

    // 更新纹理描述符集
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_testTexture.imageView();
    imageInfo.sampler = m_testTexture.sampler();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_testTextureDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    spdlog::info("Test texture created ({}x{} checkerboard)", texSize, texSize);
    return Result<void>::ok();
}

Result<void> VulkanRenderer::createChunkPipeline() {
    assert(m_context && m_context->device() != VK_NULL_HANDLE && "Context must be valid");
    assert(m_renderPass != VK_NULL_HANDLE && "Render pass must be valid");

    m_chunkPipeline = std::make_unique<VulkanPipeline>();

    PipelineConfig config{};

    // 着色器路径 - 使用区块着色器
    config.vertexShaderPath = "shaders/chunk.vert.spv";
    config.fragmentShaderPath = "shaders/chunk.frag.spv";

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

        // 更新描述符集
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_cameraUBO.buffer(i).buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraUBO);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_chunkDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_context->device(), 1, &descriptorWrite, 0, nullptr);
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

    // 创建纹理图集
    auto result = m_chunkTextureAtlas.create(device, physicalDevice,
                                              DefaultTextureAtlas::atlasSize(),
                                              DefaultTextureAtlas::tileSize());
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
    void* data = stagingBuffer.map().value();
    if (!data) {
        stagingBuffer.destroy();
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer");
    }
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

    // 直接调用纹理的upload方法（不需要再次复制数据）
    // VulkanTexture::upload会处理布局转换和复制
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_chunkTextureAtlas.texture().width(),
                          m_chunkTextureAtlas.texture().height(), 1};

    // 转换图像布局到传输目标
    m_chunkTextureAtlas.texture().transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲区到图像
    vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer.buffer(),
        m_chunkTextureAtlas.texture().image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    // 转换到着色器只读布局
    m_chunkTextureAtlas.texture().transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

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

    // 初始化方块模型注册表
    TextureAtlas cpuAtlas(DefaultTextureAtlas::atlasSize(), DefaultTextureAtlas::atlasSize(), DefaultTextureAtlas::tileSize());
    BlockModelRegistry::instance().initialize(cpuAtlas);

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

Result<void> VulkanRenderer::createTestChunk() {
    // 创建测试区块数据
    auto chunkData = std::make_unique<ChunkData>(0, 0);

    // 填充区块 - 创建一个简单的地形
    for (i32 x = 0; x < ChunkData::WIDTH; ++x) {
        for (i32 z = 0; z < ChunkData::WIDTH; ++z) {
            // 简单的高度图：中间高，边缘低
            i32 height = 32 + (x - 8) * (x - 8) / 8 + (z - 8) * (z - 8) / 8;
            height = std::max(1, std::min(height, 60));

            for (i32 y = 0; y < height; ++y) {
                BlockId blockId;
                if (y == 0) {
                    blockId = BlockId::Bedrock;
                } else if (y < height - 4) {
                    blockId = BlockId::Stone;
                } else if (y < height - 1) {
                    blockId = BlockId::Dirt;
                } else {
                    blockId = BlockId::GrassBlock;
                }
                chunkData->setBlock(x, y, z, BlockState(blockId));
            }

            // 添加一些矿石
            if (height > 20) {
                for (i32 y = 5; y < 20; ++y) {
                    if ((x + y + z) % 7 == 0) {
                        chunkData->setBlock(x, y, z, BlockState(BlockId::CoalOre));
                    }
                    if ((x * 2 + y + z * 3) % 13 == 0) {
                        chunkData->setBlock(x, y, z, BlockState(BlockId::IronOre));
                    }
                    if ((x + y * 2 + z * 2) % 23 == 0 && y < 15) {
                        chunkData->setBlock(x, y, z, BlockState(BlockId::DiamondOre));
                    }
                }
            }
        }
    }

    // 在中间放置一些特殊方块
    chunkData->setBlock(8, 40, 8, BlockState(BlockId::GoldOre));
    chunkData->setBlock(8, 41, 8, BlockState(BlockId::DiamondBlock));
    chunkData->setBlock(7, 40, 8, BlockState(BlockId::OakPlanks));
    chunkData->setBlock(9, 40, 8, BlockState(BlockId::Cobblestone));

    // 直接生成网格，使用正确的UV坐标
    MeshData meshData;
    generateChunkMesh(*chunkData, meshData);

    spdlog::info("Test chunk generated: {} vertices, {} indices",
                 meshData.vertices.size(), meshData.indices.size());

    if (meshData.empty()) {
        return Error(ErrorCode::InvalidState, "Generated mesh is empty");
    }

    // 上传到GPU
    ChunkId chunkId(0, 0);
    auto result = m_chunkRenderer.updateChunk(chunkId, meshData);
    if (!result.success()) {
        return Error(ErrorCode::OperationFailed, "Failed to upload chunk mesh: " + result.error().toString());
    }

    spdlog::info("Test chunk uploaded to GPU (chunks: {})", m_chunkRenderer.chunkCount());
    return Result<void>::ok();
}

void VulkanRenderer::generateChunkMesh(const ChunkData& chunk, MeshData& outMesh) {
    outMesh.clear();
    outMesh.reserve(65536, 98304);

    constexpr i32 SIZE = ChunkSection::SIZE;

    // 遍历所有区块段
    for (i32 sectionY = 0; sectionY < ChunkData::SECTIONS; ++sectionY) {
        if (!chunk.hasSection(sectionY)) continue;

        const ChunkSection* section = chunk.getSection(sectionY);
        if (!section || section->isEmpty()) continue;

        const i32 baseY = sectionY * SIZE;

        // 遍历段内所有方块
        for (i32 y = 0; y < SIZE; ++y) {
            for (i32 z = 0; z < SIZE; ++z) {
                for (i32 x = 0; x < SIZE; ++x) {
                    BlockState block = section->getBlock(x, y, z);
                    if (block.isAir()) continue;

                    // 检查每个面
                    for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
                        Face face = static_cast<Face>(faceIdx);
                        auto dir = BlockGeometry::getFaceDirection(face);

                        // 计算邻居坐标
                        i32 nx = x + dir[0];
                        i32 ny = y + dir[1];
                        i32 nz = z + dir[2];

                        BlockState neighbor;
                        if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
                            neighbor = section->getBlock(nx, ny, nz);
                        } else {
                            i32 worldY = baseY + ny;
                            if (worldY < 0 || worldY >= world::CHUNK_HEIGHT) {
                                neighbor = BlockState(BlockId::Air);
                            } else {
                                neighbor = chunk.getBlock(nx, worldY, nz);
                            }
                        }

                        // 如果邻居是空气，渲染该面
                        if (neighbor.isAir() || neighbor.isTransparent()) {
                            addBlockFace(outMesh, block.id(), face,
                                        static_cast<f32>(x),
                                        static_cast<f32>(baseY + y),
                                        static_cast<f32>(z));
                        }
                    }
                }
            }
        }
    }
}

void VulkanRenderer::addBlockFace(MeshData& mesh, BlockId blockId, Face face, f32 x, f32 y, f32 z) {
    // 获取纹理UV - 使用DefaultTextureAtlas的映射
    TextureRegion tex = getBlockTexture(blockId, face);

    auto normal = BlockGeometry::getFaceNormal(face);
    auto vertices = BlockGeometry::getFaceVertices(face);

    // 创建4个顶点
    std::array<Vertex, 4> faceVerts;

    // UV坐标根据顶点位置设置
    // 顶点顺序: 左下、右下、右上、左上
    f32 uvs[4][2] = {
        { tex.u0, tex.v1 }, // 左下
        { tex.u1, tex.v1 }, // 右下
        { tex.u1, tex.v0 }, // 右上
        { tex.u0, tex.v0 }  // 左上
    };

    // 光照值映射到0-255范围（15*17=255）
    u8 light = 15 * 17; // 全亮度

    for (size_t i = 0; i < 4; ++i) {
        faceVerts[i] = Vertex(
            x + vertices[i * 3 + 0],
            y + vertices[i * 3 + 1],
            z + vertices[i * 3 + 2],
            normal[0], normal[1], normal[2],
            uvs[i][0], uvs[i][1],
            0xFFFFFFFF,
            light
        );
    }

    // 添加顶点和索引
    u32 baseIndex = static_cast<u32>(mesh.vertices.size());
    for (const auto& v : faceVerts) {
        mesh.vertices.push_back(v);
    }

    auto indices = BlockGeometry::getFaceIndices();
    for (u32 idx : indices) {
        mesh.indices.push_back(baseIndex + idx);
    }
}

TextureRegion VulkanRenderer::getBlockTexture(BlockId blockId, Face face) {
    // 使用VulkanTextureAtlas获取正确的UV坐标
    // 纹理位置映射：
    // 0: Air (空)
    // 1: Stone (石头)
    // 2: GrassTop (草地顶部)
    // 3: GrassSide (草地侧面)
    // 4: Dirt (泥土)
    // 5: Cobblestone (圆石)
    // 6: WoodSide (木头侧面)
    // 7: WoodTop (木头顶部)
    // 8: Leaves (树叶)
    // 9: Sand (沙子)
    // 10: Gravel (沙砾)
    // 11: GoldOre (金矿)
    // 12: IronOre (铁矿)
    // 13: CoalOre (煤矿)
    // 14: DiamondOre (钻石矿)
    // 15: Bedrock (基岩)

    u32 tileIndex = 1; // 默认石头

    switch (blockId) {
        case BlockId::Air:
            return TextureRegion(0, 0, 0, 0); // 空气不渲染
        case BlockId::Stone:
            tileIndex = 1;
            break;
        case BlockId::GrassBlock:
            if (face == Face::Top) tileIndex = 2;       // 草地顶部
            else if (face == Face::Bottom) tileIndex = 4; // 泥土底部
            else tileIndex = 3;                          // 草地侧面
            break;
        case BlockId::Dirt:
            tileIndex = 4;
            break;
        case BlockId::Cobblestone:
            tileIndex = 5;
            break;
        case BlockId::OakPlanks:
            tileIndex = 6; // 使用木头侧面纹理
            break;
        case BlockId::OakLog:
            if (face == Face::Top || face == Face::Bottom) tileIndex = 7;
            else tileIndex = 6;
            break;
        case BlockId::OakLeaves:
            tileIndex = 8;
            break;
        case BlockId::Sand:
            tileIndex = 9;
            break;
        case BlockId::Gravel:
            tileIndex = 10;
            break;
        case BlockId::GoldOre:
            tileIndex = 11;
            break;
        case BlockId::IronOre:
            tileIndex = 12;
            break;
        case BlockId::CoalOre:
            tileIndex = 13;
            break;
        case BlockId::DiamondOre:
            tileIndex = 14;
            break;
        case BlockId::Bedrock:
            tileIndex = 15;
            break;
        case BlockId::DiamondBlock:
            tileIndex = 1; // 暂时用石头纹理
            break;
        default:
            tileIndex = 1;
            break;
    }

    return m_chunkTextureAtlas.getRegion(tileIndex);
}

} // namespace mr::client
