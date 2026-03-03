#include "VulkanRenderer.hpp"
#include <spdlog/spdlog.h>
#include <array>

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

    // 创建Surface
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(
        m_context->instance() ? m_context->instance() : VkInstance{nullptr},
        window,
        nullptr,
        &surface);

    // 初始化上下文
    auto contextResult = m_context->initialize(config.vulkanConfig, surface);
    if (contextResult.failed()) {
        return contextResult.error();
    }

    // 创建Surface (在上下文初始化后)
    result = glfwCreateWindowSurface(m_context->instance(), window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create window surface: " + std::to_string(result));
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

    // 创建同步对象
    auto syncResult = createSyncObjects();
    if (syncResult.failed()) {
        return syncResult.error();
    }

    m_initialized = true;
    spdlog::info("Vulkan renderer initialized successfully");
    return Result<void>::ok();
}

void VulkanRenderer::destroy() {
    if (!m_initialized) {
        return;
    }

    m_context->waitIdle();

    destroySyncObjects();
    destroyFramebuffers();
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

    // 等待上一帧完成
    vkWaitForFences(m_context->device(), 1, &m_frameSyncs[m_currentFrame].inFlightFence,
                    VK_TRUE, UINT64_MAX);

    // 获取下一帧图像
    auto acquireResult = m_swapchain->acquireNextImage(
        m_frameSyncs[m_currentFrame].imageAvailableSemaphore);

    if (acquireResult.failed()) {
        // 交换链需要重建
        if (acquireResult.error().code() == ErrorCode::InvalidState) {
            auto recreateResult = recreateSwapchain();
            if (recreateResult.failed()) {
                return recreateResult.error();
            }
            return beginFrame(); // 重试
        }
        return acquireResult.error();
    }

    m_imageIndex = acquireResult.value();

    // 重置栅栏
    vkResetFences(m_context->device(), 1, &m_frameSyncs[m_currentFrame].inFlightFence);

    // 开始命令缓冲区
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
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

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.2f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

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

    VkSemaphore waitSemaphores[] = {m_frameSyncs[m_currentFrame].imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = {m_frameSyncs[m_currentFrame].renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo,
                           m_frameSyncs[m_currentFrame].inFlightFence);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to submit draw command buffer: " + std::to_string(result));
    }

    // 呈现
    auto presentResult = m_swapchain->present(m_imageIndex,
                                               m_frameSyncs[m_currentFrame].renderFinishedSemaphore);

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

    // 下一帧
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    m_frameStarted = false;

    return Result<void>::ok();
}

Result<void> VulkanRenderer::render() {
    auto beginResult = beginFrame();
    if (beginResult.failed()) {
        return beginResult.error();
    }

    // TODO: 在这里添加实际渲染代码

    return endFrame();
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

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(m_context->device(), &renderPassInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create render pass: " + std::to_string(result));
    }

    spdlog::info("Render pass created");
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
        VkImageView attachments[] = {m_swapchain->imageViews()[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
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
    m_frameSyncs.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkResult result = vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr,
                                            &m_frameSyncs[i].imageAvailableSemaphore);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to create semaphore: " + std::to_string(result));
        }

        result = vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr,
                                   &m_frameSyncs[i].renderFinishedSemaphore);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to create semaphore: " + std::to_string(result));
        }

        result = vkCreateFence(m_context->device(), &fenceInfo, nullptr,
                               &m_frameSyncs[i].inFlightFence);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::Unknown, "Failed to create fence: " + std::to_string(result));
        }
    }

    spdlog::info("Sync objects created");
    return Result<void>::ok();
}

void VulkanRenderer::destroyRenderPass() {
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_context->device(), m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
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
    for (auto& sync : m_frameSyncs) {
        if (sync.imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_context->device(), sync.imageAvailableSemaphore, nullptr);
        }
        if (sync.renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_context->device(), sync.renderFinishedSemaphore, nullptr);
        }
        if (sync.inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(m_context->device(), sync.inFlightFence, nullptr);
        }
    }
    m_frameSyncs.clear();
}

Result<void> VulkanRenderer::recreateSwapchain() {
    m_context->waitIdle();

    destroyFramebuffers();
    destroyCommandBuffers();
    destroyRenderPass();

    auto result = m_swapchain->recreate(
        m_swapchain->extent().width,
        m_swapchain->extent().height);

    if (result.failed()) {
        return result;
    }

    auto renderPassResult = createRenderPass();
    if (renderPassResult.failed()) {
        return renderPassResult.error();
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

} // namespace mr::client
