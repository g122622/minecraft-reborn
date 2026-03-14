#include "DebugScreen.hpp"
#include "../../../common/entity/Entity.hpp"
#include "../../../common/resource/ResourceLocation.hpp"
#include "../../../common/util/Direction.hpp"
#include "../../../common/util/PlatformInfo.hpp"
#include "../../../common/world/biome/BiomeRegistry.hpp"
#include "../../../common/world/block/Block.hpp"
#include "../../../common/world/time/GameTime.hpp"
#include "../../network/NetworkClient.hpp"
#include "../../renderer/sky/CelestialCalculations.hpp"
#include "../../world/ClientWorld.hpp"
#include "../../world/entity/ClientEntityManager.hpp"
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>

namespace mc::client {

DebugScreen::DebugScreen()
    : m_version("Minecraft Reborn 0.1.0")
    , m_rendererInfo("Vulkan") {
    // 初始化时获取一次系统信息
    m_cpuInfo = util::PlatformInfo::getCpuInfo();
}

Result<void> DebugScreen::initialize(GuiRenderer* guiRenderer) {
    if (guiRenderer == nullptr) {
        return Error(ErrorCode::NullPointer, "GuiRenderer is null");
    }

    m_guiRenderer = guiRenderer;
    m_initialized = true;

    // 初始化时获取CPU信息
    m_cpuInfo = util::PlatformInfo::getCpuInfo();

    return {};
}

void DebugScreen::update(f32 deltaTime) {
    if (!m_visible) return;

    updateFps(deltaTime);
    updateSystemInfo();
    buildLeftDebugText();
    buildRightDebugText();
}

void DebugScreen::render() {
    if (!m_visible || m_guiRenderer == nullptr) return;

    f32 screenWidth = static_cast<f32>(m_guiRenderer->screenWidth());

    // 渲染左侧面板
    renderPanel(m_leftLines, 2.0f, false);

    // 渲染右侧面板
    renderPanel(m_rightLines, screenWidth - 2.0f, true);
}

void DebugScreen::updateFps(f32 deltaTime) {
    m_frameTime = deltaTime;
    m_fpsAccumulator += deltaTime;
    m_frameCount++;

    m_fpsUpdateTimer += deltaTime;
    if (m_fpsUpdateTimer >= FPS_UPDATE_INTERVAL) {
        m_fps = static_cast<f32>(m_frameCount) / m_fpsAccumulator;
        m_fpsAccumulator = 0.0f;
        m_frameCount = 0;
        m_fpsUpdateTimer = 0.0f;
    }
}

void DebugScreen::updateSystemInfo() {
    // 每秒更新一次系统信息
    m_systemInfoTimer += m_frameTime;
    if (m_systemInfoTimer >= SYSTEM_INFO_UPDATE_INTERVAL) {
        m_systemInfoTimer = 0.0f;
        m_memoryInfo = util::PlatformInfo::getMemoryInfo();
    }
}

void DebugScreen::buildLeftDebugText() {
    m_leftLines.clear();

    std::ostringstream oss;

    // ========== 版本信息 ==========
    // MC格式: "Minecraft 1.16.5 (1.16.5/vanilla)"
    oss.str("");
    oss << m_version << " (" << m_version << "/" << m_rendererInfo << ")";
    m_leftLines.push_back(oss.str());

    // ========== FPS 信息 ==========
    // MC格式: "XX fps T: XX ms"
    oss.str("");
    oss << std::fixed << std::setprecision(0) << m_fps << " fps"
        << " T: " << std::setprecision(1) << (m_frameTime * 1000.0f) << " ms";
    m_leftLines.push_back(oss.str());

    // ========== 服务器信息 ==========
    // MC格式: "Integrated server @ XX ms ticks, XX tx, XX rx"
    if (m_networkClient != nullptr) {
        oss.str("");
        oss << "Integrated server @ " << std::fixed << std::setprecision(0)
            << m_serverTickTimeMs << " ms ticks, "
            << m_networkClient->packetsSent() << " tx, "
            << m_networkClient->packetsReceived() << " rx";
        m_leftLines.push_back(oss.str());
    } else {
        m_leftLines.push_back("Server: local (integrated)");
    }

    // ========== 渲染统计 ==========
    // MC格式: "C: XX/XX (s) D: XX, L: XX, E: XX"
    // C: 已渲染区块/总区块数 (s: 实体渲染)
    // D: 渲染距离
    // L: 灯光更新
    // E: 实体数
    if (m_world != nullptr) {
        oss.str("");
        oss << "ChunkCount: 0/" << m_world->chunkCount()
            << ", RenderDistance: " << m_renderDistance
            << ", LightUpdates: 0"
            << ", EntityCount: " << (m_entityManager ? m_entityManager->entityCount() : 0);
        m_leftLines.push_back(oss.str());
    }

    // ========== 实体统计 ==========
    // MC格式: "P: XX. T: XX"  (粒子数和实体数)
    oss.str("");
    oss << "Particles: " << m_particleCount << ", Entities: " << (m_entityManager ? m_entityManager->entityCount() : 0);
    m_leftLines.push_back(oss.str());

    // ========== 维度信息 ==========
    m_leftLines.push_back("Dimension: minecraft:overworld");

    // ========== 强制加载区块 ==========
    // MC格式: "minecraft:overworld FC: 0"
    m_leftLines.push_back("minecraft:overworld ForcedChunks: 0");

    // 空行
    m_leftLines.push_back("");

    // ========== 位置信息 ==========
    if (m_camera != nullptr) {
        const auto& pos = m_camera->position();

        // XYZ: 浮点坐标
        // MC格式: "XYZ: XX.XXX / XX.XXXXX / XX.XXX"
        oss.str("");
        oss << "XYZ: " << std::fixed << std::setprecision(3)
            << pos.x << " / " << pos.y << " / " << pos.z;
        m_leftLines.push_back(oss.str());

        // Block: 整数坐标
        i32 blockX = static_cast<i32>(std::floor(pos.x));
        i32 blockY = static_cast<i32>(std::floor(pos.y));
        i32 blockZ = static_cast<i32>(std::floor(pos.z));
        oss.str("");
        oss << "Block: " << blockX << " " << blockY << " " << blockZ;
        m_leftLines.push_back(oss.str());

        // Chunk: 区块内相对坐标和区块坐标
        // MC格式: "Chunk: X Y Z in CX CY CZ"
        i32 chunkX = blockX >> 4;
        i32 chunkY = blockY >> 4;
        i32 chunkZ = blockZ >> 4;
        i32 relX = blockX & 15;
        i32 relY = blockY & 15;
        i32 relZ = blockZ & 15;
        oss.str("");
        oss << "Chunk: " << relX << " " << relY << " " << relZ
            << " in " << chunkX << " " << chunkY << " " << chunkZ;
        m_leftLines.push_back(oss.str());

        // Facing: 方向和角度
        const auto& rot = m_camera->rotation();
        auto [dirName, dirDesc] = getFacingDirection(rot.y);
        oss.str("");
        oss << "Facing: " << dirName << " (" << dirDesc << ")"
            << " (" << std::fixed << std::setprecision(1)
            << rot.y << " / " << rot.x << ")";
        m_leftLines.push_back(oss.str());

        // ========== 光照信息 ==========
        if (m_world != nullptr) {
            // 检查是否在已加载区块内
            bool blockLoaded = m_world->getChunkAt(chunkX, chunkZ) != nullptr;

            if (blockLoaded) {
                // Client Light: 总光照 (天空, 方块)
                u8 skyLight = m_world->getSkyLight(blockX, blockY, blockZ);
                u8 blockLight = m_world->getBlockLight(blockX, blockY, blockZ);
                u8 totalLight = std::max(skyLight, blockLight);
                oss.str("");
                oss << "Client Light: " << static_cast<i32>(totalLight)
                    << " (" << static_cast<i32>(skyLight) << " sky, "
                    << static_cast<i32>(blockLight) << " block)";
                m_leftLines.push_back(oss.str());

                // Server Light: 服务端光照（如果有服务端数据）
                // 当前显示为占位符
                m_leftLines.push_back("Server Light: (?? sky, ?? block)");
            } else {
                m_leftLines.push_back("Outside of world...");
            }

            // ========== 高度图信息 ==========
            // MC格式: "CH S: XX O: XX M: XX ML: XX"
            if (m_heightmapInfo.available && blockLoaded) {
                oss.str("");
                oss << "CH S:" << m_heightmapInfo.worldSurface
                    << " O:" << m_heightmapInfo.oceanFloor
                    << " M:" << m_heightmapInfo.motionBlocking
                    << " ML:" << m_heightmapInfo.motionBlockingNoLeaves;
                m_leftLines.push_back(oss.str());

                // SH: 服务端高度图 (如果有)
                oss.str("");
                oss << "SH S:?? O:?? M:?? ML:??";
                m_leftLines.push_back(oss.str());
            }

            // ========== 生物群系 ==========
            if (blockLoaded && blockY >= 0 && blockY < 256) {
                if (const Biome* biome = m_world->getBiomeAtBlock(blockX, blockY, blockZ)) {
                    oss.str("");
                    oss << "Biome: minecraft:" << biome->name();
                    m_leftLines.push_back(oss.str());

                    // 气候信息
                    oss.str("");
                    oss << "Climate: T=" << std::fixed << std::setprecision(2)
                        << biome->temperature()
                        << " H=" << biome->humidity()
                        << " C=" << biome->continentalness();
                    m_leftLines.push_back(oss.str());
                }

                // ========== 本地难度 ==========
                // 直接从 m_world 获取时间信息
                i64 gameTime = m_world->gameTime();
                i64 dayCount = gameTime / 24000; // TimeConstants::TICKS_PER_DAY
                i32 moonPhase = CelestialCalculations::calculateMoonPhase(gameTime);
                // 本地难度计算需要 DifficultyManager，当前使用简化公式
                f32 localDifficulty = 0.0f; // TODO: 需要 DifficultyManager 实现
                oss.str("");
                oss << "Local Difficulty: " << std::fixed << std::setprecision(2)
                    << localDifficulty << " // " << localDifficulty
                    << " (Day " << dayCount << ", Moon " << moonPhase << ")";
                m_leftLines.push_back(oss.str());
            }
        } else {
            m_leftLines.push_back("Outside of world...");
        }

        // ========== Spawn统计 ==========
        // MC格式: "SC: XX, M: XX C: XX A: XX W: XX N: XX"
        if (m_spawnStats.available) {
            oss.str("");
            oss << "SC: " << m_spawnStats.spawnableChunkCount
                << ", M:" << m_spawnStats.categoryCounts[0]  // Monster
                << " C:" << m_spawnStats.categoryCounts[1]   // Creature
                << " A:" << m_spawnStats.categoryCounts[2]   // Ambient
                << " W:" << m_spawnStats.categoryCounts[3]   // WaterCreature
                << " N:" << m_spawnStats.categoryCounts[4];  // NetherCreature
            m_leftLines.push_back(oss.str());
        }

        // ========== Shader信息 ==========
        if (!m_activeShader.empty()) {
            oss.str("");
            oss << "Shader: " << m_activeShader;
            m_leftLines.push_back(oss.str());
        }

        // ========== 声音信息 ==========
        // MC格式: "XX/XX sounds (Mood XX%)"
        oss.str("");
        oss << m_activeSounds << "/" << m_activeSounds << " sounds"
            << " (Mood " << static_cast<i32>(m_moodPercentage * 100.0f) << "%)";
        m_leftLines.push_back(oss.str());

    } else {
        m_leftLines.push_back("No camera");
    }

    // ========== 帮助提示 ==========
    m_leftLines.push_back("");
    m_leftLines.push_back("Debug: Pie [shift]: hidden FPS + TPS [alt]: hidden");
    m_leftLines.push_back("For help: press F3 + Q");
}

void DebugScreen::buildRightDebugText() {
    m_rightLines.clear();

    std::ostringstream oss;

    // ========== 语言/编译器信息 ==========
    // MC格式: "Java: 1.8.0_xx 64bit"
    oss.str("");
    oss << "C++17 " << (util::PlatformInfo::is64BitSystem() ? "64bit" : "32bit");
    m_rightLines.push_back(oss.str());

    // ========== 内存信息 ==========
    // MC格式: "Mem: XX% XXX/XXXMB"
    oss.str("");
    oss << "Mem: " << std::setw(3) << m_memoryInfo.usagePercent << "% "
        << std::setw(4) << m_memoryInfo.processUsedMB << "/"
        << std::setw(4) << m_memoryInfo.totalPhysicalMB << "MB";
    m_rightLines.push_back(oss.str());

    // Allocated
    oss.str("");
    f32 allocatedPercent = m_memoryInfo.totalPhysicalMB > 0
        ? (static_cast<f32>(m_memoryInfo.processUsedMB) / m_memoryInfo.totalPhysicalMB) * 100.0f
        : 0.0f;
    oss << "Allocated: " << std::setw(3) << static_cast<i32>(allocatedPercent) << "%";
    m_rightLines.push_back(oss.str());

    m_rightLines.push_back("");

    // ========== CPU 信息 ==========
    // MC格式: "CPU: XXX"
    oss.str("");
    if (!m_cpuInfo.brand.empty()) {
        // 截断过长的CPU名称
        String cpuName = m_cpuInfo.brand;
        if (cpuName.length() > 40) {
            cpuName = cpuName.substr(0, 37) + "...";
        }
        oss << "CPU: " << cpuName;
    } else if (!m_cpuInfo.vendor.empty()) {
        oss << "CPU: " << m_cpuInfo.vendor << " " << m_cpuInfo.coreCount << " cores";
    } else {
        oss << "CPU: " << m_cpuInfo.coreCount << " cores";
    }
    m_rightLines.push_back(oss.str());

    m_rightLines.push_back("");

    // ========== 显示器信息 ==========
    // MC格式: "Display: XXXXxXXXX (NVIDIA/AMD/Intel)"
    if (m_guiRenderer != nullptr) {
        oss.str("");
        oss << "Display: " << static_cast<i32>(m_guiRenderer->screenWidth())
            << "x" << static_cast<i32>(m_guiRenderer->screenHeight());
        if (m_gpuInfoSet && !m_gpuInfo.vendor.empty()) {
            oss << " (" << m_gpuInfo.vendor << ")";
        }
        m_rightLines.push_back(oss.str());
    } else {
        m_rightLines.push_back("Display: Unknown");
    }

    // GPU Renderer
    if (m_gpuInfoSet && !m_gpuInfo.name.empty()) {
        m_rightLines.push_back(m_gpuInfo.name);
    } else {
        m_rightLines.push_back(m_rendererInfo);
    }

    // Vulkan版本
    if (m_gpuInfoSet) {
        oss.str("");
        oss << "Vulkan " << m_gpuInfo.apiMajorVersion << "." << m_gpuInfo.apiMinorVersion;
        if (!m_gpuInfo.driverVersion.empty()) {
            oss << " (Driver: " << m_gpuInfo.driverVersion << ")";
        }
        m_rightLines.push_back(oss.str());
    } else {
        m_rightLines.push_back("Vulkan 1.x");
    }

    // ========== 目标方块信息 ==========
    if (m_targetBlock != nullptr && !m_targetBlock->isMiss()) {
        const auto& blockPos = m_targetBlock->blockPos();

        m_rightLines.push_back("");
        // MC格式: "Targeted Block: X, Y, Z" (带下划线)
        oss.str("");
        oss << "Targeted Block: " << blockPos.x << ", " << blockPos.y << ", " << blockPos.z;
        m_rightLines.push_back(oss.str());

        // 方块ID
        if (m_world != nullptr) {
            const BlockState* state = m_world->getBlockState(blockPos.x, blockPos.y, blockPos.z);
            if (state != nullptr) {
                const ResourceLocation& loc = state->blockLocation();
                oss.str("");
                oss << loc.toString();
                m_rightLines.push_back(oss.str());

                // 显示方块属性
                String stateStr = state->toString();
                size_t bracketPos = stateStr.find('[');
                if (bracketPos != String::npos && stateStr.back() == ']') {
                    String props = stateStr.substr(bracketPos + 1, stateStr.length() - bracketPos - 2);
                    if (!props.empty()) {
                        std::istringstream propStream(props);
                        String prop;
                        while (std::getline(propStream, prop, ',')) {
                            m_rightLines.push_back("  " + prop);
                        }
                    }
                }
            }
        }
    }

    // ========== 目标流体信息 ==========
    // TODO: 流体系统实现后添加
    // 参考 MC: "Targeted Fluid: minecraft:water[level=0]"

    // ========== 目标实体信息 ==========
    if (m_entityManager != nullptr && m_camera != nullptr) {
        const auto& pos = m_camera->position();
        auto entityIds = m_entityManager->getEntitiesInRange(pos.x, pos.y, pos.z, 5.0f);
        if (!entityIds.empty()) {
            m_rightLines.push_back("");
            oss.str("");
            oss << "Targeted Entity";  // MC格式，带下划线
            m_rightLines.push_back(oss.str());

            // 显示前几个实体类型
            i32 count = 0;
            for (EntityId id : entityIds) {
                if (count >= 3) break;
                const ClientEntity* entity = m_entityManager->getEntity(id);
                if (entity != nullptr) {
                    oss.str("");
                    oss << "  " << entity->typeId();
                    m_rightLines.push_back(oss.str());
                    count++;
                }
            }
        }
    }
}

std::pair<std::string, std::string> DebugScreen::getFacingDirection(f32 yaw) const {
    // 规范化偏航角到 [-180, 180]
    while (yaw > 180.0f) yaw -= 360.0f;
    while (yaw < -180.0f) yaw += 360.0f;

    // 根据偏航角确定方向
    // MC方向：South=0, West=90, North=180, East=-90 (或270)
    if (yaw >= -45.0f && yaw < 45.0f) {
        return {"South", "Towards positive Z"};
    } else if (yaw >= 45.0f && yaw < 135.0f) {
        return {"West", "Towards negative X"};
    } else if (yaw >= 135.0f || yaw < -135.0f) {
        return {"North", "Towards negative Z"};
    } else { // yaw >= -135.0f && yaw < -45.0f
        return {"East", "Towards positive X"};
    }
}

std::string DebugScreen::getDirectionName(f32 yaw) const {
    auto [name, desc] = getFacingDirection(yaw);
    return name;
}

void DebugScreen::renderPanel(const std::vector<std::string>& lines, f32 startX, bool alignRight) {
    if (lines.empty()) return;

    f32 lineHeight = static_cast<f32>(m_guiRenderer->getFontHeight() + 2);
    f32 screenWidth = static_cast<f32>(m_guiRenderer->screenWidth());
    f32 y = 2.0f;

    // 计算最大宽度
    f32 maxWidth = 0.0f;
    for (const auto& line : lines) {
        if (!line.empty()) {
            f32 width = static_cast<f32>(m_guiRenderer->getTextWidth(line));
            maxWidth = std::max(maxWidth, width);
        }
    }

    // 计算背景位置
    f32 bgX = alignRight ? (screenWidth - maxWidth - 12.0f) : 1.0f;
    f32 bgY = 1.0f;
    f32 bgWidth = maxWidth + 10.0f;
    f32 bgHeight = lineHeight * static_cast<f32>(lines.size()) + 4.0f;

    // 绘制半透明背景
    m_guiRenderer->fillRect(bgX, bgY, bgWidth, bgHeight, 0xA0303030);

    // 绘制文本
    for (const auto& line : lines) {
        if (!line.empty()) {
            f32 x = startX;
            if (alignRight) {
                x = screenWidth - static_cast<f32>(m_guiRenderer->getTextWidth(line)) - 4.0f;
            }
            m_guiRenderer->drawText(line, x, y, COLOR_WHITE, true);
        }
        y += lineHeight;
    }
}

} // namespace mc::client
