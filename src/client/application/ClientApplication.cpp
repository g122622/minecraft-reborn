#include "ClientApplication.hpp"
#include "common/item/Items.hpp"
#include "common/item/BlockItemRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/drop/DropTables.hpp"
#include "common/math/ray/Raycast.hpp"
#include "common/resource/VanillaResources.hpp"
#include "common/entity/VanillaEntities.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/EntityRendererManager.hpp"
#include "client/resource/ResourceManager.hpp"
#include "client/resource/TextureAtlasBuilder.hpp"
#include "client/ui/hud/HudRenderer.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/ui/screen/CraftingScreen.hpp"
#include "minecraft-reborn/version.h"

#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <chrono>
#include <filesystem>

namespace mc::client {

namespace {

template <typename Menu>
void applyContainerContents(Menu* menu, const std::vector<ItemStack>& items) {
    if (menu == nullptr) {
        return;
    }

    const i32 slotCount = std::min(menu->getSlotCount(), static_cast<i32>(items.size()));
    for (i32 slotIndex = 0; slotIndex < slotCount; ++slotIndex) {
        Slot* slot = menu->getSlot(slotIndex);
        if (slot != nullptr) {
            slot->set(items[slotIndex]);
        }
    }
}

template <typename Menu>
void applyContainerSlot(Menu* menu, i32 slotIndex, const ItemStack& item) {
    if (menu == nullptr) {
        return;
    }

    Slot* slot = menu->getSlot(slotIndex);
    if (slot != nullptr) {
        slot->set(item);
    }
}

void releaseMouseForScreen(InputManager& input, bool& mouseCaptured) {
    if (mouseCaptured) {
        input.setMouseLocked(false);
        mouseCaptured = false;
    }
}

void captureMouseAfterScreens(InputManager& input, bool& mouseCaptured) {
    if (!mouseCaptured) {
        input.setMouseLocked(true);
        mouseCaptured = true;
    }
}

[[nodiscard]] f32 calculateBlockBreakingDelta(const Player& player, const BlockState& state)
{
    const f32 hardness = state.hardness();
    if (hardness < 0.0f) {
        return 0.0f;
    }

    if (hardness == 0.0f) {
        return 1.0f;
    }

    const ItemStack heldItem = player.inventory().getSelectedStack();
    const f32 destroySpeed = std::max(heldItem.getDestroySpeed(state), 1.0f);
    const bool canHarvest = heldItem.isEmpty() ? true : heldItem.canHarvestBlock(state);
    const f32 divisor = canHarvest ? 30.0f : 100.0f;
    return destroySpeed / hardness / divisor;
}

/**
 * @brief ClientWorld 到 IBlockReader 的轻量适配器
 *
 * 射线检测接口当前要求 IBlockReader，
 * 而 ClientWorld 实现的是 ICollisionWorld（方法签名兼容）。
 */
class ClientWorldBlockReader final : public mc::IBlockReader {
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
    spdlog::info("Version: {}.{}.{}", MC_VERSION_MAJOR, MC_VERSION_MINOR, MC_VERSION_PATCH);
    spdlog::info("Initializing client...");

    // 初始化性能追踪
    mc::perfetto::TraceConfig traceConfig;
    traceConfig.outputPath = "client_trace.perfetto-trace";
    traceConfig.bufferSizeKb = 65536; // 64MB
    mc::perfetto::PerfettoManager::instance().initialize(traceConfig);
    mc::perfetto::PerfettoManager::instance().startTracing();

    // 设置进程和主线程名称
    mc::perfetto::PerfettoManager::instance().setProcessName("MinecraftClient");
    mc::perfetto::PerfettoManager::instance().setThreadName("ClientMainThread");
    spdlog::info("Perfetto tracing initialized");

    // 初始化方块注册表
    VanillaBlocks::initialize();
    spdlog::info("Vanilla blocks initialized");

    Items::initialize();
    DropTableRegistry::instance().initializeVanillaDrops();
    spdlog::info("Vanilla items and drop tables initialized");

    // 注册实体类型
    entity::VanillaEntities::registerAll();
    spdlog::info("Entity types registered");

    // 初始化方块物品注册表
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    spdlog::info("Block items initialized");

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

    // 初始化Trident渲染引擎
    spdlog::info("Initializing Trident renderer...");
    m_renderer = std::make_unique<renderer::trident::TridentEngine>();

    renderer::api::RenderEngineConfig rendererConfig;
    rendererConfig.appName = "Minecraft Reborn";
    rendererConfig.enableValidation = true; // Debug模式启用验证层
    rendererConfig.enableVSync = m_settings.vsync.get();
    rendererConfig.initialWindowWidth = static_cast<u32>(windowConfig.width);
    rendererConfig.initialWindowHeight = static_cast<u32>(windowConfig.height);

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

    // 更新渲染器纹理图集（使用 ResourceManager 构建的纹理）
    if (m_resourceManager) {
        spdlog::info("ResourceManager exists, atlas built: {}", m_resourceManager->isAtlasBuilt());
        if (m_resourceManager->isAtlasBuilt()) {
            const auto& atlasResult = m_resourceManager->atlasResult();
            spdlog::info("Atlas pixels size: {}, width: {}, height: {}",
                        atlasResult.pixels.size(), atlasResult.width, atlasResult.height);
            if (!atlasResult.pixels.empty()) {
                spdlog::info("Updating renderer texture atlas...");
                auto atlasUpdateResult = m_renderer->updateTextureAtlas(atlasResult);
                if (atlasUpdateResult.failed()) {
                    spdlog::error("Failed to update texture atlas: {}", atlasUpdateResult.error().toString());
                } else {
                    spdlog::info("Renderer texture atlas updated from resource pack");
                }
            } else {
                spdlog::warn("Atlas pixels empty, skipping renderer update");
            }
        } else {
            spdlog::warn("Atlas not built, skipping renderer update");
        }
    } else {
        spdlog::warn("ResourceManager is null, skipping texture atlas update");
    }

    // 初始化 Trident 子渲染器
    {
        auto skyInitResult = m_renderer->initializeSkyRenderer();
        if (skyInitResult.failed()) {
            spdlog::warn("Failed to initialize sky renderer: {}", skyInitResult.error().toString());
        }

        auto guiInitResult = m_renderer->initializeGuiRenderer();
        if (guiInitResult.failed()) {
            spdlog::warn("Failed to initialize GUI renderer: {}", guiInitResult.error().toString());
        }

        // 实体渲染器必须先初始化（创建 EntityPipeline）
        auto entityInitResult = m_renderer->initializeEntityRenderer();
        if (entityInitResult.failed()) {
            spdlog::warn("Failed to initialize entity renderer: {}", entityInitResult.error().toString());
        }

        // 实体纹理图集在 EntityPipeline 创建后初始化
        if (m_resourceManager) {
            spdlog::info("Initializing entity texture atlas...");
            auto entityAtlasResult = m_renderer->initializeEntityTextureAtlas(m_resourceManager.get());
            if (entityAtlasResult.failed()) {
                spdlog::warn("Failed to initialize entity texture atlas: {}", entityAtlasResult.error().toString());
            }
        }

        if (m_resourceManager) {
            auto itemInitResult = m_renderer->initializeItemRenderer(m_resourceManager.get());
            if (itemInitResult.failed()) {
                spdlog::warn("Failed to initialize item renderer: {}", itemInitResult.error().toString());
            }
        }

        // 初始化雾效果管理器
        auto fogInitResult = m_renderer->initializeFogManager();
        if (fogInitResult.failed()) {
            spdlog::warn("Failed to initialize fog manager: {}", fogInitResult.error().toString());
        }
    }

    // 启动内置服务端
    if (!params.skipIntegratedServer) {
        spdlog::info("Starting integrated server...");

        m_integratedServer = std::make_unique<server::IntegratedServer>();
        server::IntegratedServerConfig serverConfig;
        serverConfig.seed = 12345;
        serverConfig.viewDistance = m_settings.renderDistance.get();
        serverConfig.defaultGameMode = GameMode::Creative;

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
        auto clientResult = m_networkClient->connectLocal(m_integratedServer->getClientEndpoint(), clientConfig);
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

    // 设置实体渲染回调
    m_renderer->setEntityRenderCallback([this](VkCommandBuffer cmd, f32 partialTick) {
        m_world.entityManager().forEachEntity([&](client::ClientEntity& entity) {
            m_renderer->entityRendererManager().renderWithPipeline(cmd, entity, partialTick);
        });
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
    m_player->setCreativeModeInventory();
    m_player->abilities().flying = true;
    spdlog::info("Player created at (8, 50, 8)");

    spdlog::info("Client initialized successfully");
    spdlog::info("Window: {}x{}", m_window.width(), m_window.height());
    spdlog::info("Controls: WASD to move, Space to jump/fly up, Shift to sneak/fly down, mouse to look");
    spdlog::info("Press F to toggle flying, F3 to toggle debug screen, ALT to toggle mouse capture");

    // 初始化调试屏幕
    if (m_renderer->isGuiRendererInitialized()) {
        auto debugResult = m_debugScreen.initialize(&m_renderer->guiRenderer());
        if (debugResult.failed()) {
            spdlog::warn("Failed to initialize debug screen: {}", debugResult.error().toString());
        } else {
            m_debugScreen.setCamera(&m_camera);
            m_debugScreen.setWorld(&m_world);
            m_debugScreen.setEntityManager(&m_world.entityManager());
            m_debugScreen.setNetworkClient(m_networkClient.get());
            m_debugScreen.setRenderDistance(m_settings.renderDistance.get());

            // 设置GPU信息
            auto* context = m_renderer->context();
            if (context != nullptr) {
                DebugGpuInfo gpuInfo;
                gpuInfo.name = context->deviceProperties().deviceName;
                gpuInfo.apiMajorVersion = VK_API_VERSION_MAJOR(context->deviceProperties().apiVersion);
                gpuInfo.apiMinorVersion = VK_API_VERSION_MINOR(context->deviceProperties().apiVersion);
                gpuInfo.driverVersion = std::to_string(VK_API_VERSION_MAJOR(context->deviceProperties().driverVersion)) + "." +
                                        std::to_string(VK_API_VERSION_MINOR(context->deviceProperties().driverVersion)) + "." +
                                        std::to_string(VK_API_VERSION_PATCH(context->deviceProperties().driverVersion));

                // 计算显存
                u64 dedicatedVideoMemory = 0;
                u64 sharedSystemMemory = 0;
                for (u32 i = 0; i < context->memoryProperties().memoryHeapCount; ++i) {
                    const auto& heap = context->memoryProperties().memoryHeaps[i];
                    if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                        dedicatedVideoMemory += heap.size;
                    } else {
                        sharedSystemMemory += heap.size;
                    }
                }
                gpuInfo.dedicatedVideoMB = dedicatedVideoMemory / (1024 * 1024);
                gpuInfo.sharedSystemMB = sharedSystemMemory / (1024 * 1024);

                // 厂商名称
                switch (context->deviceProperties().vendorID) {
                    case 0x10DE: gpuInfo.vendor = "NVIDIA"; break;
                    case 0x1002:
                    case 0x1022: gpuInfo.vendor = "AMD"; break;
                    case 0x8086:
                    case 0x8087: gpuInfo.vendor = "Intel"; break;
                    case 0x13B5: gpuInfo.vendor = "ARM"; break;
                    case 0x1010: gpuInfo.vendor = "Apple"; break;
                    case 0x5143: gpuInfo.vendor = "Qualcomm"; break;
                    default: gpuInfo.vendor = "Unknown"; break;
                }

                m_debugScreen.setGpuInfo(gpuInfo);
                m_debugScreen.setVersion("Minecraft Reborn 0.1.0");
                m_debugScreen.setRendererInfo(gpuInfo.name);
            }

            spdlog::info("Debug screen initialized");
        }

        // 初始化准星渲染器
        auto crosshairResult = m_crosshair.initialize(&m_renderer->guiRenderer());
        if (crosshairResult.failed()) {
            spdlog::warn("Failed to initialize crosshair: {}", crosshairResult.error().toString());
        } else {
            spdlog::info("Crosshair initialized");
        }

        // 初始化HUD渲染器（ItemRenderer已在TridentEngine中初始化）
        if (m_hudRenderer.initialize(&m_renderer->itemRenderer())) {
            spdlog::info("HUD renderer initialized");
        } else {
            spdlog::warn("Failed to initialize HUD renderer");
        }

        // 初始化聊天屏幕
        m_chatScreen.initialize(m_renderer->guiRenderer().font());
        m_chatScreen.setCommandCallback([this](const String& input) {
            handleChatCommand(input);
        });
        spdlog::info("Chat screen initialized");

        // 设置字符输入回调
        m_input.setCharCallback([this](u32 codepoint) {
            if (m_chatScreen.isOpen()) {
                m_chatScreen.onCharInput(codepoint);
                return;
            }

            ScreenManager::instance().onChar(codepoint);
        });

        // 设置键盘事件回调（用于聊天框输入）
        m_input.setKeyEventCallback([this](i32 key, i32 action, i32 mods) {
            if (m_chatScreen.isOpen()) {
                m_chatScreen.onKeyInput(key, action, mods);
                return;
            }

            ScreenManager::instance().onKey(key, 0, action, mods);
            if (action == GLFW_PRESS && !ScreenManager::instance().hasScreen()) {
                captureMouseAfterScreens(m_input, m_mouseCaptured);
            }
        });

        // 设置GUI渲染回调
        m_renderer->setGuiRenderCallback([this]() {
            // 先渲染准星
            m_crosshair.render();
            // 渲染HUD（快捷栏、生命值、饥饿值等）
            if (m_player) {
                if (m_hudRenderer.isVisible()) {
                    m_hudRenderer.render(m_renderer->guiRenderer(), *m_player,
                                         m_player->inventory(),
                                         static_cast<f32>(m_window.width()),
                                         static_cast<f32>(m_window.height()));
                }
            }
            // 渲染聊天屏幕（消息列表和输入框）
            m_chatScreen.render(m_renderer->guiRenderer(),
                                static_cast<f32>(m_window.width()),
                                static_cast<f32>(m_window.height()));
            ScreenManager::instance().render(static_cast<i32>(m_input.mouseX()),
                                             static_cast<i32>(m_input.mouseY()),
                                             0.0f);
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
        MC_TRACE_EVENT("rendering.frame", "Frame");

        // 计算帧时间
        const f64 currentTime = glfwGetTime();
        const f32 deltaTime = static_cast<f32>(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;

        // 处理事件
        {
            MC_TRACE_EVENT("rendering.frame", "HandleEvents");
            handleEvents();
        }

        // 更新
        {
            MC_TRACE_EVENT("rendering.frame", "Update");
            update(deltaTime);
        }

        // 渲染
        {
            MC_TRACE_EVENT("rendering.frame", "Render");
            render();
        }

        // 清理本帧的瞬时输入状态
        m_input.endFrame();

        // 帧计数
        ++m_frameCount;

        // 追踪 FPS
        MC_TRACE_COUNTER("rendering.frame", "FPS", static_cast<i64>(1.0 / deltaTime));

        // 每秒输出一次FPS
        if (m_frameCount % 60 == 0) {
            SPDLOG_TRACE("FPS: {:.1f}, Frame: {}", 1.0f / deltaTime, m_frameCount);
        }
    }

    shutdown();
}

void ClientApplication::handleEvents()
{
    m_window.pollEvents();
    m_input.update();

    // 处理聊天框键盘输入（优先于游戏输入）
    if (m_chatScreen.isOpen()) {
        // 聊天框打开时，只处理聊天相关按键
        // ESC 关闭聊天框
        if (m_input.isKeyJustPressed(GLFW_KEY_ESCAPE)) {
            m_chatScreen.close();
            m_input.setMouseLocked(true);
            m_mouseCaptured = true;
        }
        return;
    }

    if (ScreenManager::instance().hasScreen()) {
        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            ScreenManager::instance().onClick(static_cast<i32>(m_input.mouseX()),
                                              static_cast<i32>(m_input.mouseY()),
                                              GLFW_MOUSE_BUTTON_LEFT);
        }
        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            ScreenManager::instance().onClick(static_cast<i32>(m_input.mouseX()),
                                              static_cast<i32>(m_input.mouseY()),
                                              GLFW_MOUSE_BUTTON_RIGHT);
        }
        if (m_input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
            ScreenManager::instance().onRelease(static_cast<i32>(m_input.mouseX()),
                                                static_cast<i32>(m_input.mouseY()),
                                                GLFW_MOUSE_BUTTON_LEFT);
        }
        if (m_input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
            ScreenManager::instance().onRelease(static_cast<i32>(m_input.mouseX()),
                                                static_cast<i32>(m_input.mouseY()),
                                                GLFW_MOUSE_BUTTON_RIGHT);
        }

        if (!ScreenManager::instance().hasScreen()) {
            captureMouseAfterScreens(m_input, m_mouseCaptured);
        }
        return;
    }

    // 检查ALT键切换鼠标捕获
    if (m_input.isKeyJustPressed(GLFW_KEY_LEFT_ALT) ||
        m_input.isKeyJustPressed(GLFW_KEY_RIGHT_ALT)) {
        toggleMouseCapture();
    }

    // T 键打开聊天框
    if (m_input.isKeyJustPressed(GLFW_KEY_T)) {
        m_chatScreen.open(false);
        if (m_mouseCaptured) {
            m_input.setMouseLocked(false);
            m_mouseCaptured = false;
        }
        return;
    }

    // / 键打开命令框
    if (m_input.isKeyJustPressed(GLFW_KEY_SLASH)) {
        m_chatScreen.open(true);
        if (m_mouseCaptured) {
            m_input.setMouseLocked(false);
            m_mouseCaptured = false;
        }
        return;
    }

    if (m_input.isKeyJustPressed(GLFW_KEY_E) && m_player) {
        releaseMouseForScreen(m_input, m_mouseCaptured);
        ScreenManager::instance().openScreen(
            std::make_unique<InventoryCraftingScreen>(
                std::make_unique<InventoryCraftingMenu>(inventory::PLAYER_CONTAINER_ID, &m_player->inventory())));
        return;
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
        // InputManager 返回 f64 (GLFW)，转换为 f32 用于内部计算
        f32 sensitivity = m_settings.mouseSensitivity.get() * 0.2f;
        f32 deltaYaw = static_cast<f32>(m_input.mouseDeltaX()) * sensitivity;
        f32 deltaPitch = static_cast<f32>(m_input.mouseDeltaY()) * sensitivity;
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

        // 滚轮选择物品栏槽位（scrollDeltaY 返回 f64）
        const f64 scrollDelta = m_input.scrollDeltaY();
        if (scrollDelta != 0.0) {
            i32 selectedSlot = m_player->inventory().getSelectedSlot();
            i32 delta = scrollDelta > 0.0 ? -1 : 1;
            selectedSlot = (selectedSlot + delta + PlayerInventory::HOTBAR_SIZE) % PlayerInventory::HOTBAR_SIZE;
            m_player->inventory().setSelectedSlot(selectedSlot);
            if (m_networkClient && m_networkClient->isLoggedIn()) {
                m_networkClient->sendHotbarSelect(selectedSlot);
            }
        }

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
        mc::Vector3 origin(eyePos.x, eyePos.y, eyePos.z);
        mc::Vector3 direction(forward.x, forward.y, forward.z);
        mc::Ray ray(origin, direction);

        // 执行射线检测（创造模式使用更远的距离）
        mc::RaycastContext context(ray, 5.0f);  // 生存模式5格
        ClientWorldBlockReader blockReader(m_world);
        m_raycastResult = mc::raycastBlocks(context, blockReader);

        // 更新调试屏幕的目标方块
        m_debugScreen.setTargetBlock(&m_raycastResult);
    } else {
        // 没有玩家时清除目标方块
        m_debugScreen.setTargetBlock(nullptr);
    }

    handleBlockInteractionInput(deltaTime);

    // 处理方块放置输入
    handleBlockPlacementInput(deltaTime);

    // 更新世界（根据相机位置加载/卸载区块）
    m_world.update(m_camera.position(), m_settings.renderDistance.get());

    // 更新客户端实体（每tick调用）
    m_world.entityManager().tick();

    // 更新实体动画状态（用于渲染插值）
    constexpr f32 partialTick = 0.0f;  // TODO: 从主循环获取实际的部分tick
    m_world.entityManager().updateAnimations(partialTick);

    // 处理异步网格构建结果
    m_world.processMeshBuildResults(4);  // 每帧最多处理 4 个网格

    // 同步时间到渲染器（驱动天空、太阳、月亮、星空变化）
    // 客户端每帧平滑推进时间，同时在收到服务端同步时纠正
    if (m_renderer) {
        constexpr i64 DAY_LENGTH_TICKS = 24000;

        // 每帧推进时间（无论是否有服务端同步）
        // 这确保天空、太阳、月亮在每帧平滑变化
        m_renderTickAccumulator += deltaTime * 20.0f;
        while (m_renderTickAccumulator >= 1.0f) {
            m_renderTickAccumulator -= 1.0f;
            ++m_renderGameTime;
            m_renderDayTime = (m_renderDayTime + 1) % DAY_LENGTH_TICKS;
        }

        // 当有服务端同步时，逐渐纠正到服务端时间（避免跳变）
        if (m_hasServerTimeSync) {
            const i64 serverDayTime = m_world.dayTime();
            const i64 serverGameTime = m_world.gameTime();

            // 计算时间差（处理 dayTime 循环）
            i64 dayTimeDiff = serverDayTime - m_renderDayTime;
            if (dayTimeDiff > DAY_LENGTH_TICKS / 2) {
                dayTimeDiff -= DAY_LENGTH_TICKS;
            } else if (dayTimeDiff < -DAY_LENGTH_TICKS / 2) {
                dayTimeDiff += DAY_LENGTH_TICKS;
            }

            // 平滑纠正：每帧纠正差值的 1%，避免跳变（TODO:根据用户设定的帧率调整纠正率。CORRECTION_RATE = 1 / 用户设定FPS）
            constexpr f32 CORRECTION_RATE = 0.01f;
            const i64 correction = static_cast<i64>(dayTimeDiff * CORRECTION_RATE);
            if (correction != 0) {
                m_renderDayTime = (m_renderDayTime + correction + DAY_LENGTH_TICKS) % DAY_LENGTH_TICKS;
            }

            // gameTime 同步纠正
            i64 gameTimeDiff = serverGameTime - m_renderGameTime;
            m_renderGameTime += static_cast<i64>(gameTimeDiff * CORRECTION_RATE);
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

    // 关闭性能追踪
    mc::perfetto::PerfettoManager::instance().stopTracing();
    mc::perfetto::PerfettoManager::instance().shutdown();
    spdlog::info("Perfetto tracing stopped");

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
    (void)ClientSettings::ensureSettingsDir("minecraft-reborn");

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
        m_debugScreen.setRenderDistance(value);
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
        if (m_chatScreen.isOpen()) {
            return;
        }

        if (ScreenManager::instance().hasScreen()) {
            ScreenManager::instance().closeScreen();
            captureMouseAfterScreens(m_input, m_mouseCaptured);
            return;
        }

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

    callbacks.onTeleport = [this](f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 /*teleportId*/) {
        if (m_player) {
            // 网络协议使用 f64，内部使用 f32
            m_player->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
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

    callbacks.onPlayerInventory = [this](i32 selectedSlot, const std::vector<ItemStack>& items) {
        if (!m_player) {
            return;
        }

        m_player->inventory().clear();
        const i32 maxSlots = std::min(static_cast<i32>(items.size()), PlayerInventory::TOTAL_SIZE);
        for (i32 slot = 0; slot < maxSlots; ++slot) {
            m_player->inventory().setItem(slot, items[slot]);
        }
        m_player->inventory().setSelectedSlot(selectedSlot);
    };

    callbacks.onOpenContainer = [this](const OpenContainerPacket& packet) {
        if (!m_player) {
            return;
        }

        if (static_cast<ContainerType>(packet.type()) != ContainerType::CraftingTable) {
            spdlog::debug("Unhandled open container type {}", packet.type());
            return;
        }

        auto clickSender = [this](ContainerId containerId, i32 slotIndex, i32 button, ClickAction action,
                                  const ItemStack& cursorItem) {
            if (m_networkClient) {
                m_networkClient->sendContainerClick(
                    ContainerClickPacket(containerId, slotIndex, button, action, cursorItem));
            }
        };

        auto closeSender = [this](ContainerId containerId) {
            if (m_networkClient) {
                m_networkClient->sendCloseContainer(containerId);
            }
        };

        releaseMouseForScreen(m_input, m_mouseCaptured);
        ScreenManager::instance().openScreen(
            std::make_unique<CraftingScreen>(
                std::make_unique<CraftingMenu>(packet.containerId(), &m_player->inventory(), nullptr),
                clickSender,
                closeSender));
    };

    callbacks.onContainerContent = [this](const ContainerContentPacket& packet) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (auto* craftingScreen = dynamic_cast<CraftingScreen*>(screen)) {
            if (craftingScreen->getMenu() != nullptr && craftingScreen->getMenu()->getId() == packet.containerId()) {
                applyContainerContents(craftingScreen->getMenu(), packet.items());
            }
        }
    };

    callbacks.onContainerSlot = [this](const ContainerSlotPacket& packet) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (auto* craftingScreen = dynamic_cast<CraftingScreen*>(screen)) {
            if (craftingScreen->getMenu() != nullptr && craftingScreen->getMenu()->getId() == packet.containerId()) {
                applyContainerSlot(craftingScreen->getMenu(), packet.slotIndex(), packet.item());
            }
        }
    };

    callbacks.onCloseContainer = [this](ContainerId containerId) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (auto* craftingScreen = dynamic_cast<CraftingScreen*>(screen)) {
            if (craftingScreen->getMenu() != nullptr && craftingScreen->getMenu()->getId() == containerId) {
                ScreenManager::instance().closeScreen();
                if (!ScreenManager::instance().hasScreen()) {
                    captureMouseAfterScreens(m_input, m_mouseCaptured);
                }
            }
        }
    };

    // ========== 实体事件回调 ==========
    callbacks.onSpawnMob = [this](u32 entityId, const String& typeId,
                                   f32 x, f32 y, f32 z,
                                   f32 yaw, f32 pitch, f32 headYaw) {
        auto* entity = m_world.entityManager().spawnEntity(
            static_cast<EntityId>(entityId), typeId);
        if (entity) {
            entity->setPosition(x, y, z);
            entity->setRotation(yaw, pitch);
            entity->setHeadRotation(headYaw);
            // spdlog::info("Client received SpawnMob: {} (ID: {}) at ({:.1f}, {:.1f}, {:.1f})",
            //               typeId, entityId, x, y, z);
        }
    };

    callbacks.onSpawnEntity = [this](u32 entityId, const String& typeId,
                                      f32 x, f32 y, f32 z,
                                      f32 yaw, f32 pitch) {
        auto* entity = m_world.entityManager().spawnEntity(
            static_cast<EntityId>(entityId), typeId);
        if (entity) {
            entity->setPosition(x, y, z);
            entity->setRotation(yaw, pitch);
            // spdlog::info("Client received SpawnEntity: {} (ID: {}) at ({:.1f}, {:.1f}, {:.1f})",
            //               typeId, entityId, x, y, z);
        }
    };

    callbacks.onEntityDestroy = [this](const std::vector<u32>& entityIds) {
        for (u32 id : entityIds) {
            m_world.entityManager().removeEntity(static_cast<EntityId>(id));
            spdlog::debug("Destroyed entity {}", id);
        }
    };

    callbacks.onEntityMove = [this](u32 entityId, f32 dx, f32 dy, f32 dz) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            entity->setTargetPosition(
                entity->x() + dx,
                entity->y() + dy,
                entity->z() + dz);
        }
    };

    callbacks.onEntityTeleport = [this](u32 entityId, f32 x, f32 y, f32 z,
                                         f32 yaw, f32 pitch) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            entity->setPosition(x, y, z);
            entity->setRotation(yaw, pitch);
        }
    };

    callbacks.onEntityVelocity = [this](u32 entityId, i16 vx, i16 vy, i16 vz) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            // 速度单位转换: 1/8000 block/tick -> block/tick
            entity->setVelocity(
                static_cast<f32>(vx) / 8000.0f,
                static_cast<f32>(vy) / 8000.0f,
                static_cast<f32>(vz) / 8000.0f);
        }
    };

    callbacks.onEntityHeadLook = [this](u32 entityId, f32 headYaw) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            entity->setHeadRotation(headYaw);
        }
    };

    callbacks.onEntityAnimation = [this](u32 entityId, u8 /*animation*/) {
        // TODO: 处理实体动画（挥手等）
        (void)entityId;
    };

    callbacks.onEntityStatus = [this](u32 entityId, u8 /*status*/) {
        // TODO: 处理实体状态（受伤、死亡等）
        (void)entityId;
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

void ClientApplication::handleBlockInteractionInput(f32 deltaTime)
{
    auto abortBreakingBlock = [this]() {
        if (!m_breakingBlockActive) {
            return;
        }

        sendBlockInteraction(network::BlockInteractionAction::AbortDestroyBlock,
                             m_breakingBlockPos,
                             m_breakingBlockFace);
        m_breakingBlockActive = false;
        m_breakingBlockProgress = 0.0f;
        m_breakingBlockFace = Direction::None;
    };

    if (!m_mouseCaptured || !m_player) {
        abortBreakingBlock();
        return;
    }

    const bool attackPressed = m_input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool attackJustPressed = m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool hasTargetBlock = m_raycastResult.isHit();

    if (!attackPressed) {
        abortBreakingBlock();
        return;
    }

    if (!hasTargetBlock) {
        abortBreakingBlock();
        return;
    }

    const BlockPos currentTargetPos = m_raycastResult.blockPos();
    const Direction currentTargetFace = m_raycastResult.face();
    const bool targetChanged = !m_breakingBlockActive ||
        currentTargetPos != m_breakingBlockPos ||
        currentTargetFace != m_breakingBlockFace;

    if (targetChanged) {
        abortBreakingBlock();
        m_breakingBlockPos = currentTargetPos;
        m_breakingBlockFace = currentTargetFace;
        m_breakingBlockActive = true;
        m_breakingBlockProgress = 0.0f;
        sendBlockInteraction(network::BlockInteractionAction::StartDestroyBlock,
                             m_breakingBlockPos,
                             m_breakingBlockFace);
    }

    const BlockState* targetState = m_world.getBlockState(
        m_breakingBlockPos.x,
        m_breakingBlockPos.y,
        m_breakingBlockPos.z);
    if (targetState == nullptr || targetState->isAir() || targetState->hardness() < 0.0f) {
        abortBreakingBlock();
        return;
    }

    if (m_player->gameMode() == GameMode::Creative || targetState->hardness() == 0.0f) {
        if (!attackJustPressed) {
            return;
        }

        sendBlockInteraction(network::BlockInteractionAction::StopDestroyBlock,
                             m_breakingBlockPos,
                             m_breakingBlockFace);
        m_breakingBlockActive = false;
        m_breakingBlockProgress = 0.0f;
        m_breakingBlockFace = Direction::None;
        return;
    }

    m_breakingBlockProgress += deltaTime * constants::TICK_RATE *
        calculateBlockBreakingDelta(*m_player, *targetState);

    if (m_breakingBlockProgress >= 1.0f) {
        sendBlockInteraction(network::BlockInteractionAction::StopDestroyBlock,
                             m_breakingBlockPos,
                             m_breakingBlockFace);
        m_breakingBlockActive = false;
        m_breakingBlockProgress = 0.0f;
        m_breakingBlockFace = Direction::None;
    }
}

void ClientApplication::handleBlockPlacementInput(f32 deltaTime)
{
    m_placeCooldown = std::max(0.0f, m_placeCooldown - deltaTime);

    if (!m_mouseCaptured || !m_player) {
        return;
    }

    // 检查右键是否刚刚按下
    const bool usePressed = m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT);
    if (!usePressed) {
        return;
    }

    // 检查放置冷却
    if (m_placeCooldown > 0.0f) {
        return;
    }

    // 检查射线是否击中方块
    if (m_raycastResult.isMiss()) {
        return;
    }

    // 计算击中点相对坐标
    BlockPos pos = m_raycastResult.blockPos();
    Direction face = m_raycastResult.face();
    Vector3 hitPos = m_raycastResult.hitPosition();
    Vector3 blockPosFloat(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
    Vector3 relativeHit = hitPos - blockPosFloat;  // 转换为方块内相对坐标

    // 发送放置包
    sendBlockPlacement(pos, face, relativeHit);
    m_placeCooldown = PLACE_COOLDOWN_TIME;
}

void ClientApplication::sendBlockPlacement(const BlockPos& pos, Direction face, const Vector3& hitPos)
{
    if (!m_networkClient || !m_networkClient->isLoggedIn()) {
        spdlog::info("[Place] Skip sending block placement because client is not logged in");
        return;
    }

    spdlog::info("[Place] Send placement pos=({}, {}, {}) face={} hit=({:.2f}, {:.2f}, {:.2f})",
                 pos.x, pos.y, pos.z,
                 static_cast<i32>(face),
                 hitPos.x, hitPos.y, hitPos.z);

    m_networkClient->sendBlockPlacement(pos.x, pos.y, pos.z, face,
                                        hitPos.x, hitPos.y, hitPos.z);
}

void ClientApplication::sendBlockInteraction(network::BlockInteractionAction action,
                                             const BlockPos& pos,
                                             Direction face)
{
    if (!m_networkClient || !m_networkClient->isLoggedIn()) {
        spdlog::debug("[Mining] Skip sending block interaction because client is not logged in");
        return;
    }

    // spdlog::info("[Mining] Send action={} pos=({}, {}, {}) face={}",
    //              static_cast<i32>(action),
    //              pos.x,
    //              pos.y,
    //              pos.z,
    //              static_cast<i32>(face));

    m_networkClient->sendBlockInteraction(action, pos.x, pos.y, pos.z, face);
}

void ClientApplication::sendPlayerPosition()
{
    if (!m_networkClient || !m_networkClient->isLoggedIn() || !m_player) {
        return;
    }

    const auto& pos = m_player->position();

    // 检查是否需要发送（位置或旋转变化）
    bool positionChanged =
        std::abs(pos.x - m_lastSentX) > 0.001f ||
        std::abs(pos.y - m_lastSentY) > 0.001f ||
        std::abs(pos.z - m_lastSentZ) > 0.001f;

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
    // 1. 创建 ResourceManager 并首先添加内置资源包（最低优先级）
    m_resourceManager = std::make_unique<ResourceManager>();

    // 添加原版内置资源包（提供基础模型如 cube_all, cube_column 等）
    auto vanillaPack = VanillaResources::createResourcePack();
    auto vanillaResult = vanillaPack->initialize();
    if (vanillaResult.success()) {
        (void)m_resourceManager->addResourcePack(std::move(vanillaPack));
        spdlog::info("Added vanilla built-in resource pack");
    } else {
        spdlog::warn("Failed to initialize vanilla resource pack: {}", vanillaResult.error().toString());
    }

    // 2. 扫描资源包目录
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

    // 3. 从设置加载资源包配置
    m_resourcePackList.loadFromSettings(m_settings.resourcePacks);

    // 4. 添加启用的资源包（外部资源包优先级高于内置）
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

    // 首先添加内置资源包
    auto vanillaPack = VanillaResources::createResourcePack();
    auto vanillaResult = vanillaPack->initialize();
    if (vanillaResult.success()) {
        (void)m_resourceManager->addResourcePack(std::move(vanillaPack));
    }

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

        if (m_renderer) {
            auto atlasUpdateResult = m_renderer->updateTextureAtlas(atlasResult.value());
            if (atlasUpdateResult.failed()) {
                spdlog::error("Failed to update renderer texture atlas after reload: {}",
                              atlasUpdateResult.error().toString());
            }
        }

        m_world.forEachChunk([](const ChunkId&, ClientChunk& chunk) {
            chunk.needsMeshUpdate = true;
        });
        spdlog::info("Marked loaded chunks dirty after resource reload");
    }
}

void ClientApplication::handleChatCommand(const String& input)
{
    if (input.empty()) {
        return;
    }

    // 添加到聊天历史
    m_chatScreen.addMessage(input, 0xFFFFFFFF);

    // 检查是否为命令（以 / 开头）
    if (input[0] == '/') {
        String command = input.substr(1);

        // TODO: 连接到 CommandDispatcher 执行命令
        // 目前只显示命令被接收
        spdlog::info("Chat command received: {}", command);

        // 发送到服务端
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else {
            // 本地回显
            m_chatScreen.addSystemMessage("Command executed locally (not connected to server)");
        }
    } else {
        // 普通聊天消息，发送到服务端
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else {
            // 本地回显
            m_chatScreen.addSystemMessage("Message sent locally (not connected to server)");
        }
    }
}

} // namespace mc::client
