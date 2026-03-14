#include "BiomeProvider.hpp"
#include "BiomeRegistry.hpp"

namespace mc {

// ============================================================================
// BiomeProvider 实现
// ============================================================================

BiomeProvider::BiomeProvider(u64 seed)
    : m_seed(seed)
{
}

const Biome& BiomeProvider::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

} // namespace mc
