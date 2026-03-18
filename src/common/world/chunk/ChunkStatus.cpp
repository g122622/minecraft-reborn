#include "ChunkStatus.hpp"

namespace mc {

// ============================================================================
// 静态成员初始化
// ============================================================================

// 阶段定义（按顺序）
const ChunkStatus ChunkStatus::EMPTY("empty", ChunkStatuses::EMPTY_ORDINAL, nullptr, 0);
const ChunkStatus ChunkStatus::BIOMES("biomes", ChunkStatuses::BIOMES_ORDINAL, &EMPTY, 0);
const ChunkStatus ChunkStatus::NOISE("noise", ChunkStatuses::NOISE_ORDINAL, &BIOMES, 0);
const ChunkStatus ChunkStatus::SURFACE("surface", ChunkStatuses::SURFACE_ORDINAL, &NOISE, 0);
const ChunkStatus ChunkStatus::CARVERS("carvers", ChunkStatuses::CARVERS_ORDINAL, &SURFACE, 0);
const ChunkStatus ChunkStatus::FEATURES("features", ChunkStatuses::FEATURES_ORDINAL, &CARVERS, 8);
const ChunkStatus ChunkStatus::HEIGHTMAPS("heightmaps", ChunkStatuses::HEIGHTMAPS_ORDINAL, &FEATURES, 0);
const ChunkStatus ChunkStatus::LIGHT("light", ChunkStatuses::LIGHT_ORDINAL, &HEIGHTMAPS, 0);
const ChunkStatus ChunkStatus::FULL("full", ChunkStatuses::FULL_ORDINAL, &LIGHT, 0);

// ============================================================================
// 构造函数
// ============================================================================

ChunkStatus::ChunkStatus(const String& name, i32 ordinal, const ChunkStatus* parent, i32 taskRange)
    : m_name(name)
    , m_ordinal(ordinal)
    , m_parent(parent ? parent : this)
    , m_taskRange(taskRange)
{
}

// ============================================================================
// 静态方法
// ============================================================================

const std::vector<ChunkStatus>& ChunkStatus::getAll()
{
    static const std::vector<ChunkStatus> allStatuses = {
        EMPTY,
        BIOMES,
        NOISE,
        SURFACE,
        CARVERS,
        FEATURES,
        HEIGHTMAPS,
        LIGHT,
        FULL
    };
    return allStatuses;
}

const ChunkStatus* ChunkStatus::byName(const String& name)
{
    const auto& all = getAll();
    for (const auto& status : all) {
        if (status.name() == name) {
            return &status;
        }
    }
    return nullptr;
}

const ChunkStatus* ChunkStatus::byOrdinal(i32 ordinal)
{
    const auto& all = getAll();
    if (ordinal >= 0 && ordinal < static_cast<i32>(all.size())) {
        return &all[ordinal];
    }
    return nullptr;
}

} // namespace mc
