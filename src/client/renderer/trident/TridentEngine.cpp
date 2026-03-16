#include <vulkan/vulkan.h>
#include "TridentEngine.hpp"
#include "TridentContext.hpp"
#include "TridentSwapchain.hpp"
#include "render/RenderPassManager.hpp"
#include "render/FrameManager.hpp"
#include "render/DescriptorManager.hpp"
#include "render/UniformManager.hpp"
#include "buffer/TridentBuffer.hpp"
#include "texture/TridentTexture.hpp"
#include "../api/IRenderEngine.hpp"
#include "../ChunkRenderer.hpp"
#include "../sky/SkyRenderer.hpp"
#include "../../ui/GuiRenderer.hpp"
#include "../../ui/Font.hpp"
#include "../item/ItemRenderer.hpp"
#include "../../resource/ItemTextureAtlas.hpp"
#include "../entity/EntityRendererManager.hpp"
#include "../entity/EntityTextureAtlas.hpp"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <array>

namespace mc::client::renderer::trident {

// ============================================================================
// 构造/析构
// ============================================================================

TridentEngine::TridentEngine() = default;

TridentEngine::~TridentEngine() {
    destroy();
}

// ============================================================================
// IRenderEngine 接口实现 - 生命周期
// ============================================================================

Result<void> TridentEngine::initialize(void* window, const api::RenderEngineConfig& config) {
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "TridentEngine already initialized");
    }

    if (!window) {
        return Error(ErrorCode::NullPointer, "Window pointer is null");
    }

    m_config = config;
    m_windowWidth = config.initialWindowWidth;
    m_windowHeight = config.initialWindowHeight;

    // 转换配置
    m_tridentConfig.appName = config.appName;
    m_tridentConfig.enableValidation = config.enableValidation;
    m_tridentConfig.enableVSync = config.enableVSync;
    m_tridentConfig.maxFramesInFlight = config.maxFramesInFlight;

    // 1. 创建 Vulkan 上下文
    m_context = std::make_unique<TridentContext>();
    auto contextResult = m_context->initialize(
        static_cast<GLFWwindow*>(window),
        m_tridentConfig
    );
    if (contextResult.failed()) {
        m_context.reset();
        return contextResult.error();
    }

    // 2. 创建交换链
    m_swapchain = std::make_unique<TridentSwapchain>();
    SwapChainConfig swapchainConfig{};
    swapchainConfig.width = config.initialWindowWidth;
    swapchainConfig.height = config.initialWindowHeight;
    swapchainConfig.vsync = config.enableVSync;

    auto swapchainResult = m_swapchain->initialize(m_context.get(), swapchainConfig);
    if (swapchainResult.failed()) {
        m_context->destroy();
        m_context.reset();
        m_swapchain.reset();
        return swapchainResult.error();
    }

    // 3. 创建渲染通道管理器
    m_renderPassManager = std::make_unique<RenderPassManager>();
    auto renderPassResult = m_renderPassManager->initialize(m_context.get(), m_swapchain.get());
    if (renderPassResult.failed()) {
        m_swapchain->destroy();
        m_context->destroy();
        m_swapchain.reset();
        m_context.reset();
        m_renderPassManager.reset();
        return renderPassResult.error();
    }

    // 4. 创建帧管理器
    m_frameManager = std::make_unique<FrameManager>();
    auto frameResult = m_frameManager->initialize(m_context.get(), config.maxFramesInFlight);
    if (frameResult.failed()) {
        m_renderPassManager->destroy();
        m_swapchain->destroy();
        m_context->destroy();
        m_renderPassManager.reset();
        m_swapchain.reset();
        m_context.reset();
        m_frameManager.reset();
        return frameResult.error();
    }

    // 5. 创建描述符管理器
    m_descriptorManager = std::make_unique<DescriptorManager>();
    auto descriptorResult = m_descriptorManager->initialize(m_context.get(), config.maxFramesInFlight);
    if (descriptorResult.failed()) {
        m_frameManager->destroy();
        m_renderPassManager->destroy();
        m_swapchain->destroy();
        m_context->destroy();
        m_frameManager.reset();
        m_renderPassManager.reset();
        m_swapchain.reset();
        m_context.reset();
        m_descriptorManager.reset();
        return descriptorResult.error();
    }

    // 6. 创建 Uniform 管理器
    m_uniformManager = std::make_unique<UniformManager>();
    auto uniformResult = m_uniformManager->initialize(
        m_context.get(),
        m_descriptorManager.get(),
        config.maxFramesInFlight
    );
    if (uniformResult.failed()) {
        m_descriptorManager->destroy();
        m_frameManager->destroy();
        m_renderPassManager->destroy();
        m_swapchain->destroy();
        m_context->destroy();
        m_descriptorManager.reset();
        m_frameManager.reset();
        m_renderPassManager.reset();
        m_swapchain.reset();
        m_context.reset();
        m_uniformManager.reset();
        return uniformResult.error();
    }

    // 初始化帧上下文
    m_frameContext = api::FrameContext{};

    m_initialized = true;
    spdlog::info("TridentEngine initialized successfully");
    return {};
}

void TridentEngine::destroy() {
    if (!m_initialized) return;

    // 等待设备空闲
    if (m_context) {
        m_context->waitIdle();
    }

    // 先销毁依赖 Vulkan 设备的子渲染器与纹理资源
    if (m_chunkRenderer) {
        m_chunkRenderer->destroy();
        m_chunkRenderer.reset();
    }

    if (m_skyRenderer) {
        m_skyRenderer->destroy();
        m_skyRenderer.reset();
    }

    if (m_guiRendererPtr) {
        m_guiRendererPtr->destroy();
        m_guiRendererPtr.reset();
    }

    m_itemRenderer.reset();
    m_entityRendererManager.reset();
    m_font.reset();

    m_itemTextureAtlas.destroy();
    m_entityTextureAtlas.destroy();
    m_textureRegions.clear();

    m_chunkRendererInitialized = false;
    m_skyRendererInitialized = false;
    m_guiRendererInitialized = false;
    m_itemRendererInitialized = false;
    m_itemTextureAtlasInitialized = false;
    m_entityRendererInitialized = false;
    m_entityTextureAtlasInitialized = false;

    m_guiRenderCallback = nullptr;
    m_entityRenderCallback = nullptr;

    // 按相反顺序销毁
    m_uniformManager.reset();
    m_descriptorManager.reset();
    m_frameManager.reset();
    m_renderPassManager.reset();
    m_swapchain.reset();

    if (m_context) {
        m_context->destroy();
        m_context.reset();
    }

    m_initialized = false;
    spdlog::info("TridentEngine destroyed");
}

// ============================================================================
// IRenderEngine 接口实现 - 帧渲染
// ============================================================================

Result<void> TridentEngine::beginFrame() {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    // 检查窗口是否最小化
    if (m_minimized) {
        return Error(ErrorCode::InvalidState, "Window is minimized");
    }

    // 获取下一帧图像
    auto imageResult = m_frameManager->acquireNextImage(m_swapchain.get());
    if (imageResult.failed()) {
        // 需要重建交换链
        if (imageResult.error().code() == ErrorCode::InvalidState) {
            return recreateSwapchain();
        }
        return imageResult.error();
    }

    m_frameContext.imageIndex = imageResult.value();
    m_frameContext.frameIndex = m_frameManager->currentFrameIndex();

    // 开始帧录制
    m_frameManager->beginFrame();

    // 开始渲染通道
    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "Command buffer is null");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPassManager->renderPass();
    renderPassInfo.framebuffer = m_renderPassManager->framebuffer(m_frameContext.imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain->extent();

    // 清除值
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.1f, 0.1f, 0.2f, 1.0f}};  // 天空蓝
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 设置视口和裁剪
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(m_swapchain->extent().width);
    viewport.height = static_cast<f32>(m_swapchain->extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain->extent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // 绑定管线布局
    VkDescriptorSet cameraSet = m_uniformManager->cameraDescriptorSet(m_frameContext.frameIndex);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_descriptorManager->pipelineLayout(),
        0,  // first set
        1,  // set count
        &cameraSet,
        0,
        nullptr
    );

    m_frameStarted = true;
    return {};
}

Result<void> TridentEngine::endFrame() {
    if (!m_frameStarted) {
        return Error(ErrorCode::InvalidState, "Frame not started");
    }

    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    vkCmdEndRenderPass(cmd);

    m_frameManager->endFrame();
    m_frameStarted = false;

    return {};
}

Result<void> TridentEngine::present() {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    auto result = m_frameManager->submitAndPresent(m_swapchain.get());
    if (result.failed()) {
        // 需要重建交换链
        if (result.error().code() == ErrorCode::InvalidState) {
            return recreateSwapchain();
        }
        return result.error();
    }

    return {};
}

Result<void> TridentEngine::render() {
    // 1. 开始帧
    auto beginResult = beginFrame();
    if (beginResult.failed()) {
        return beginResult.error();
    }

    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "Command buffer is null");
    }

    // 2. 调用实体渲染回调（在区块渲染之前）
    if (m_entityRenderCallback) {
        m_entityRenderCallback(cmd, m_frameContext.deltaTime);
    }

    // 3. 调用 GUI 渲染回调
    if (m_guiRenderCallback) {
        m_guiRenderCallback();
    }

    // 4. 结束帧
    auto endResult = endFrame();
    if (endResult.failed()) {
        return endResult.error();
    }

    // 5. 呈现
    return present();
}

// ============================================================================
// IRenderEngine 接口实现 - 窗口和相机
// ============================================================================

Result<void> TridentEngine::onResize(u32 width, u32 height) {
    if (!m_initialized) return {};

    if (width == 0 || height == 0) {
        m_minimized = true;
        return {};
    }

    m_minimized = false;
    m_windowWidth = width;
    m_windowHeight = height;

    return recreateSwapchain();
}

void TridentEngine::setCamera(const api::ICamera* camera) {
    m_frameContext.camera = camera;

    if (camera) {
        m_frameContext.viewMatrix = camera->viewMatrix();
        m_frameContext.projectionMatrix = camera->projectionMatrix();
        m_frameContext.viewProjectionMatrix = m_frameContext.projectionMatrix * m_frameContext.viewMatrix;

        // 更新 Uniform 缓冲区
        m_uniformManager->updateCamera(
            m_frameContext.viewMatrix,
            m_frameContext.projectionMatrix,
            m_frameContext.frameIndex
        );
    }
}

// ============================================================================
// IRenderEngine 接口实现 - 资源创建
// ============================================================================

Result<std::unique_ptr<api::IVertexBuffer>> TridentEngine::createVertexBuffer(u64 size, u32 vertexStride) {
    auto buffer = std::make_unique<TridentVertexBuffer>();
    auto result = buffer->create(m_context.get(), size, vertexStride);
    if (result.failed()) {
        return result.error();
    }
    return buffer;
}

Result<std::unique_ptr<api::IIndexBuffer>> TridentEngine::createIndexBuffer(u64 size, api::IndexType type) {
    auto buffer = std::make_unique<TridentIndexBuffer>();
    auto result = buffer->create(m_context.get(), size, type);
    if (result.failed()) {
        return result.error();
    }
    return buffer;
}

Result<std::unique_ptr<api::IUniformBuffer>> TridentEngine::createUniformBuffer(u64 size, u32 frameCount) {
    auto buffer = std::make_unique<TridentUniformBuffer>();
    auto result = buffer->create(m_context.get(), size, frameCount);
    if (result.failed()) {
        return result.error();
    }
    return buffer;
}

Result<std::unique_ptr<api::ITexture>> TridentEngine::createTexture(const api::TextureDesc& desc) {
    auto texture = std::make_unique<TridentTexture>();
    auto result = texture->create(m_context.get(), desc);
    if (result.failed()) {
        return result.error();
    }
    return texture;
}

Result<std::unique_ptr<api::ITextureAtlas>> TridentEngine::createTextureAtlas(u32 width, u32 height, u32 tileSize) {
    // TODO: 实现 ITextureAtlas 接口
    return Error(ErrorCode::Unsupported, "createTextureAtlas not yet implemented");
}

// ============================================================================
// IRenderEngine 接口实现 - 渲染状态
// ============================================================================

void TridentEngine::setRenderType(const api::RenderType& type) {
    m_currentRenderType = type;
    // TODO: 实际绑定对应的管线
}

const api::RenderType& TridentEngine::currentRenderType() const {
    return m_currentRenderType;
}

void TridentEngine::bindTexture(u32 binding, const api::ITexture* texture) {
    // TODO: 实现纹理绑定
}

void TridentEngine::bindUniformBuffer(u32 binding, const api::IUniformBuffer* buffer) {
    // TODO: 实现 Uniform 缓冲区绑定
}

// ============================================================================
// IRenderEngine 接口实现 - 绘制
// ============================================================================

void TridentEngine::drawIndexed(u32 indexCount, u32 firstIndex, i32 vertexOffset) {
    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) return;

    vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, vertexOffset, 0);
}

void TridentEngine::draw(u32 vertexCount, u32 firstVertex) {
    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) return;

    vkCmdDraw(cmd, vertexCount, 1, firstVertex, 0);
}

void TridentEngine::drawIndexedInstanced(u32 indexCount, u32 instanceCount,
                                          u32 firstIndex, i32 vertexOffset, u32 firstInstance) {
    // TODO: 实现实例化渲染
    // 暂时使用循环绘制代替
    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) return;

    // 简化实现：使用多次绘制代替实例化
    // 实际实现应该使用 vkCmdDrawIndexedInstanced 或间接绘制
    for (u32 i = 0; i < instanceCount; ++i) {
        vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, vertexOffset, firstInstance + i);
    }
}

// ============================================================================
// IRenderEngine 接口实现 - 状态查询
// ============================================================================

bool TridentEngine::isInitialized() const {
    return m_initialized;
}

u32 TridentEngine::currentFrameIndex() const {
    return m_frameManager ? m_frameManager->currentFrameIndex() : 0;
}

u32 TridentEngine::currentImageIndex() const {
    return m_frameManager ? m_frameManager->currentImageIndex() : 0;
}

const api::FrameContext& TridentEngine::frameContext() const {
    return m_frameContext;
}

u32 TridentEngine::maxFramesInFlight() const {
    return m_tridentConfig.maxFramesInFlight;
}

bool TridentEngine::isMinimized() const {
    return m_minimized;
}

u32 TridentEngine::windowWidth() const {
    return m_windowWidth;
}

u32 TridentEngine::windowHeight() const {
    return m_windowHeight;
}

const api::ICamera* TridentEngine::camera() const {
    return m_frameContext.camera;
}

// ============================================================================
// Trident 特有接口
// ============================================================================

VkRenderPass TridentEngine::renderPass() const {
    return m_renderPassManager ? m_renderPassManager->renderPass() : VK_NULL_HANDLE;
}

VkCommandBuffer TridentEngine::currentCommandBuffer() const {
    return m_frameManager ? m_frameManager->currentCommandBuffer() : VK_NULL_HANDLE;
}

VkPipelineLayout TridentEngine::pipelineLayout() const {
    return m_descriptorManager ? m_descriptorManager->pipelineLayout() : VK_NULL_HANDLE;
}

VkDescriptorPool TridentEngine::descriptorPool() const {
    return m_descriptorManager ? m_descriptorManager->pool() : VK_NULL_HANDLE;
}

VkDescriptorSetLayout TridentEngine::cameraDescriptorLayout() const {
    return m_descriptorManager ? m_descriptorManager->cameraLayout() : VK_NULL_HANDLE;
}

VkDescriptorSetLayout TridentEngine::textureDescriptorLayout() const {
    return m_descriptorManager ? m_descriptorManager->textureLayout() : VK_NULL_HANDLE;
}

VkDescriptorSet TridentEngine::cameraDescriptorSet() const {
    if (!m_uniformManager || !m_frameManager) return VK_NULL_HANDLE;
    return m_uniformManager->cameraDescriptorSet(m_frameManager->currentFrameIndex());
}

void TridentEngine::updateTime(i64 dayTime, i64 gameTime, f32 partialTick) {
    if (m_uniformManager) {
        m_uniformManager->updateLighting(dayTime, gameTime, partialTick);
    }
}

VkCommandPool TridentEngine::commandPool() const {
    return m_frameManager ? m_frameManager->commandPool() : VK_NULL_HANDLE;
}

VkCommandBuffer TridentEngine::beginSingleTimeCommands() const {
    return m_context ? m_context->beginSingleTimeCommands() : VK_NULL_HANDLE;
}

void TridentEngine::endSingleTimeCommands(VkCommandBuffer cmd) const {
    if (m_context && cmd != VK_NULL_HANDLE) {
        m_context->endSingleTimeCommands(cmd);
    }
}

// ============================================================================
// 兼容性接口
// ============================================================================

VkDevice TridentEngine::device() const {
    return m_context ? m_context->device() : VK_NULL_HANDLE;
}

VkPhysicalDevice TridentEngine::physicalDevice() const {
    return m_context ? m_context->physicalDevice() : VK_NULL_HANDLE;
}

VkQueue TridentEngine::graphicsQueue() const {
    return m_context ? m_context->graphicsQueue() : VK_NULL_HANDLE;
}

VkImageView TridentEngine::swapchainImageView(u32 index) const {
    return m_swapchain ? m_swapchain->imageView(index) : VK_NULL_HANDLE;
}

u32 TridentEngine::swapchainImageCount() const {
    return m_swapchain ? m_swapchain->imageCount() : 0;
}

VkFormat TridentEngine::swapchainFormat() const {
    return m_swapchain ? m_swapchain->format() : VK_FORMAT_UNDEFINED;
}

VkExtent2D TridentEngine::swapchainExtent() const {
    return m_swapchain ? m_swapchain->extent() : VkExtent2D{0, 0};
}

VkImageView TridentEngine::depthImageView() const {
    return m_renderPassManager ? m_renderPassManager->depthImageView() : VK_NULL_HANDLE;
}

// ============================================================================
// 渲染回调
// ============================================================================

void TridentEngine::setGuiRenderCallback(GuiRenderCallback callback) {
    m_guiRenderCallback = std::move(callback);
}

void TridentEngine::setEntityRenderCallback(EntityRenderCallback callback) {
    m_entityRenderCallback = std::move(callback);
}

// ============================================================================
// 私有方法
// ============================================================================

Result<void> TridentEngine::recreateSwapchain() {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    m_context->waitIdle();

    // 重建交换链
    auto swapchainResult = m_swapchain->recreate(m_windowWidth, m_windowHeight);
    if (swapchainResult.failed()) {
        return swapchainResult.error();
    }

    // 重建渲染通道资源
    auto renderPassResult = m_renderPassManager->recreate(m_windowWidth, m_windowHeight);
    if (renderPassResult.failed()) {
        return renderPassResult.error();
    }

    // 重建天空渲染器
    if (m_skyRendererInitialized && m_skyRenderer) {
        auto skyResult = m_skyRenderer->onResize(VkExtent2D{m_windowWidth, m_windowHeight});
        if (skyResult.failed()) {
            spdlog::warn("Failed to recreate sky renderer: {}", skyResult.error().toString());
        }
    }

    return {};
}

// ============================================================================
// 子渲染器访问器
// ============================================================================

ChunkRenderer& TridentEngine::chunkRenderer() {
    if (!m_chunkRenderer) {
        m_chunkRenderer = std::make_unique<ChunkRenderer>();
    }
    return *m_chunkRenderer;
}

const ChunkRenderer& TridentEngine::chunkRenderer() const {
    return *m_chunkRenderer;
}

SkyRenderer& TridentEngine::skyRenderer() {
    if (!m_skyRenderer) {
        m_skyRenderer = std::make_unique<SkyRenderer>();
    }
    return *m_skyRenderer;
}

const SkyRenderer& TridentEngine::skyRenderer() const {
    return *m_skyRenderer;
}

GuiRenderer& TridentEngine::guiRenderer() {
    if (!m_guiRendererPtr) {
        m_guiRendererPtr = std::make_unique<GuiRenderer>();
    }
    return *m_guiRendererPtr;
}

const GuiRenderer& TridentEngine::guiRenderer() const {
    return *m_guiRendererPtr;
}

Font& TridentEngine::font() {
    if (!m_font) {
        m_font = std::make_unique<Font>();
    }
    return *m_font;
}

const Font& TridentEngine::font() const {
    return *m_font;
}

ItemRenderer& TridentEngine::itemRenderer() {
    if (!m_itemRenderer) {
        m_itemRenderer = std::make_unique<ItemRenderer>();
    }
    return *m_itemRenderer;
}

const ItemRenderer& TridentEngine::itemRenderer() const {
    return *m_itemRenderer;
}

ItemTextureAtlas& TridentEngine::itemTextureAtlas() {
    return m_itemTextureAtlas;
}

const ItemTextureAtlas& TridentEngine::itemTextureAtlas() const {
    return m_itemTextureAtlas;
}

renderer::EntityRendererManager& TridentEngine::entityRendererManager() {
    if (!m_entityRendererManager) {
        m_entityRendererManager = std::make_unique<renderer::EntityRendererManager>();
    }
    return *m_entityRendererManager;
}

const renderer::EntityRendererManager& TridentEngine::entityRendererManager() const {
    return *m_entityRendererManager;
}

EntityTextureAtlas& TridentEngine::entityTextureAtlas() {
    return m_entityTextureAtlas;
}

const EntityTextureAtlas& TridentEngine::entityTextureAtlas() const {
    return m_entityTextureAtlas;
}

// ============================================================================
// 子渲染器初始化
// ============================================================================

Result<void> TridentEngine::initializeChunkRenderer() {
    if (m_chunkRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing chunk renderer...");

    if (!m_chunkRenderer) {
        m_chunkRenderer = std::make_unique<ChunkRenderer>();
    }

    auto result = m_chunkRenderer->initialize(
        device(),
        physicalDevice(),
        commandPool(),
        graphicsQueue(),
        1024  // max chunks
    );

    if (result.failed()) {
        m_chunkRenderer.reset();
        return result.error();
    }

    m_chunkRendererInitialized = true;
    spdlog::info("Chunk renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeSkyRenderer() {
    if (m_skyRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing sky renderer...");

    if (!m_skyRenderer) {
        m_skyRenderer = std::make_unique<SkyRenderer>();
    }

    auto result = m_skyRenderer->initialize(
        device(),
        physicalDevice(),
        commandPool(),
        graphicsQueue(),
        renderPass(),
        swapchainExtent()
    );

    if (result.failed()) {
        m_skyRenderer.reset();
        return result.error();
    }

    m_skyRendererInitialized = true;
    spdlog::info("Sky renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeGuiRenderer() {
    if (m_guiRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing GUI renderer...");

    if (!m_guiRendererPtr) {
        m_guiRendererPtr = std::make_unique<GuiRenderer>();
    }

    auto result = m_guiRendererPtr->initialize(
        device(),
        physicalDevice(),
        commandPool(),
        renderPass()
    );

    if (result.failed()) {
        m_guiRendererPtr.reset();
        return result.error();
    }

    m_guiRendererInitialized = true;
    spdlog::info("GUI renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeItemRenderer(ResourceManager* resourceManager) {
    if (m_itemRendererInitialized) {
        return {};
    }

    if (!resourceManager) {
        return Error(ErrorCode::NullPointer, "ResourceManager is null");
    }

    spdlog::info("Initializing item renderer...");

    // 创建物品渲染器
    m_itemRenderer = std::make_unique<ItemRenderer>();
    auto result = m_itemRenderer->initialize(resourceManager, &m_itemTextureAtlas);
    if (result.failed()) {
        m_itemRenderer.reset();
        return result.error();
    }

    m_itemRendererInitialized = true;
    spdlog::info("Item renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeEntityRenderer() {
    if (m_entityRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing entity renderer...");

    // 创建实体渲染器管理器
    if (!m_entityRendererManager) {
        m_entityRendererManager = std::make_unique<renderer::EntityRendererManager>();
    }

    // 初始化默认实体渲染器
    m_entityRendererManager->initializeDefaults();

    m_entityRendererInitialized = true;
    spdlog::info("Entity renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeEntityTextureAtlas(ResourceManager* resourceManager) {
    if (m_entityTextureAtlasInitialized) {
        return {};
    }

    if (!resourceManager) {
        return Error(ErrorCode::NullPointer, "ResourceManager is null");
    }

    spdlog::info("Initializing entity texture atlas...");

    // 初始化实体纹理图集
    auto initResult = m_entityTextureAtlas.initialize(
        device(),
        physicalDevice(),
        commandPool(),
        graphicsQueue()
    );
    if (initResult.failed()) {
        return initResult.error();
    }

    m_entityTextureAtlasInitialized = true;
    spdlog::info("Entity texture atlas initialized");
    return {};
}

Result<void> TridentEngine::updateTextureAtlas(const AtlasBuildResult& atlasResult) {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    if (atlasResult.pixels.empty()) {
        return Error(ErrorCode::InvalidArgument, "Atlas result has no pixel data");
    }

    spdlog::info("Updating texture atlas: {}x{}, {} regions",
                 atlasResult.width, atlasResult.height, atlasResult.regions.size());

    // 保存纹理区域映射
    m_textureRegions = atlasResult.regions;

    // 初始化区块渲染器（如果尚未初始化）
    if (!m_chunkRendererInitialized) {
        auto chunkResult = initializeChunkRenderer();
        if (chunkResult.failed()) {
            spdlog::warn("Failed to initialize chunk renderer: {}", chunkResult.error().toString());
            return chunkResult.error();
        }
    }

    // 加载纹理图集到区块渲染器
    auto loadResult = m_chunkRenderer->loadTextureAtlas(
        atlasResult.pixels.data(),
        atlasResult.width,
        atlasResult.height,
        16  // tileSize
    );
    if (loadResult.failed()) {
        spdlog::error("Failed to load texture atlas to chunk renderer: {}", loadResult.error().toString());
        return loadResult.error();
    }

    // 更新实体管线的纹理（如果已初始化）
    if (m_entityRendererInitialized && m_entityRendererManager) {
        // 实体渲染器可以从 ChunkRenderer 的纹理图集获取纹理
    }

    spdlog::info("Texture atlas updated successfully");
    return {};
}

const TextureRegion* TridentEngine::getTextureRegion(const ResourceLocation& location) const {
    auto it = m_textureRegions.find(location);
    if (it != m_textureRegions.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// 工厂函数实现
// ============================================================================

} // namespace mc::client::renderer::trident

namespace mc::client::renderer::api {

std::unique_ptr<IRenderEngine> createRenderEngine(RenderBackend backend) {
    switch (backend) {
        case RenderBackend::Vulkan:
            return std::make_unique<trident::TridentEngine>();
        default:
            return nullptr;
    }
}

} // namespace mc::client::renderer::api
