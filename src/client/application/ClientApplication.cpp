#include "ClientApplication.hpp"
#include "minecraft-reborn/version.h"

#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <chrono>

namespace mr::client {

ClientApplication::ClientApplication() = default;

ClientApplication::~ClientApplication()
{
    if (m_running) {
        stop();
    }
}

Result<void> ClientApplication::initialize(const ClientConfig& config)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Client already initialized");
    }

    m_config = config;

    // 设置日志级别
    if (m_config.logLevel == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (m_config.logLevel == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (m_config.logLevel == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (m_config.logLevel == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (m_config.logLevel == "error") {
        spdlog::set_level(spdlog::level::err);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    spdlog::info("=== Minecraft Reborn Client ===");
    spdlog::info("Version: {}.{}.{}", MR_VERSION_MAJOR, MR_VERSION_MINOR, MR_VERSION_PATCH);
    spdlog::info("Initializing client...");

    // 创建窗口
    WindowConfig windowConfig;
    windowConfig.width = m_config.windowWidth;
    windowConfig.height = m_config.windowHeight;
    windowConfig.title = m_config.windowTitle;
    windowConfig.fullscreen = m_config.fullscreen;
    windowConfig.vsync = m_config.vsync;
    windowConfig.samples = m_config.samples;

    auto windowResult = m_window.create(windowConfig);
    if (windowResult.failed()) {
        spdlog::error("Failed to create window: {}", windowResult.error().toString());
        return windowResult.error();
    }

    // 初始化输入管理器
    m_input.initialize(m_window.handle());

    // 设置按键绑定
    m_input.bindKeyAction(GLFW_KEY_ESCAPE, "exit");
    m_input.bindKeyAction(GLFW_KEY_W, "forward");
    m_input.bindKeyAction(GLFW_KEY_S, "backward");
    m_input.bindKeyAction(GLFW_KEY_A, "left");
    m_input.bindKeyAction(GLFW_KEY_D, "right");
    m_input.bindKeyAction(GLFW_KEY_SPACE, "jump");
    m_input.bindKeyAction(GLFW_KEY_LEFT_SHIFT, "sneak");

    // 设置动作回调
    m_input.bindActionCallback("exit", [this]() {
        spdlog::info("Exit key pressed");
        stop();
    });

    // 设置窗口大小变化回调
    m_window.setResizeCallback([](i32 width, i32 height, void* userData) {
        auto* app = static_cast<ClientApplication*>(userData);
        spdlog::info("Window resized: {}x{}", width, height);
        if (app && app->m_renderer) {
            auto result = app->m_renderer->onResize(static_cast<u32>(width), static_cast<u32>(height));
            if (result.failed()) {
                spdlog::error("Failed to handle resize: {}", result.error().toString());
            }
        }
    }, this);

    // 初始化Vulkan渲染器
    spdlog::info("Initializing Vulkan renderer...");
    m_renderer = std::make_unique<VulkanRenderer>();

    RendererConfig rendererConfig;
    rendererConfig.vulkanConfig.appName = m_config.windowTitle;
    rendererConfig.vulkanConfig.enableValidation = true; // Debug模式启用验证层
    rendererConfig.enableVSync = m_config.vsyncEnabled;
    rendererConfig.swapChainConfig.width = static_cast<u32>(m_config.windowWidth);
    rendererConfig.swapChainConfig.height = static_cast<u32>(m_config.windowHeight);

    auto rendererResult = m_renderer->initialize(m_window.handle(), rendererConfig);
    if (rendererResult.failed()) {
        spdlog::error("Failed to initialize renderer: {}", rendererResult.error().toString());
        m_window.destroy();
        return rendererResult.error();
    }

    spdlog::info("Client initialized successfully");
    spdlog::info("Window: {}x{}", m_window.width(), m_window.height());

    m_initialized = true;
    return Result<void>::ok();
}

Result<void> ClientApplication::run()
{
    if (!m_initialized) {
        return Error(ErrorCode::InvalidArgument, "Client not initialized");
    }

    if (m_running) {
        return Error(ErrorCode::AlreadyExists, "Client already running");
    }

    spdlog::info("Starting client main loop...");
    m_running = true;

    try {
        mainLoop();
    } catch (const std::exception& e) {
        spdlog::critical("Client crashed: {}", e.what());
        m_running = false;
        return Error(ErrorCode::Unknown, e.what());
    }

    return Result<void>::ok();
}

void ClientApplication::stop()
{
    if (!m_running) {
        return;
    }

    spdlog::info("Stopping client...");
    m_running = false;
}

void ClientApplication::mainLoop()
{
    using clock = std::chrono::steady_clock;

    spdlog::info("Client is now running!");
    spdlog::info("Press ESC to exit");

    m_lastFrameTime = glfwGetTime();

    while (m_running && !m_window.shouldClose()) {
        // 计算帧时间
        const f64 currentTime = glfwGetTime();
        const f32 deltaTime = static_cast<f32>(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;

        // 处理事件
        handleEvents();

        // 更新
        update(deltaTime);

        // 渲染
        render();

        // 帧计数
        ++m_frameCount;

        // 每秒输出一次FPS
        if (m_frameCount % 60 == 0) {
            const f32 fps = 1.0f / deltaTime;
            SPDLOG_TRACE("FPS: {:.1f}, Frame: {}", fps, m_frameCount);
        }
    }

    shutdown();
}

void ClientApplication::handleEvents()
{
    m_window.pollEvents();
    m_input.update();
}

void ClientApplication::update(f32 deltaTime)
{
    // TODO: 实现游戏逻辑更新
    // - 更新游戏状态
    // - 处理输入
    // - 更新网络
    // - 更新世界
    // - 更新实体
}

void ClientApplication::render()
{
    if (!m_renderer || m_renderer->isMinimized()) {
        return;
    }

    auto result = m_renderer->render();
    if (result.failed()) {
        spdlog::error("Render error: {}", result.error().toString());
    }
}

void ClientApplication::shutdown()
{
    spdlog::info("Shutting down client...");

    // 清理渲染器
    if (m_renderer) {
        m_renderer->destroy();
        m_renderer.reset();
    }

    // TODO: 清理资源
    // - 保存世界
    // - 断开服务器连接

    m_window.destroy();

    spdlog::info("Client stopped.");
}

// ClientConfig 实现

Result<ClientConfig> ClientConfig::load(const String& path)
{
    // TODO: 实现JSON配置加载
    ClientConfig config;
    spdlog::info("Loaded client config from: {}", path);
    return config;
}

Result<void> ClientConfig::save(const String& path) const
{
    // TODO: 实现JSON配置保存
    spdlog::info("Saved client config to: {}", path);
    return Result<void>::ok();
}

} // namespace mr::client
