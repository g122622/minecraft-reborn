#pragma once

#include "GuiRenderer.hpp"
#include "../renderer/Camera.hpp"
#include <string>
#include <chrono>

namespace mr::client {

// 前向声明
class ClientWorld;

/**
 * @brief 调试屏幕 (F3屏幕)
 *
 * 显示游戏调试信息，类似Minecraft的F3调试屏幕：
 * - FPS和帧时间
 * - 玩家位置
 * - 区块信息
 * - 系统信息
 * - 渲染统计
 *
 * 参考Minecraft的DebugOverlayGui实现
 */
class DebugScreen {
public:
    DebugScreen();
    ~DebugScreen() = default;

    /**
     * @brief 初始化调试屏幕
     * @param guiRenderer GUI渲染器
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(GuiRenderer* guiRenderer);

    /**
     * @brief 设置相机（用于获取位置信息）
     */
    void setCamera(Camera* camera) { m_camera = camera; }

    /**
     * @brief 设置世界（用于获取区块信息）
     */
    void setWorld(ClientWorld* world) { m_world = world; }

    /**
     * @brief 更新调试信息
     * @param deltaTime 帧时间（秒）
     */
    void update(f32 deltaTime);

    /**
     * @brief 渲染调试屏幕
     *
     * 注意：调用前必须已经调用了 GuiRenderer::beginFrame()
     * 此方法只负责绘制内容，不管理帧生命周期
     */
    void render();

    /**
     * @brief 切换显示状态
     */
    void toggle() { m_visible = !m_visible; }

    /**
     * @brief 设置显示状态
     */
    void setVisible(bool visible) { m_visible = visible; }

    /**
     * @brief 检查是否可见
     */
    [[nodiscard]] bool isVisible() const { return m_visible; }

    // 设置调试信息
    void setVersion(const std::string& version) { m_version = version; }
    void setRendererInfo(const std::string& info) { m_rendererInfo = info; }

private:
    /**
     * @brief 更新FPS统计
     */
    void updateFps(f32 deltaTime);

    /**
     * @brief 构建调试文本
     */
    void buildDebugText();

    GuiRenderer* m_guiRenderer = nullptr;
    Camera* m_camera = nullptr;
    ClientWorld* m_world = nullptr;

    bool m_visible = true;
    bool m_initialized = false;

    // FPS统计
    f32 m_fps = 0.0f;
    f32 m_frameTime = 0.0f;
    f32 m_fpsAccumulator = 0.0f;
    u32 m_frameCount = 0;
    f32 m_fpsUpdateTimer = 0.0f;
    static constexpr f32 FPS_UPDATE_INTERVAL = 0.5f;

    // 调试文本行
    std::vector<std::string> m_debugLines;
    std::string m_version;
    std::string m_rendererInfo;

    // 颜色常量
    static constexpr u32 COLOR_WHITE = Colors::WHITE;
    static constexpr u32 COLOR_GRAY = Colors::MC_GRAY;
    static constexpr u32 COLOR_YELLOW = Colors::MC_YELLOW;
    static constexpr u32 COLOR_GREEN = Colors::MC_GREEN;
};

} // namespace mr::client
