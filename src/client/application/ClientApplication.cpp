#include "ClientApplication.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/math/ray/Raycast.hpp"
#include "client/renderer/ChunkMesher.hpp"
#include "minecraft-reborn/version.h"

#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>

namespace mr::client {

namespace {

/**
 * @brief ClientWorld 到 IBlockReader 的轻量适配器
 *
 * 射线检测接口当前要求 IBlockReader，
 * 而 ClientWorld 实现的是 ICollisionWorld（方法签名兼容）。
 */
class ClientWorldBlockReader final : public mr::IBlockReader {
public:
    explicit ClientWorldBlockReader(const ClientWorld& world)
        : m_world(world)
    {
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return m_world.getBlockState(x, y, z);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override
    {
        return m_world.isWithinWorldBounds(x, y, z);
    }

private:
    const ClientWorld& m_world;
};

} // namespace

ClientApplication::ClientApplication() = default;

ClientApplication::~ClientApplication()
{
    if (m_running) {
        stop();
    }
}

Result<void> ClientApplication::initialize(const ClientLaunchParams& params)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Client already initialized");
    }

    // 加载设置
    String settingsPath = params.settingsPath.value_or(
        ClientSettings::getSettingsPath("minecraft-reborn").string());
    auto settingsResult = loadSettings(settingsPath);
    if (settingsResult.failed()) {
        spdlog::warn("Failed to load settings from {}: {}. Using defaults.",
                     settingsPath, settingsResult.error().toString());
    }

    // 应用命令行覆盖
    if (params.fullscreen.has_value()) {
        m_settings.fullscreen.set(*params.fullscreen);
    }
    if (params.serverAddress.has_value()) {
        m_settings.serverAddress.set(*params.serverAddress);
    }
    if (params.serverPort.has_value()) {
        m_settings.serverPort.set(*params.serverPort);
    }
    if (params.username.has_value()) {
        m_settings.username.set(*params.username);
    }

    // 应用设置到系统
    applySettings();

    // 设置日志级别
    const auto& logLevel = m_settings.logLevel.get();
    if (logLevel == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (logLevel == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (logLevel == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (logLevel == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (logLevel == "error") {
        spdlog::set_level(spdlog::level::err);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    spdlog::info("=== Minecraft Reborn Client ===");
    spdlog::info("Version: {}.{}.{}", MR_VERSION_MAJOR, MR_VERSION_MINOR, MR_VERSION_PATCH);
    spdlog::info("Initializing client...");

    // 初始化方块注册表
    VanillaBlocks::initialize();
    spdlog::info("Vanilla blocks initialized");

    // 初始化资源系统
    spdlog::info("Initializing resource system...");
    auto resourceResult = initializeResources();
    if (resourceResult.failed()) {
        spdlog::warn("Failed to initialize resource system: {}. Using default rendering.",
                     resourceResult.error().toString());
    }

    // 初始化按键绑定
    m_settings.initializeKeyBindings();
    spdlog::info("Key bindings initialized");

    // 创建窗口
    WindowConfig windowConfig;
    windowConfig.width = 1280;
    windowConfig.height = 720;
    windowConfig.title = "Minecraft Reborn";
    windowConfig.fullscreen = m_settings.fullscreen.get();
    windowConfig.vsync = m_settings.vsync.get();
    windowConfig.samples = 4;

    auto windowResult = m_window.create(windowConfig);
    if (windowResult.failed()) {
        spdlog::error("Failed to create window: {}", windowResult.error().toString());
        return windowResult.error();
    }

    // 初始化输入管理器
    m_input.initialize(m_window.handle());

    // 设置按键绑定
    setupInputBindings();

    // 设置设置变更回调
    setupSettingCallbacks();

    // 设置窗口大小变化回调
    m_window.setResizeCallback([](i32 width, i32 height, void* userData) {
        auto* app = static_cast<ClientApplication*>(userData);
        spdlog::info("Window resized: {}x{}", width, height);
        if (app && app->m_renderer) {
            auto result = app->m_renderer->onResize(static_cast<u32>(width), static_cast<u32>(height));
            if (result.failed()) {
                spdlog::error("Failed to handle resize: {}", result.error().toString());
            }
            // 更新相机宽高比
            app->m_camera.setAspectRatio(static_cast<f32>(width) / static_cast<f32>(height));
        }
    }, this);

    // 初始化Vulkan渲染器
    spdlog::info("Initializing Vulkan renderer...");
    m_renderer = std::make_unique<VulkanRenderer>();

    RendererConfig rendererConfig;
    rendererConfig.vulkanConfig.appName = "Minecraft Reborn";
    rendererConfig.vulkanConfig.enableValidation = true; // Debug模式启用验证层
    rendererConfig.enableVSync = m_settings.vsync.get();
    rendererConfig.swapChainConfig.width = static_cast<u32>(windowConfig.width);
    rendererConfig.swapChainConfig.height = static_cast<u32>(windowConfig.height);

    auto rendererResult = m_renderer->initialize(m_window.handle(), rendererConfig);
    if (rendererResult.failed()) {
        spdlog::error("Failed to initialize renderer: {}", rendererResult.error().toString());
        m_window.destroy();
        return rendererResult.error();
    }

    // 设置相机
    setupCamera();

    // 将相机设置给渲染器
    m_renderer->setCamera(&m_camera);

    // 启动内置服务端
    if (!params.skipIntegratedServer) {
        spdlog::info("Starting integrated server...");

        m_integratedServer = std::make_unique<server::IntegratedServer>();
        server::IntegratedServerConfig serverConfig;
        serverConfig.seed = 12345;
        serverConfig.viewDistance = m_settings.renderDistance.get();

        auto serverResult = m_integratedServer->initialize(serverConfig);
        if (serverResult.failed()) {
            spdlog::error("Failed to initialize integrated server: {}", serverResult.error().toString());
            m_renderer->destroy();
            m_window.destroy();
            return serverResult.error();
        }

        // 初始化网络客户端
        m_networkClient = std::make_unique<NetworkClient>();
        setupNetworkCallbacks();

        NetworkClientConfig clientConfig;
        clientConfig.username = m_settings.username.get();
        auto clientResult = m_networkClient->connectLocal(m_integratedServer->getClientEndpoint());
        if (clientResult.failed()) {
            spdlog::error("Failed to connect to integrated server: {}", clientResult.error().toString());
            m_integratedServer->stop();
            m_renderer->destroy();
            m_window.destroy();
            return clientResult.error();
        }

        m_useIntegratedServer = true;
        spdlog::info("Connected to integrated server");
    } else {
        m_useIntegratedServer = false;
    }

    // 初始化世界
    spdlog::info("Initializing world...");
    auto worldResult = m_world.initialize(12345); // 使用固定种子
    if (worldResult.failed()) {
        spdlog::error("Failed to initialize world: {}", worldResult.error().toString());
        if (m_integratedServer) {
            m_integratedServer->stop();
        }
        m_renderer->destroy();
        m_window.destroy();
        return worldResult.error();
    }

    // 初始化网格构建线程池
    spdlog::info("Initializing mesh worker pool...");
    m_world.initializeMeshWorkerPool();

    // 设置区块卸载回调，通知 ChunkRenderer 释放 GPU 缓冲区
    m_world.setChunkUnloadCallback([this](const ChunkId& chunkId) {
        if (m_renderer && m_renderer->isChunkRendererInitialized()) {
            m_renderer->chunkRenderer().removeChunk(chunkId);
        }
    });

    // 初始化方块碰撞注册表
    spdlog::info("Initializing block collision registry...");

    // 创建物理引擎
    m_physicsEngine = std::make_unique<PhysicsEngine>(m_world);

    // 创建玩家实体
    m_player = std::make_unique<Player>(static_cast<EntityId>(1), m_settings.username.get());
    m_player->setPosition(8.0, 50.0, 8.0);  // 初始位置在地面上方
    m_player->setPhysicsEngine(m_physicsEngine.get());
    // 默认创造模式并启用飞行
    m_player->setGameMode(GameMode::Creative);
    m_player->abilities().flying = true;
    spdlog::info("Player created at (8, 50, 8)");

    spdlog::info("Client initialized successfully");
    spdlog::info("Window: {}x{}", m_window.width(), m_window.height());
    spdlog::info("Controls: WASD to move, Space to jump/fly up, Shift to sneak/fly down, mouse to look");
    spdlog::info("Press F to toggle flying, F3 to toggle debug screen, ALT to toggle mouse capture");

    // 初始化调试屏幕
    if (m_renderer->isGuiRendererInitialized()) {
        m_debugScreen.initialize(&m_renderer->guiRenderer());
        m_debugScreen.setCamera(&m_camera);
        m_debugScreen.setWorld(&m_world);
        spdlog::info("Debug screen initialized");

        // 初始化准星渲染器
        auto crosshairResult = m_crosshair.initialize(&m_renderer->guiRenderer());
        if (crosshairResult.failed()) {
            spdlog::warn("Failed to initialize crosshair: {}", crosshairResult.error().toString());
        } else {
            spdlog::info("Crosshair initialized");
        }

        // 设置GUI渲染回调
        m_renderer->setGuiRenderCallback([this]() {
            // 先渲染准星
            m_crosshair.render();
            // 再渲染调试屏幕
            if (m_debugScreenVisible) {
                m_debugScreen.render();
            }
        });
    }

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

    // 初始捕获鼠标
    toggleMouseCapture();

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

    // 检查ALT键切换鼠标捕获
    if (m_input.isKeyJustPressed(GLFW_KEY_LEFT_ALT) ||
        m_input.isKeyJustPressed(GLFW_KEY_RIGHT_ALT)) {
        toggleMouseCapture();
    }

    // 飞行模式切换（F键）
    if (m_input.isKeyJustPressed(GLFW_KEY_F)) {
        if (m_player && m_player->abilities().canFly) {
            m_player->toggleFlying();
        }
    }

    // 传递键盘输入到玩家和鼠标控制
    if (m_mouseCaptured && m_player) {
        // 鼠标视角控制 - 更新玩家朝向（使用设置中的灵敏度）
        f32 sensitivity = m_settings.mouseSensitivity.get() * 0.2f;
        f32 deltaYaw = m_input.mouseDeltaX() * sensitivity;
        f32 deltaPitch = m_input.mouseDeltaY() * sensitivity;
        m_player->rotate(deltaYaw, -deltaPitch);  // pitch方向相反

        // 收集移动输入
        f32 forward = 0.0f;
        f32 strafe = 0.0f;
        bool jumping = false;
        bool sneaking = false;

        if (m_input.isKeyPressed(GLFW_KEY_W)) forward += 1.0f;
        if (m_input.isKeyPressed(GLFW_KEY_S)) forward -= 1.0f;
        if (m_input.isKeyPressed(GLFW_KEY_A)) strafe -= 1.0f;
        if (m_input.isKeyPressed(GLFW_KEY_D)) strafe += 1.0f;
        if (m_input.isKeyPressed(GLFW_KEY_SPACE)) jumping = true;
        if (m_input.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) sneaking = true;

        // 传递输入给玩家
        m_player->handleMovementInput(forward, strafe, jumping, sneaking);
    }
}

void ClientApplication::update(f32 deltaTime)
{
    // 更新网络客户端（处理服务端数据包）
    if (m_networkClient) {
        m_networkClient->poll();
    }

    // 更新玩家物理
    if (m_player) {
        // 应用物理（重力、碰撞检测）
        m_player->updatePhysics();

        // 同步相机位置到玩家眼睛位置
        m_camera.setPosition(
            static_cast<f32>(m_player->x()),
            static_cast<f32>(m_player->y() + m_player->eyeHeight()),
            static_cast<f32>(m_player->z())
        );
        m_camera.setYaw(m_player->yaw());
        m_camera.setPitch(m_player->pitch());
        m_camera.update(deltaTime);
    } else {
        // 后备：更新相机控制器（这会调用 Camera::update 更新矩阵）
        m_cameraController.update(deltaTime);
    }

    // 发送玩家位置到服务端（限制频率）
    if (m_networkClient && m_networkClient->isLoggedIn() && m_player) {
        m_positionSendAccumulator += deltaTime;
        if (m_positionSendAccumulator >= POSITION_SEND_INTERVAL) {
            m_positionSendAccumulator = 0.0f;
            sendPlayerPosition();
        }
    }

    // 更新调试屏幕
    m_debugScreen.update(deltaTime);

    // 执行射线检测
    if (m_player && m_mouseCaptured) {
        // 获取玩家眼睛位置
        glm::vec3 eyePos = m_camera.position();

        // 获取视线方向
        glm::vec3 forward = m_camera.forward();

        // 创建射线
        mr::Vector3 origin(eyePos.x, eyePos.y, eyePos.z);
        mr::Vector3 direction(forward.x, forward.y, forward.z);
        mr::Ray ray(origin, direction);

        // 执行射线检测（创造模式使用更远的距离）
        mr::RaycastContext context(ray, 5.0f);  // 生存模式5格
        ClientWorldBlockReader blockReader(m_world);
        m_raycastResult = mr::raycastBlocks(context, blockReader);

        // 更新调试屏幕的目标方块
        m_debugScreen.setTargetBlock(&m_raycastResult);
    } else {
        // 没有玩家时清除目标方块
        m_debugScreen.setTargetBlock(nullptr);
    }

    // 更新世界（根据相机位置加载/卸载区块）
    m_world.update(m_camera.position(), m_settings.renderDistance.get());

    // 处理异步网格构建结果
    m_world.processMeshBuildResults(4);  // 每帧最多处理 4 个网格

    // 同步时间到渲染器（驱动天空、太阳、月亮、星空变化）
    if (m_renderer) {
        constexpr i64 DAY_LENGTH_TICKS = 24000;

        if (m_hasServerTimeSync) {
            // 有服务端时间同步时，以服务端时间为准
            m_renderGameTime = m_world.gameTime();
            m_renderDayTime = m_world.dayTime();
            m_renderTickAccumulator += deltaTime * 20.0f;
            while (m_renderTickAccumulator >= 1.0f) {
                m_renderTickAccumulator -= 1.0f;
            }
        } else {
            // 本地回退：没有时间包时仍推进昼夜循环
            m_renderTickAccumulator += deltaTime * 20.0f;
            while (m_renderTickAccumulator >= 1.0f) {
                m_renderTickAccumulator -= 1.0f;
                ++m_renderGameTime;
                m_renderDayTime = (m_renderDayTime + 1) % DAY_LENGTH_TICKS;
            }
        }

        m_renderer->updateTime(m_renderDayTime, m_renderGameTime, m_renderTickAccumulator);
    }

    // 上传网格到 GPU（只处理已完成异步构建的网格）
    if (m_renderer->isChunkRendererInitialized()) {
        auto& chunkRenderer = m_renderer->chunkRenderer();
        m_world.forEachDirtyMesh([this, &chunkRenderer](const ChunkId& id, ClientChunk& chunk) {
            // 检查网格是否为空
            if (chunk.solidMesh.empty()) {
                chunk.needsMeshUpdate = false;
                return;
            }

            // 上传网格到GPU
            ChunkId chunkId(id.x, id.z);
            auto result = chunkRenderer.updateChunk(chunkId, chunk.solidMesh);
            if (result.success()) {
                chunk.needsMeshUpdate = false;
            } else {
                spdlog::error("Failed to update chunk mesh: {}", result.error().toString());
            }
        });
    }
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

    // 保存设置
    auto saveResult = m_settings.saveSettings(
        ClientSettings::getSettingsPath("minecraft-reborn"));
    if (saveResult.failed()) {
        spdlog::warn("Failed to save settings: {}", saveResult.error().toString());
    }

    // 断开网络连接
    if (m_networkClient) {
        m_networkClient->disconnect("Client shutdown");
        m_networkClient.reset();
    }

    // 停止内置服务端
    if (m_integratedServer) {
        m_integratedServer->stop();
        m_integratedServer.reset();
    }

    // 清理渲染器
    if (m_renderer) {
        m_renderer->destroy();
        m_renderer.reset();
    }

    // 清理玩家
    m_player.reset();
    m_physicsEngine.reset();

    // 清理世界（包括关闭网格构建线程池）
    m_world.destroy();

    m_window.destroy();

    spdlog::info("Client stopped.");
}

// 设置相关方法

Result<void> ClientApplication::loadSettings(const String& path)
{
    auto result = m_settings.loadSettings(path);
    if (result.failed()) {
        return result;
    }

    // 确保设置目录存在
    ClientSettings::ensureSettingsDir("minecraft-reborn");

    // 启用自动保存
    m_settings.enableAutoSave(path);

    return Result<void>::ok();
}

void ClientApplication::applySettings()
{
    // 设置变更回调在 setupSettingCallbacks 中设置
    // 这里应用初始设置值
}

void ClientApplication::setupSettingCallbacks()
{
    // 渲染距离变更
    m_settings.renderDistance.onChange([this](i32 value) {
        spdlog::info("Render distance changed to: {}", value);
        // 世界更新时会使用新值
    });

    // 全屏模式变更
    m_settings.fullscreen.onChange([this](bool value) {
        spdlog::info("Fullscreen changed to: {}", value);
        // TODO: 实现全屏切换
    });

    // VSync 变更
    m_settings.vsync.onChange([this](bool value) {
        spdlog::info("VSync changed to: {}", value);
        // TODO: 实现动态 VSync 切换
    });

    // 鼠标灵敏度变更
    m_settings.mouseSensitivity.onChange([this](f32 value) {
        spdlog::info("Mouse sensitivity changed to: {}", value);
        // 鼠标灵敏度在 handleEvents 中应用
    });

    // FOV 变更
    m_settings.fov.onChange([this](f32 value) {
        spdlog::info("FOV changed to: {}", value);
        m_camera.setFOV(value);
    });
}

// 辅助函数实现

void ClientApplication::setupInputBindings()
{
    m_input.bindKeyAction(GLFW_KEY_ESCAPE, "exit");
    m_input.bindKeyAction(GLFW_KEY_F3, "toggle_debug");

    m_input.bindActionCallback("exit", [this]() {
        spdlog::info("Exit key pressed");
        stop();
    });

    m_input.bindActionCallback("toggle_debug", [this]() {
        m_debugScreenVisible = !m_debugScreenVisible;
        m_debugScreen.setVisible(m_debugScreenVisible);
    });
}

void ClientApplication::setupCamera()
{
    // 设置相机配置
    CameraConfig cameraConfig;
    cameraConfig.fov = m_settings.fov.get();
    cameraConfig.aspectRatio = static_cast<f32>(m_window.width()) / static_cast<f32>(m_window.height());
    cameraConfig.nearPlane = 0.1f;
    cameraConfig.farPlane = 1000.0f;
    cameraConfig.moveSpeed = 10.0f;      // 移动速度
    cameraConfig.mouseSensitivity = m_settings.mouseSensitivity.get() * 0.2f; // 鼠标灵敏度

    m_camera = Camera(cameraConfig);

    // 设置初始位置（在测试区块上方）
    m_camera.setPosition(8.0f, 50.0f, 8.0f);
    m_camera.setYaw(45.0f);
    m_camera.update(0.0f);

    // 设置相机控制器
    m_cameraController.setCamera(&m_camera);
}

void ClientApplication::setupNetworkCallbacks()
{
    if (!m_networkClient) return;

    NetworkClientCallbacks callbacks;

    callbacks.onLoginSuccess = [this](PlayerId playerId, const String& username) {
        spdlog::info("Login successful: playerId={}, username={}", playerId, username);
    };

    callbacks.onLoginFailed = [this](const String& reason) {
        spdlog::error("Login failed: {}", reason);
        stop();
    };

    callbacks.onDisconnected = [this](const String& reason) {
        spdlog::warn("Disconnected: {}", reason);
    };

    callbacks.onChunkData = [this](ChunkCoord x, ChunkCoord z, const std::vector<u8>& data) {
        m_world.onChunkData(x, z, std::vector<u8>(data));
    };

    callbacks.onChunkUnload = [this](ChunkCoord x, ChunkCoord z) {
        m_world.onChunkUnload(x, z);
    };

    callbacks.onTeleport = [this](f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId) {
        if (m_player) {
            m_player->setPosition(x, y, z);
            m_player->setRotation(yaw, pitch);
        }
        m_camera.setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        m_camera.setYaw(yaw);
        m_camera.setPitch(pitch);
        m_camera.update(0.0f);  // 立即更新相机矩阵
        // 注意：sendTeleportConfirm 已在 NetworkClient::handleTeleport 中调用
    };

    callbacks.onBlockUpdate = [this](i32 bx, i32 by, i32 bz, u32 blockStateId) {
        m_world.setBlock(bx, by, bz, Block::getBlockState(blockStateId));
    };

    callbacks.onTimeUpdate = [this](i64 gameTime, i64 dayTime, bool daylightCycleEnabled) {
        m_world.onTimeUpdate(gameTime, dayTime, daylightCycleEnabled);
        m_renderGameTime = gameTime;
        m_renderDayTime = dayTime;
        m_renderTickAccumulator = 0.0f;
        m_hasServerTimeSync = true;
    };

    m_networkClient->setCallbacks(callbacks);
}

void ClientApplication::toggleMouseCapture()
{
    m_mouseCaptured = !m_mouseCaptured;
    m_input.setMouseLocked(m_mouseCaptured);

    if (m_mouseCaptured) {
        spdlog::debug("Mouse captured - first person mode");
    } else {
        spdlog::debug("Mouse released - UI mode");
    }
}

void ClientApplication::sendPlayerPosition()
{
    if (!m_networkClient || !m_networkClient->isLoggedIn() || !m_player) {
        return;
    }

    const auto& pos = m_player->position();

    // 检查是否需要发送（位置或旋转变化）
    bool positionChanged =
        std::abs(pos.x - m_lastSentX) > 0.001 ||
        std::abs(pos.y - m_lastSentY) > 0.001 ||
        std::abs(pos.z - m_lastSentZ) > 0.001;

    bool rotationChanged =
        std::abs(m_player->yaw() - m_lastSentYaw) > 0.01f ||
        std::abs(m_player->pitch() - m_lastSentPitch) > 0.01f;

    network::PlayerPosition playerPos;
    playerPos.x = pos.x;
    playerPos.y = pos.y;
    playerPos.z = pos.z;
    playerPos.yaw = m_player->yaw();
    playerPos.pitch = m_player->pitch();
    playerPos.onGround = m_player->onGround();

    network::PlayerMovePacket::MoveType type;
    if (positionChanged && rotationChanged) {
        type = network::PlayerMovePacket::MoveType::Full;
    } else if (positionChanged) {
        type = network::PlayerMovePacket::MoveType::Position;
    } else if (rotationChanged) {
        type = network::PlayerMovePacket::MoveType::Rotation;
    } else {
        // 无变化，只发送地面状态
        type = network::PlayerMovePacket::MoveType::GroundOnly;
    }

    m_networkClient->sendPlayerMove(playerPos, type);

    // 更新上次发送的位置
    m_lastSentX = pos.x;
    m_lastSentY = pos.y;
    m_lastSentZ = pos.z;
    m_lastSentYaw = m_player->yaw();
    m_lastSentPitch = m_player->pitch();
}

Result<void> ClientApplication::initializeResources()
{
    // 1. 扫描资源包目录
    String resourcePackDir = m_settings.resourcePackDir.get();
    if (resourcePackDir.empty()) {
        resourcePackDir = "resourcepacks";
    }

    spdlog::info("Scanning resource pack directory: {}", resourcePackDir);
    auto scanResult = m_resourcePackList.scanDirectory(std::filesystem::path(resourcePackDir));
    if (scanResult.success()) {
        spdlog::info("Found {} resource packs", scanResult.value());
    } else {
        spdlog::warn("Failed to scan resource pack directory: {}", scanResult.error().toString());
    }

    // 2. 从设置加载资源包配置
    m_resourcePackList.loadFromSettings(m_settings.resourcePacks);

    // 3. 创建 ResourceManager
    m_resourceManager = std::make_unique<ResourceManager>();

    // 4. 添加启用的资源包
    auto enabledPacks = m_resourcePackList.getEnabledPacks();
    for (const auto& pack : enabledPacks) {
        auto result = m_resourceManager->addResourcePack(pack);
        if (result.failed()) {
            spdlog::warn("Failed to add resource pack: {}", result.error().toString());
        } else {
            spdlog::info("Added resource pack: {}", pack->name());
        }
    }

    // 5. 加载所有资源（如果有资源包）
    bool hasResources = false;
    if (m_resourceManager->resourcePackCount() > 0) {
        auto loadResult = m_resourceManager->loadAllResources();
        if (loadResult.failed()) {
            spdlog::warn("Failed to load resources: {}", loadResult.error().toString());
        } else {
            hasResources = true;
            spdlog::info("Loaded {} resource packs", m_resourceManager->resourcePackCount());
        }

        // 6. 构建纹理图集
        auto atlasResult = m_resourceManager->buildTextureAtlas();
        if (atlasResult.failed()) {
            spdlog::warn("Failed to build texture atlas: {}", atlasResult.error().toString());
        } else {
            spdlog::info("Built texture atlas: {}x{}, {} textures",
                        atlasResult.value().width,
                        atlasResult.value().height,
                        atlasResult.value().regions.size());
        }
    } else {
        spdlog::info("No resource packs found, using default resources (missing model)");
    }

    // 7. 初始化 BlockModelCache（即使没有资源包也要初始化，使用缺失模型）
    if (m_modelCache.initialize(*m_resourceManager)) {
        spdlog::info("Block model cache initialized with {} appearances",
                    m_modelCache.cachedAppearanceCount());
        // 设置 ChunkMesher 使用 BlockModelCache
        ChunkMesher::setModelCache(&m_modelCache);
    } else {
        spdlog::warn("Failed to initialize block model cache");
    }

    // 8. 设置资源包变更回调
    m_resourcePackList.onChange([this]() {
        spdlog::info("Resource packs changed, reloading...");
        reloadResources();
    });

    return Result<void>::ok();
}

void ClientApplication::reloadResources()
{
    if (!m_resourceManager) {
        return;
    }

    // 清除资源管理器
    m_resourceManager->clearResourcePacks();

    // 重新添加启用的资源包
    auto enabledPacks = m_resourcePackList.getEnabledPacks();
    for (const auto& pack : enabledPacks) {
        auto result = m_resourceManager->addResourcePack(pack);
        if (result.failed()) {
            spdlog::warn("Failed to add resource pack: {}", result.error().toString());
        }
    }

    // 重新加载资源
    if (m_resourceManager->resourcePackCount() > 0) {
        auto loadResult = m_resourceManager->reload();
        if (loadResult.failed()) {
            spdlog::error("Failed to reload resources: {}", loadResult.error().toString());
            return;
        }

        // 重新构建纹理图集
        auto atlasResult = m_resourceManager->buildTextureAtlas();
        if (atlasResult.failed()) {
            spdlog::error("Failed to rebuild texture atlas: {}", atlasResult.error().toString());
            return;
        }

        // 重建模型缓存
        if (m_modelCache.rebuild(*m_resourceManager)) {
            spdlog::info("Reloaded resources: {} appearances cached",
                        m_modelCache.cachedAppearanceCount());
        }
    }

    // TODO: 通知渲染器重新创建纹理图集
    // TODO: 通知世界重新生成所有区块网格
}

} // namespace mr::client
