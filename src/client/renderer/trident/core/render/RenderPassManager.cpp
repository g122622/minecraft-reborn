#include "RenderPassManager.hpp"
#include "../TridentContext.hpp"
#include "../TridentSwapchain.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident {

// ============================================================================
// 构造/析构
// ============================================================================

RenderPassManager::RenderPassManager() = default;

RenderPassManager::~RenderPassManager() {
    destroy();
}

RenderPassManager::RenderPassManager(RenderPassManager&& other) noexcept
    : m_context(other.m_context)
    , m_swapchain(other.m_swapchain)
    , m_renderPass(other.m_renderPass)
    , m_framebuffers(std::move(other.m_framebuffers))
    , m_depthImage(other.m_depthImage)
    , m_depthImageMemory(other.m_depthImageMemory)
    , m_depthImageView(other.m_depthImageView)
    , m_depthFormat(other.m_depthFormat)
    , m_initialized(other.m_initialized)
{
    other.m_context = nullptr;
    other.m_swapchain = nullptr;
    other.m_renderPass = VK_NULL_HANDLE;
    other.m_depthImage = VK_NULL_HANDLE;
    other.m_depthImageMemory = VK_NULL_HANDLE;
    other.m_depthImageView = VK_NULL_HANDLE;
    other.m_initialized = false;
}

RenderPassManager& RenderPassManager::operator=(RenderPassManager&& other) noexcept {
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_swapchain = other.m_swapchain;
        m_renderPass = other.m_renderPass;
        m_framebuffers = std::move(other.m_framebuffers);
        m_depthImage = other.m_depthImage;
        m_depthImageMemory = other.m_depthImageMemory;
        m_depthImageView = other.m_depthImageView;
        m_depthFormat = other.m_depthFormat;
        m_initialized = other.m_initialized;

        other.m_context = nullptr;
        other.m_swapchain = nullptr;
        other.m_renderPass = VK_NULL_HANDLE;
        other.m_depthImage = VK_NULL_HANDLE;
        other.m_depthImageMemory = VK_NULL_HANDLE;
        other.m_depthImageView = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> RenderPassManager::initialize(TridentContext* context, TridentSwapchain* swapchain) {
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "RenderPassManager already initialized");
    }

    if (!context || !swapchain) {
        return Error(ErrorCode::NullPointer, "Context or swapchain is null");
    }

    m_context = context;
    m_swapchain = swapchain;

    // 创建渲染通道
    auto renderPassResult = createRenderPass();
    if (renderPassResult.failed()) {
        return renderPassResult.error();
    }

    // 创建深度缓冲区
    auto depthResult = createDepthResources();
    if (depthResult.failed()) {
        destroyRenderPass();
        return depthResult.error();
    }

    // 创建帧缓冲区
    auto framebufferResult = createFramebuffers();
    if (framebufferResult.failed()) {
        destroyDepthResources();
        destroyRenderPass();
        return framebufferResult.error();
    }

    m_initialized = true;
    spdlog::info("RenderPassManager initialized successfully");
    return {};
}

void RenderPassManager::destroy() {
    if (!m_initialized) return;

    destroyFramebuffers();
    destroyDepthResources();
    destroyRenderPass();

    m_context = nullptr;
    m_swapchain = nullptr;
    m_initialized = false;

    spdlog::info("RenderPassManager destroyed");
}

Result<void> RenderPassManager::recreate(u32 width, u32 height) {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "RenderPassManager not initialized");
    }

    m_context->waitIdle();

    destroyFramebuffers();
    destroyDepthResources();

    auto depthResult = createDepthResources();
    if (depthResult.failed()) {
        return depthResult.error();
    }

    auto framebufferResult = createFramebuffers();
    if (framebufferResult.failed()) {
        return framebufferResult.error();
    }

    return {};
}

VkFramebuffer RenderPassManager::framebuffer(u32 index) const {
    if (index >= m_framebuffers.size()) {
        return VK_NULL_HANDLE;
    }
    return m_framebuffers[index];
}

// ============================================================================
// 私有方法 - 创建
// ============================================================================

Result<void> RenderPassManager::createRenderPass() {
    // 查找深度格式
    auto depthFormatResult = m_context->findDepthFormat();
    if (depthFormatResult.failed()) {
        return depthFormatResult.error();
    }
    m_depthFormat = depthFormatResult.value();

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
    depthAttachment.format = m_depthFormat;
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

    // 子通道
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // 子通道依赖
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

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
        return Error(ErrorCode::OperationFailed, "Failed to create render pass: " + std::to_string(result));
    }

    return {};
}

Result<void> RenderPassManager::createDepthResources() {
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
    imageInfo.format = m_depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice device = m_context->device();

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &m_depthImage);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create depth image: " + std::to_string(result));
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_depthImage, &memRequirements);

    auto memoryTypeResult = m_context->findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (memoryTypeResult.failed()) {
        vkDestroyImage(device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
        return memoryTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_depthImageMemory);
    if (result != VK_SUCCESS) {
        vkDestroyImage(device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate depth image memory: " + std::to_string(result));
    }

    vkBindImageMemory(device, m_depthImage, m_depthImageMemory, 0);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device, &viewInfo, nullptr, &m_depthImageView);
    if (result != VK_SUCCESS) {
        vkFreeMemory(device, m_depthImageMemory, nullptr);
        vkDestroyImage(device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
        m_depthImageMemory = VK_NULL_HANDLE;
        return Error(ErrorCode::OperationFailed, "Failed to create depth image view: " + std::to_string(result));
    }

    return {};
}

Result<void> RenderPassManager::createFramebuffers() {
    VkExtent2D extent = m_swapchain->extent();
    const auto& imageViews = m_swapchain->imageViews();

    m_framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            imageViews[i],
            m_depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(
            m_context->device(),
            &framebufferInfo,
            nullptr,
            &m_framebuffers[i]);

        if (result != VK_SUCCESS) {
            // 清理已创建的帧缓冲区
            for (size_t j = 0; j < i; j++) {
                vkDestroyFramebuffer(m_context->device(), m_framebuffers[j], nullptr);
            }
            m_framebuffers.clear();
            return Error(ErrorCode::OperationFailed, "Failed to create framebuffer: " + std::to_string(result));
        }
    }

    return {};
}

// ============================================================================
// 私有方法 - 销毁
// ============================================================================

void RenderPassManager::destroyRenderPass() {
    if (m_renderPass != VK_NULL_HANDLE && m_context) {
        vkDestroyRenderPass(m_context->device(), m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
}

void RenderPassManager::destroyDepthResources() {
    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;

    if (m_depthImageView != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }

    if (m_depthImage != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
    }

    if (m_depthImageMemory != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_depthImageMemory, nullptr);
        m_depthImageMemory = VK_NULL_HANDLE;
    }
}

void RenderPassManager::destroyFramebuffers() {
    if (!m_context) return;

    VkDevice device = m_context->device();
    for (auto framebuffer : m_framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    m_framebuffers.clear();
}

} // namespace mc::client::renderer::trident
