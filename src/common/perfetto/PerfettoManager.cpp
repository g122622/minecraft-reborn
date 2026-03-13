/**
 * @file PerfettoManager.cpp
 * @brief Perfetto 追踪管理器实现
 */

#include "PerfettoManager.hpp"
#include "TraceCategories.hpp"

#if MC_ENABLE_TRACING

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <perfetto.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <fstream>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace mc {
namespace perfetto {

/**
 * @brief PerfettoManager 的实现细节
 *
 * 使用 Pimpl 模式隐藏 Perfetto 特定类型。
 */
class PerfettoManager::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    std::unique_ptr<::perfetto::TracingSession> tracingSession;
};

PerfettoManager& PerfettoManager::instance() {
    static PerfettoManager instance;
    return instance;
}

PerfettoManager::PerfettoManager()
    : m_impl(std::make_unique<Impl>()) {
}

PerfettoManager::~PerfettoManager() {
    if (m_initialized && m_tracing) {
        stopTracing();
    }
    if (m_initialized) {
        shutdown();
    }
}

void PerfettoManager::initialize(const TraceConfig& config) {
    if (m_initialized) {
        spdlog::warn("[Perfetto] Already initialized, skipping");
        return;
    }

    m_config = config;

    // 初始化 Perfetto
    ::perfetto::TracingInitArgs args;
    args.backends = ::perfetto::kInProcessBackend;
    ::perfetto::Tracing::Initialize(args);
    ::perfetto::TrackEvent::Register();

    m_initialized = true;
    m_enabled = config.enabled;

    spdlog::info("[Perfetto] Initialized with buffer size {} KB", m_config.bufferSizeKb);
    spdlog::info("[Perfetto] Output file: {}", m_config.outputPath);
}

void PerfettoManager::shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_tracing) {
        stopTracing();
    }

    m_initialized = false;
    spdlog::info("[Perfetto] Shutdown complete");
}

void PerfettoManager::startTracing() {
    if (!m_initialized) {
        spdlog::error("[Perfetto] Cannot start tracing: not initialized");
        return;
    }

    if (m_tracing) {
        spdlog::warn("[Perfetto] Tracing already started");
        return;
    }

    // 配置追踪会话
    ::perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(static_cast<uint32_t>(m_config.bufferSizeKb));

    // 配置数据源
    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    // 配置分类过滤
    ::perfetto::protos::gen::TrackEventConfig te_cfg;

    // 如果指定了启用的分类
    if (!m_config.enabledCategories.empty()) {
        for (const auto& cat : m_config.enabledCategories) {
            te_cfg.add_enabled_categories(cat);
        }
    }

    // 如果指定了禁用的分类
    if (!m_config.disabledCategories.empty()) {
        for (const auto& cat : m_config.disabledCategories) {
            te_cfg.add_disabled_categories(cat);
        }
    }

    ds_cfg->set_track_event_config_raw(te_cfg.SerializeAsString());

    // 配置文件输出
    if (m_config.outputToFile && !m_config.outputPath.empty()) {
        cfg.set_write_into_file(true);
        cfg.set_output_path(m_config.outputPath);
        cfg.set_file_write_period_ms(5000);  // 每 5 秒写入一次
    }

    // 创建并启动追踪会话
    m_impl->tracingSession = ::perfetto::Tracing::NewTrace();
    m_impl->tracingSession->Setup(cfg);
    m_impl->tracingSession->StartBlocking();

    m_tracing = true;
    spdlog::info("[Perfetto] Tracing started");
}

void PerfettoManager::stopTracing() {
    if (!m_initialized || !m_tracing) {
        return;
    }

    // 刷新 TrackEvent 数据源
    ::perfetto::TrackEvent::Flush();

    // 停止追踪会话
    if (m_impl->tracingSession) {
        m_impl->tracingSession->StopBlocking();

        // 如果没有配置直接写入文件，则手动读取并写入
        if (!m_config.outputToFile || m_config.outputPath.empty()) {
            std::vector<char> trace_data = m_impl->tracingSession->ReadTraceBlocking();
            if (!trace_data.empty()) {
                std::ofstream output(m_config.outputPath, std::ios::binary);
                if (output.is_open()) {
                    output.write(trace_data.data(), trace_data.size());
                    spdlog::info("[Perfetto] Trace written to: {}", m_config.outputPath);
                } else {
                    spdlog::error("[Perfetto] Failed to open output file: {}", m_config.outputPath);
                }
            }
        }

        m_impl->tracingSession.reset();
    }

    m_tracing = false;
    spdlog::info("[Perfetto] Tracing stopped");
}

void PerfettoManager::flush() {
    if (!m_initialized || !m_tracing) {
        return;
    }

    ::perfetto::TrackEvent::Flush();
    spdlog::debug("[Perfetto] Flushed");
}

bool PerfettoManager::isEnabled() const {
    return m_initialized && m_enabled && m_tracing;
}

} // namespace perfetto
} // namespace mc

#endif // MC_ENABLE_TRACING
