#include "ClientApplication.hpp"
#include "common/world/block/VanillaBlocks.hpp"
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
    // 初始化方块注册表
    VanillaBlocks::initialize();
    spdlog::info("Vanilla blocks initialized");

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
    setupInputBindings();

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

    // 设置相机
    setupCamera();

    // 将相机设置给渲染器
    m_renderer->setCamera(&m_camera);

    // 启动内置服务端
    if (m_useIntegratedServer) {
        spdlog::info("Starting integrated server...");

        m_integratedServer = std::make_unique<server::IntegratedServer>();
        server::IntegratedServerConfig serverConfig;
        serverConfig.seed = 12345;
        serverConfig.viewDistance = m_config.renderDistance;

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
        clientConfig.username = m_config.username;
        auto clientResult = m_networkClient->connectLocal(m_integratedServer->getClientEndpoint());
        if (clientResult.failed()) {
            spdlog::error("Failed to connect to integrated server: {}", clientResult.error().toString());
            m_integratedServer->stop();
            m_renderer->destroy();
            m_window.destroy();
            return clientResult.error();
        }

        spdlog::info("Connected to integrated server");
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

    // 初始化方块碰撞注册表
    spdlog::info("Initializing block collision registry...");

    // 创建物理引擎
    m_physicsEngine = std::make_unique<PhysicsEngine>(m_world);

    // 创建玩家实体
    m_player = std::make_unique<Player>(static_cast<EntityId>(1), m_config.username);
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
        // 设置GUI渲染回调
        m_renderer->setGuiRenderCallback([this]() {
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
        // 鼠标视角控制 - 更新玩家朝向
        f32 deltaYaw = m_input.mouseDeltaX() * 0.1f;   // 鼠标灵敏度
        f32 deltaPitch = m_input.mouseDeltaY() * 0.1f;
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

    // 更新世界（根据相机位置加载/卸载区块）
    m_world.update(m_camera.position(), m_config.renderDistance);

    // 上传网格到 GPU
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

    // 清理世界
    m_world.destroy();

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
    cameraConfig.fov = 70.0f;
    cameraConfig.aspectRatio = static_cast<f32>(m_config.windowWidth) / static_cast<f32>(m_config.windowHeight);
    cameraConfig.nearPlane = 0.1f;
    cameraConfig.farPlane = 1000.0f;
    cameraConfig.moveSpeed = 10.0f;      // 移动速度
    cameraConfig.mouseSensitivity = 0.1f; // 鼠标灵敏度

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

} // namespace mr::client
