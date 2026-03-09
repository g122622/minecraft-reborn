#include "UniformRealDistribution.hpp"
#include "Random.hpp"

namespace mr::math {

f32 UniformRealDistribution::generate() const {
    Random rng;
    return rng.nextFloat(m_min, m_max);
}

} // namespace mr::math
