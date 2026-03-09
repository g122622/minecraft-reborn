#include "UniformIntDistribution.hpp"
#include "Random.hpp"

namespace mr::math {

i32 UniformIntDistribution::generate() const {
    Random rng;
    return rng.nextInt(m_min, m_max);
}

} // namespace mr::math
