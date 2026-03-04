#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "../window/Window.hpp"
#include "../input/InputManager.hpp"
#include "../renderer/VulkanRenderer.hpp"
#include "../renderer/Camera.hpp"
#include "../world/ClientWorld.hpp"

#include <string>
#include <memory>
#include <atomic>

namespace mr::client {

/**
 * @brief 客户端配置
 */
struct ClientConfig {
    // 窗口配置
    i32 windowWidth = 1280;
    i32 windowHeight = 720;
    String windowTitle = "Minecraft Reborn";
    bool fullscreen = false;
    bool vsync = true;
    i32 samples = 4;

    // 渲染配置
    i32 renderDistance = 12;
    i32 maxFps = 60;
    bool vsyncEnabled = true;

    // 网络配置
    String serverAddress = "127.0.0.1";
    u16 serverPort = 19132;
    String username = "Player";

    // 日志配置
    String logLevel = "info";

    /**
     * @brief 从JSON文件加载配置
     */
    static Result<ClientConfig> load(const String& path);

    /**
     * @brief 保存配置到JSON文件
     */
    Result<void> save(const String& path) const;
};

/**
 * @brief 客户端应用
 */
class ClientApplication {
public:
    ClientApplication();
    ~ClientApplication();

    // 禁止拷贝
    ClientApplication(const ClientApplication&) = delete;
    ClientApplication& operator=(const ClientApplication&) = delete;

    /**
     * @brief 初始化客户端
     */
    [[nodiscard]] Result<void> initialize(const ClientConfig& config);

    /**
     * @brief 运行客户端主循环
     */
    [[nodiscard]] Result<void> run();

    /**
     * @brief 停止客户端
     */
    void stop();

    /**
     * @brief 检查客户端是否正在运行
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }

    /**
     * @brief 获取窗口
     */
    [[nodiscard]] Window& window() noexcept { return m_window; }
    [[nodiscard]] const Window& window() const noexcept { return m_window; }

    /**
     * @brief 获取输入管理器
     */
    [[nodiscard]] InputManager& input() noexcept { return m_input; }
    [[nodiscard]] const InputManager& input() const noexcept { return m_input; }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const ClientConfig& config() const noexcept { return m_config; }

    /**
     * @brief 获取渲染器
     */
    [[nodiscard]] VulkanRenderer& renderer() noexcept { return *m_renderer; }
    [[nodiscard]] const VulkanRenderer& renderer() const noexcept { return *m_renderer; }

    /**
     * @brief 获取相机
     */
    [[nodiscard]] Camera& camera() noexcept { return m_camera; }
    [[nodiscard]] const Camera& camera() const noexcept { return m_camera; }

    /**
     * @brief 获取世界
     */
    [[nodiscard]] ClientWorld& world() noexcept { return m_world; }
    [[nodiscard]] const ClientWorld& world() const noexcept { return m_world; }

    // 友元声明，用于回调
    friend void onWindowResize(i32 width, i32 height, void* userData);

private:
    void mainLoop();
    void handleEvents();
    void update(f32 deltaTime);
    void render();
    void shutdown();

    // 初始化辅助函数
    void setupInputBindings();
    void setupCamera();
    void toggleMouseCapture();

    ClientConfig m_config;
    Window m_window;
    InputManager m_input;
    std::unique_ptr<VulkanRenderer> m_renderer;

    // 相机
    Camera m_camera;
    CameraController m_cameraController;
    bool m_mouseCaptured = false;

    // 世界
    ClientWorld m_world;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};

    f64 m_lastFrameTime = 0.0;
    u64 m_frameCount = 0;
};

} // namespace mr::client
