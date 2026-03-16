#pragma once

#include "../../renderer/trident/gui/GuiRenderer.hpp"
#include "../../renderer/Camera.hpp"
#include "../../../common/core/BlockRaycastResult.hpp"
#include "../../../common/world/time/GameTime.hpp"
#include "../../../common/util/PlatformInfo.hpp"
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
 * @brief GPU信息结构
 *
 * 用于存储从Vulkan获取的GPU信息
 */
struct DebugGpuInfo {
    String vendor;              // GPU厂商 (NVIDIA, AMD, Intel等)
    String name;                // GPU型号
    String driverVersion;       // 驱动版本
    u64 dedicatedVideoMB = 0;   // 专用显存 (MB)
    u64 sharedSystemMB = 0;     // 共享系统内存 (MB)
    u32 apiMajorVersion = 0;    // Vulkan API主版本
    u32 apiMinorVersion = 0;    // Vulkan API次版本
};

/**
 * @brief Spawn统计信息
 *
 * 参考 MC 1.16.5 NaturalSpawner.EntityDensityManager
 */
struct SpawnStats {
    i32 spawnableChunkCount = 0;         // 可生成区块数 (SC)
    std::array<i32, 5> categoryCounts{}; // 各分类实体数 (M: 怪物, C: 生物, A: 环境生物, W: 水中生物, N: 下界生物)
    bool available = false;              // 是否可用
};

/**
 * @brief 高度图信息
 *
 * 参考 MC 1.16.5 Heightmap.Type
 */
struct HeightmapInfo {
    i32 worldSurfaceWG = -1;    // SW: 世界表面（世界生成）
    i32 worldSurface = -1;      // S: 世界表面
    i32 oceanFloorWG = -1;      // OW: 海底（世界生成）
    i32 oceanFloor = -1;        // O: 海底
    i32 motionBlocking = -1;    // M: 阻挡运动
    i32 motionBlockingNoLeaves = -1; // ML: 阻挡运动（无树叶）
    bool available = false;
};

/**
 * @brief 调试屏幕 (F3屏幕)
 *
 * 显示游戏调试信息，高度还原Minecraft 1.16.5的F3调试屏幕：
 * - 左侧面板：版本、FPS、位置、区块、光照、生物群系、高度图等
 * - 右侧面板：Java/C++、内存、CPU、GPU、目标方块/流体/实体
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
     * @brief 设置GPU信息
     * @param info GPU信息
     */
    void setGpuInfo(const DebugGpuInfo& info) { m_gpuInfo = info; m_gpuInfoSet = true; }

    /**
     * @brief 设置当前渲染距离
     */
    void setRenderDistance(i32 distance) { m_renderDistance = distance; }

    /**
     * @brief 设置粒子数
     */
    void setParticleCount(u32 count) { m_particleCount = count; }

    /**
     * @brief 设置活跃Shader名称
     */
    void setActiveShader(const String& shaderName) { m_activeShader = shaderName; }

    /**
     * @brief 设置声音信息
     * @param activeSounds 活跃声音数
     * @param mood 氛围音效百分比 (0.0-1.0)
     */
    void setSoundInfo(u32 activeSounds, f32 mood) {
        m_activeSounds = activeSounds;
        m_moodPercentage = mood;
    }

    /**
     * @brief 设置Spawn统计信息
     */
    void setSpawnStats(const SpawnStats& stats) { m_spawnStats = stats; }

    /**
     * @brief 设置高度图信息
     */
    void setHeightmapInfo(const HeightmapInfo& info) { m_heightmapInfo = info; }

    /**
     * @brief 设置服务器tick时间（毫秒）
     */
    void setServerTickTime(f32 ms) { m_serverTickTimeMs = ms; }

    /**
     * @brief 设置服务器TPS
     */
    void setServerTps(f32 tps) { m_serverTps = tps; }

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
     * @brief 更新系统信息（每秒更新一次）
     */
    void updateSystemInfo();

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
     * @brief 渲染面板
     * @param lines 文本行
     * @param startX X起始位置
     * @param alignRight 是否右对齐
     */
    void renderPanel(const std::vector<std::string>& lines, f32 startX, bool alignRight);

    GuiRenderer* m_guiRenderer = nullptr;
    Camera* m_camera = nullptr;
    ClientWorld* m_world = nullptr;
    const BlockRaycastResult* m_targetBlock = nullptr;
    ClientEntityManager* m_entityManager = nullptr;
    NetworkClient* m_networkClient = nullptr;
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

    // 系统信息更新定时器
    f32 m_systemInfoTimer = 0.0f;
    static constexpr f32 SYSTEM_INFO_UPDATE_INTERVAL = 1.0f;

    // 调试文本行
    std::vector<std::string> m_leftLines;
    std::vector<std::string> m_rightLines;
    std::string m_version;
    std::string m_rendererInfo;

    // 系统信息缓存
    util::MemoryInfo m_memoryInfo;
    util::CpuInfo m_cpuInfo;
    DebugGpuInfo m_gpuInfo;
    bool m_gpuInfoSet = false;

    // 游戏状态信息
    i32 m_renderDistance = 12;
    u32 m_particleCount = 0;
    String m_activeShader;
    u32 m_activeSounds = 0;
    f32 m_moodPercentage = 0.0f;
    SpawnStats m_spawnStats;
    HeightmapInfo m_heightmapInfo;
    f32 m_serverTickTimeMs = 0.0f;
    f32 m_serverTps = 20.0f;

    // 颜色常量
    static constexpr u32 COLOR_WHITE = Colors::WHITE;
    static constexpr u32 COLOR_GRAY = Colors::MC_GRAY;
    static constexpr u32 COLOR_YELLOW = Colors::MC_YELLOW;
    static constexpr u32 COLOR_GREEN = Colors::MC_GREEN;
};

} // namespace client
} // namespace mc
