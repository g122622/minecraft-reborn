#include "DebugScreen.hpp"
#include "../world/ClientWorld.hpp"
#include "../../common/world/block/Block.hpp"
#include "../../common/resource/ResourceLocation.hpp"
#include "../../common/util/Direction.hpp"
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
    buildDebugText();
    if (1) {
        logColumnBlocks();
    }
}

void DebugScreen::render() {
    if (!m_visible || m_guiRenderer == nullptr) return;

    f32 x = 10.0f;
    f32 y = 10.0f;
    f32 lineHeight = static_cast<f32>(m_guiRenderer->getFontHeight() + 2);

    // 绘制半透明背景
    m_guiRenderer->fillRect(5.0f, 5.0f,
                            m_guiRenderer->screenWidth() - 10.0f,
                            lineHeight * static_cast<f32>(m_debugLines.size()) + 10.0f,
                            0xA0303030); // 半透明深灰

    // 绘制调试文本
    for (const auto& line : m_debugLines) {
        m_guiRenderer->drawText(line, x, y, COLOR_WHITE, true);
        y += lineHeight;
    }
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

void DebugScreen::buildDebugText() {
    m_debugLines.clear();

    std::ostringstream oss;

    // 版本信息
    oss.str("");
    oss << m_version << " (" << m_rendererInfo << ")";
    m_debugLines.push_back(oss.str());

    // FPS信息
    oss.str("");
    oss << std::fixed << std::setprecision(0) << m_fps << " fps"
        << " T: " << std::setprecision(1) << (m_frameTime * 1000.0f) << " ms";
    m_debugLines.push_back(oss.str());

    // 分隔行
    m_debugLines.push_back("");

    // 相机/玩家信息
    if (m_camera != nullptr) {
        const auto& pos = m_camera->position();

        // 坐标
        oss.str("");
        oss << "XYZ: " << std::fixed << std::setprecision(3)
            << pos.x << " / " << pos.y << " / " << pos.z;
        m_debugLines.push_back(oss.str());

        // 区块坐标
        oss.str("");
        i32 chunkX = static_cast<i32>(std::floor(pos.x / 16.0));
        i32 chunkZ = static_cast<i32>(std::floor(pos.z / 16.0));
        oss << "Chunk: " << chunkX << ", " << chunkZ;
        m_debugLines.push_back(oss.str());

        // 相对于区块的位置
        oss.str("");
        f32 relX = pos.x - chunkX * 16.0f;
        f32 relZ = pos.z - chunkZ * 16.0f;
        oss << "Relative pos in current chunk: " << std::fixed << std::setprecision(1)
            << relX << ", " << relZ;
        m_debugLines.push_back(oss.str());

        // 朝向
        oss.str("");
        const auto& rot = m_camera->rotation();
        oss << "Facing: " << std::fixed << std::setprecision(1)
            << rot.x << ", " << rot.y;
        m_debugLines.push_back(oss.str());
    } else {
        m_debugLines.push_back("No camera");
    }

    // 分隔行
    m_debugLines.push_back("");

    // 世界信息
    if (m_world != nullptr) {
        oss.str("");
        oss << "Loaded Chunks: " << m_world->chunkCount();
        m_debugLines.push_back(oss.str());

        oss.str("");
        oss << "Render Distance: " << m_world->renderDistance();
        m_debugLines.push_back(oss.str());
    } else {
        m_debugLines.push_back("No world loaded");
    }

    // 分隔行
    m_debugLines.push_back("");

    // 目标方块信息
    buildTargetBlockText();

    // Java版MC风格的调试信息
    m_debugLines.push_back("Press F3 to hide debug info");
    m_debugLines.push_back("Press F3+H for advanced tooltips");
}

void DebugScreen::buildTargetBlockText() {
    if (m_targetBlock == nullptr || m_targetBlock->isMiss()) {
        if (m_targetBlock == nullptr) {
            m_debugLines.push_back("No target block");
        } else if (m_targetBlock->isMiss()) {
            // 添加射线检测调试信息
            m_debugLines.push_back("Looking at: miss (raycast returned miss)");
        } else {
            m_debugLines.push_back("Looking at: Unknown");
        }
        return;
    }

    std::ostringstream oss;

    // 显示击中的方块坐标
    const auto& blockPos = m_targetBlock->blockPos();
    oss.str("");
    oss << "Looking at: " << blockPos.x << ", " << blockPos.y << ", " << blockPos.z;
    m_debugLines.push_back(oss.str());

    // 尝试获取方块名称
    if (m_world != nullptr) {
        const BlockState* state = m_world->getBlockState(blockPos.x, blockPos.y, blockPos.z);
        if (state != nullptr) {
            const ResourceLocation& loc = state->blockLocation();
            oss.str("");
            oss << "BLOCK: " << loc.toString();
            m_debugLines.push_back(oss.str());
        }
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

    // 从 y=0 开始遍历到最顶层
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
