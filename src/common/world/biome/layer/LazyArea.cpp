#include "LazyArea.hpp"
#include <algorithm>

namespace mr {

// ============================================================================
// LazyArea 实现
// ============================================================================

LazyArea::LazyArea(std::unique_ptr<IAreaTransformer> transformer,
                   std::shared_ptr<IAreaContext> context,
                   std::shared_ptr<IArea> input,
                   i32 width, i32 height)
    : m_transformer(std::move(transformer))
    , m_context(std::move(context))
    , m_input(std::move(input))
    , m_width(width)
    , m_height(height)
    , m_cache(static_cast<size_t>(width) * height, 0)
    , m_cached(static_cast<size_t>(width) * height, false)
{
}

i32 LazyArea::getValue(i32 x, i32 z) const
{
    // 边界检查和 wrapping
    x = ((x % m_width) + m_width) % m_width;
    z = ((z % m_height) + m_height) % m_height;

    const i32 index = getIndex(x, z);

    // 检查缓存
    if (m_cached[index]) {
        return m_cache[index];
    }

    // 计算并缓存
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_cached[index]) {
        return m_cache[index];
    }

    const i32 value = computeValue(x, z);
    m_cache[index] = value;
    m_cached[index] = true;
    return value;
}

bool LazyArea::getCachedValue(i32 x, i32 z, i32& outValue) const
{
    x = ((x % m_width) + m_width) % m_width;
    z = ((z % m_height) + m_height) % m_height;

    const i32 index = getIndex(x, z);
    if (m_cached[index]) {
        outValue = m_cache[index];
        return true;
    }
    return false;
}

void LazyArea::clearCache()
{
    std::fill(m_cached.begin(), m_cached.end(), false);
}

i32 LazyArea::computeValue(i32 x, i32 z) const
{
    if (m_transformer && m_context) {
        return m_transformer->apply(*m_context, *m_input, x, z);
    }
    return 0;
}

i32 LazyArea::getIndex(i32 x, i32 z) const
{
    return z * m_width + x;
}

// ============================================================================
// SimpleAreaContext 实现
// ============================================================================

SimpleAreaContext::SimpleAreaContext(u64 seed)
    : m_baseSeed(seed)
    , m_currentSeed(seed)
{
    m_rng.setSeed(seed);
}

void SimpleAreaContext::initRandom(u64 seed)
{
    m_currentSeed = seed;
    m_rng.setSeed(seed);
}

i32 SimpleAreaContext::nextInt(i32 bound)
{
    if (bound <= 0) {
        return 0;
    }
    return m_rng.nextInt(bound);
}

i32 SimpleAreaContext::nextIntWithMod(i32 bound)
{
    // 参考 MC 的混洗随机
    i32 i = static_cast<i32>(m_currentSeed >> 24) % bound;
    if (i < 0) {
        i += bound;
    }
    m_currentSeed = m_currentSeed * m_currentSeed * 6364136223846793005ULL + 1442695040888963407ULL;
    return i;
}

} // namespace mr
