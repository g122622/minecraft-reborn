#pragma once

#include "GuiRenderer.hpp"
#include "../renderer/Camera.hpp"
#include "../../common/core/BlockRaycastResult.hpp"
#include "../../common/world/time/GameTime.hpp"
#include <string>
#include <chrono>
#include <vector>

namespace mc {

// 前向声明
class Entity;

namespace client {

// 前向声明
class ClientWorld;
class ClientEntityManager;
class NetworkClient;

/**
 * @brief 调试屏幕 (F3屏幕)
 *
 * 显示游戏调试信息，高度还原Minecraft 1.16.5的F3调试屏幕：
 * - 左侧面板：版本、FPS、位置、区块、光照、生物群系等
 * - 右侧面板：Java、内存、CPU、GPU、目标方块/流体/实体
 *
 * 参考 MC 1.16.5 DebugOverlayGui
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
     * @brief 设置目标方块（射线检测结果）
     * @param result 射线检测结果指针（可为nullptr）
     */
    void setTargetBlock(const BlockRaycastResult* result) { m_targetBlock = result; }

    /**
     * @brief 设置实体管理器（用于实体计数）
     */
    void setEntityManager(ClientEntityManager* manager) { m_entityManager = manager; }

    /**
     * @brief 设置网络客户端（用于网络统计）
     */
    void setNetworkClient(NetworkClient* client) { m_networkClient = client; }

    /**
     * @brief 设置游戏时间
     */
    void setGameTime(const time::GameTime* gameTime) { m_gameTime = gameTime; }

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

    /**
     * @brief 设置玩家实体（用于调试信息）
     */
    void setPlayerEntity(Entity* entity) { m_playerEntity = entity; }

private:
    /**
     * @brief 更新FPS统计
     */
    void updateFps(f32 deltaTime);

    /**
     * @brief 构建左侧调试文本
     */
    void buildLeftDebugText();

    /**
     * @brief 构建右侧调试文本
     */
    void buildRightDebugText();

    /**
     * @brief 获取方向名称
     * @param yaw 偏航角（度）
     * @return 方向名称和描述
     */
    std::pair<std::string, std::string> getFacingDirection(f32 yaw) const;

    /**
     * @brief 获取方向名称（中文）
     */
    std::string getDirectionName(f32 yaw) const;

    /**
     * @brief 格式化内存大小
     */
    static std::string formatMemory(i64 bytes);

    /**
     * @brief 渲染面板
     * @param lines 文本行
     * @param startX X起始位置
     * @param alignRight 是否右对齐
     */
    void renderPanel(const std::vector<std::string>& lines, f32 startX, bool alignRight);

    /**
     * @brief 输出玩家当前位置的柱状方块信息（每秒一次）
     */
    void logColumnBlocks();

    GuiRenderer* m_guiRenderer = nullptr;
    Camera* m_camera = nullptr;
    ClientWorld* m_world = nullptr;
    const BlockRaycastResult* m_targetBlock = nullptr;
    ClientEntityManager* m_entityManager = nullptr;
    NetworkClient* m_networkClient = nullptr;
    const time::GameTime* m_gameTime = nullptr;
    Entity* m_playerEntity = nullptr;

    bool m_visible = true;
    bool m_initialized = false;

    // FPS统计
    f32 m_fps = 0.0f;
    f32 m_frameTime = 0.0f;
    f32 m_fpsAccumulator = 0.0f;
    u32 m_frameCount = 0;
    f32 m_fpsUpdateTimer = 0.0f;
    static constexpr f32 FPS_UPDATE_INTERVAL = 0.5f;

    // 柱状方块输出计时器（每秒输出一次）
    f32 m_columnLogTimer = 0.0f;
    static constexpr f32 COLUMN_LOG_INTERVAL = 1.0f;

    // 调试文本行
    std::vector<std::string> m_leftLines;
    std::vector<std::string> m_rightLines;
    std::string m_version;
    std::string m_rendererInfo;

    // 颜色常量
    static constexpr u32 COLOR_WHITE = Colors::WHITE;
    static constexpr u32 COLOR_GRAY = Colors::MC_GRAY;
    static constexpr u32 COLOR_YELLOW = Colors::MC_YELLOW;
    static constexpr u32 COLOR_GREEN = Colors::MC_GREEN;

    // 高度图类型名称（MC 1.16.5风格）
    static constexpr const char* HEIGHTMAP_NAMES[6] = {
        "SW",   // WORLD_SURFACE_WG
        "S",    // WORLD_SURFACE
        "OW",   // OCEAN_FLOOR_WG
        "O",    // OCEAN_FLOOR
        "M",    // MOTION_BLOCKING
        "ML"    // MOTION_BLOCKING_NO_LEAVES
    };
};

} // namespace client
} // namespace mc
