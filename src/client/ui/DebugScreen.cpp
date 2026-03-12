#include "DebugScreen.hpp"
#include "../world/ClientWorld.hpp"
#include "../world/entity/ClientEntityManager.hpp"
#include "../network/NetworkClient.hpp"
#include "../../common/world/block/Block.hpp"
#include "../../common/world/biome/BiomeRegistry.hpp"
#include "../../common/resource/ResourceLocation.hpp"
#include "../../common/util/Direction.hpp"
#include "../../common/entity/Entity.hpp"
#include "../../common/world/time/GameTime.hpp"
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>

namespace mr::client {

DebugScreen::DebugScreen()
    : m_version("Minecraft Reborn 0.1.0")
    , m_rendererInfo("Vulkan") {
}

Result<void> DebugScreen::initialize(GuiRenderer* guiRenderer) {
    if (guiRenderer == nullptr) {
        return Error(ErrorCode::NullPointer, "GuiRenderer is null");
    }

    m_guiRenderer = guiRenderer;
    m_initialized = true;
    return {};
}

void DebugScreen::update(f32 deltaTime) {
    if (!m_visible) return;

    updateFps(deltaTime);
    buildLeftDebugText();
    buildRightDebugText();
    // 注：logColumnBlocks() 仅在调试时启用
    // logColumnBlocks();
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

void DebugScreen::buildLeftDebugText() {
    m_leftLines.clear();

    std::ostringstream oss;

    // ========== 版本信息 ==========
    // Minecraft 1.16.5 格式: "Minecraft 1.16.5 (1.16.5/vanilla)"
    oss.str("");
    oss << m_version << " (" << m_version << "/" << m_rendererInfo << ")";
    m_leftLines.push_back(oss.str());

    // ========== FPS 信息 ==========
    // MC格式: "FPS: XX T: XX ms"
    oss.str("");
    oss << std::fixed << std::setprecision(0) << m_fps << " fps"
        << " T: " << std::setprecision(1) << (m_frameTime * 1000.0f) << " ms";
    m_leftLines.push_back(oss.str());

    // ========== 服务器信息 ==========
    // MC格式: "Integrated server @ XX ms ticks, XX tx, XX rx"
    if (m_networkClient != nullptr) {
        oss.str("");
        oss << "Server: " << m_networkClient->packetsSent() << " tx, "
            << m_networkClient->packetsReceived() << " rx"
            << " ping: " << m_networkClient->ping() << "ms";
        m_leftLines.push_back(oss.str());
    } else {
        m_leftLines.push_back("Server: local (integrated)");
    }

    // ========== 渲染统计 ==========
    // MC格式: "C: XX/XX (s) D: XX, L: XX, E: XX"
    // 这里简化为区块计数
    if (m_world != nullptr) {
        oss.str("");
        oss << "Chunks: " << m_world->chunkCount()
            << " Render: " << m_world->renderDistance();
        m_leftLines.push_back(oss.str());
    }

    // ========== 实体统计 ==========
    // MC格式: "P: XX. T: XX"  (粒子数和实体数)
    if (m_entityManager != nullptr) {
        oss.str("");
        oss << "P: 0. T: " << m_entityManager->entityCount();
        m_leftLines.push_back(oss.str());
    }

    // ========== 维度信息 ==========
    m_leftLines.push_back("Dimension: minecraft:overworld");

    // ========== 强制加载区块 ==========
    // MC格式: "minecraft:overworld FC: 0"
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
            // Client Light: 总光照 (天空, 方块)
            u8 skyLight = m_world->getSkyLight(blockX, blockY, blockZ);
            u8 blockLight = m_world->getBlockLight(blockX, blockY, blockZ);
            u8 totalLight = std::max(skyLight, blockLight);
            oss.str("");
            oss << "Client Light: " << static_cast<i32>(totalLight)
                << " (" << static_cast<i32>(skyLight) << " sky, "
                << static_cast<i32>(blockLight) << " block)";
            m_leftLines.push_back(oss.str());

            // Server Light: 服务端光照（占位）
            m_leftLines.push_back("Server Light: (?? sky, ?? block)");
        }

        // ========== 生物群系 ==========
        if (const Biome* biome = m_world != nullptr ? m_world->getBiomeAtBlock(blockX, blockY, blockZ) : nullptr) {
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
        if (m_gameTime != nullptr) {
            // MC格式: "Local Difficulty: X.XX // X.XX (Day XXXX)"
            i64 dayCount = m_gameTime->dayCount();
            f32 localDifficulty = 0.0f; // TODO: 计算实际难度
            oss.str("");
            oss << "Local Difficulty: " << std::fixed << std::setprecision(2)
                << localDifficulty << " // " << localDifficulty
                << " (Day " << dayCount << ")";
            m_leftLines.push_back(oss.str());
        }
    } else {
        m_leftLines.push_back("No camera");
    }

    // ========== 帮助提示 ==========
    m_leftLines.push_back("");
    m_leftLines.push_back("For help: press F3 + Q");
}

void DebugScreen::buildRightDebugText() {
    m_rightLines.clear();

    std::ostringstream oss;

    // ========== Java 信息 ==========
    // MC格式: "Java: 1.8.0_xx 64bit"
    m_rightLines.push_back("C++17 (64bit)");

    // ========== 内存信息 ==========
    // MC格式: "Mem: XX% XXX/XXXMB"
    // 获取内存信息
    // 注意：C++没有直接的方法获取进程内存，这里用估算
    // TODO: 使用平台特定API获取更准确的内存信息
    oss.str("");
    oss << "Mem: " << std::setw(3) << "?" << "% " << std::setw(3) << "?/" << std::setw(3) << "?MB";
    m_rightLines.push_back(oss.str());

    // Allocated
    oss.str("");
    oss << "Allocated: " << std::setw(3) << "?%";
    m_rightLines.push_back(oss.str());

    m_rightLines.push_back("");

    // ========== CPU 信息 ==========
    // MC格式: "CPU: XXX"
    m_rightLines.push_back("CPU: Unknown");

    m_rightLines.push_back("");

    // ========== 显示器信息 ==========
    // MC格式: "Display: XXXXxXXXX (NVIDIA/AMD/Intel)"
    if (m_guiRenderer != nullptr) {
        oss.str("");
        oss << "Display: " << static_cast<i32>(m_guiRenderer->screenWidth())
            << "x" << static_cast<i32>(m_guiRenderer->screenHeight());
        m_rightLines.push_back(oss.str());
    } else {
        m_rightLines.push_back("Display: Unknown");
    }

    // GPU Renderer
    m_rightLines.push_back(m_rendererInfo);

    // Vulkan版本
    m_rightLines.push_back("Vulkan 1.x");

    // ========== 目标方块信息 ==========
    if (m_targetBlock != nullptr && !m_targetBlock->isMiss()) {
        const auto& blockPos = m_targetBlock->blockPos();

        m_rightLines.push_back("");
        // MC格式: "Targeted Block: X, Y, Z"
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

                // TODO: 显示方块属性
                // for (const auto& prop : state->properties()) {
                //     oss.str("");
                //     oss << "  " << prop.name() << ": " << prop.valueString();
                //     m_rightLines.push_back(oss.str());
                // }
            }
        }
    }

    // ========== 目标流体信息 ==========
    // TODO: 添加流体射线检测

    // ========== 目标实体信息 ==========
    if (m_entityManager != nullptr && m_camera != nullptr) {
        const auto& pos = m_camera->position();
        auto entityIds = m_entityManager->getEntitiesInRange(pos.x, pos.y, pos.z, 5.0f);
        if (!entityIds.empty()) {
            m_rightLines.push_back("");
            oss.str("");
            oss << "Nearby Entities: " << entityIds.size();
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

std::string DebugScreen::formatMemory(i64 bytes) {
    const i64 MB = 1024 * 1024;
    return std::to_string(bytes / MB);
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

void DebugScreen::logColumnBlocks() {
    // 每秒输出一次
    m_columnLogTimer += m_frameTime;
    if (m_columnLogTimer < COLUMN_LOG_INTERVAL) {
        return;
    }
    m_columnLogTimer = 0.0f;

    // 检查必要对象
    if (m_camera == nullptr || m_world == nullptr) {
        return;
    }

    const auto& pos = m_camera->position();
    i32 blockX = static_cast<i32>(std::floor(pos.x));
    i32 blockZ = static_cast<i32>(std::floor(pos.z));

    // 获取世界高度范围
    i32 minY = m_world->getMinBuildHeight();
    i32 maxY = m_world->getMaxBuildHeight();

    spdlog::info("=== Column at X={}, Z={} (Y: {} to {}) ===", blockX, blockZ, minY, maxY - 1);

    // 从底层开始遍历到最顶层
    for (i32 y = minY; y < maxY; ++y) {
        const BlockState* state = m_world->getBlockState(blockX, y, blockZ);
        if (state == nullptr) {
            continue;  // 区块未加载，跳过
        }

        // 跳过空气方块
        if (state->isAir()) {
            continue;
        }

        const ResourceLocation& loc = state->blockLocation();
        spdlog::info("  Y={}: {} (id={})", y, loc.toString(), state->blockId());
    }

    spdlog::info("=== End of column ===");
}

} // namespace mr::client
