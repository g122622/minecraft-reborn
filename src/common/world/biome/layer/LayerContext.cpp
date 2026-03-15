#include "LayerContext.hpp"
#include "transformers/TransformerTraits.hpp"
#include "common/perfetto/TraceEvents.hpp"

namespace mc {

// ============================================================================
// Long2IntLRUCache 实现 - O(1) LRU 缓存
// ============================================================================

Long2IntLRUCache::Long2IntLRUCache(i32 maxSize)
    : m_maxSize(maxSize)
{
    m_cache.reserve(static_cast<size_t>(maxSize * 2));  // 预分配空间
}

bool Long2IntLRUCache::get(i64 key, i32& value) {
    // MC_TRACE_EVENT("world.biome", "LRUCache_Get");
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // 移动到链表前端（最近访问）
        m_list.splice(m_list.begin(), m_list, it->second);
        value = it->second->second;
        return true;
    }
    return false;
}

void Long2IntLRUCache::put(i64 key, i32 value) {
    // MC_TRACE_EVENT("world.biome", "LRUCache_Put");
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否已存在
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // 更新值并移动到前端
        it->second->second = value;
        m_list.splice(m_list.begin(), m_list, it->second);
        return;
    }

    // 检查是否需要淘汰
    if (static_cast<i32>(m_cache.size()) >= m_maxSize) {
        // 移除链表尾部（最旧）
        i64 oldestKey = m_list.back().first;
        m_cache.erase(oldestKey);
        m_list.pop_back();
    }

    // 插入新条目到前端
    m_list.emplace_front(key, value);
    m_cache[key] = m_list.begin();
}

i32 Long2IntLRUCache::size() const {
    return static_cast<i32>(m_cache.size());
}

void Long2IntLRUCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_list.clear();
}

// ============================================================================
// LayerContext 实现
// ============================================================================

LayerContext::LayerContext(i32 maxCacheSize, u64 worldSeed, u64 modifier)
    : m_maxCacheSize(maxCacheSize)
    , m_worldSeed(worldSeed)
    , m_layerSeed(hashLayerSeed(worldSeed, modifier))
    , m_noise(worldSeed)  // 使用种子初始化噪声生成器
    , m_cache(maxCacheSize)
{
    // MC_TRACE_EVENT("world.biome", "LayerContext_Construct", "maxCacheSize", maxCacheSize, "modifier", static_cast<i64>(modifier));
}

void LayerContext::setPosition(i64 x, i64 z) {
    // 使用 FastRandom.mix 算法计算位置种子
    // 参考 MC LazyAreaLayerContext.setPosition:
    // long i = this.seed;
    // i = FastRandom.mix(i, x);
    // i = FastRandom.mix(i, z);
    // i = FastRandom.mix(i, x);
    // i = FastRandom.mix(i, z);
    // this.positionSeed = i;

    u64 seed = m_layerSeed;
    seed = mix(seed, static_cast<u64>(x));
    seed = mix(seed, static_cast<u64>(z));
    seed = mix(seed, static_cast<u64>(x));
    seed = mix(seed, static_cast<u64>(z));
    m_positionSeed = seed;
}

i32 LayerContext::nextInt(i32 bound) {
    // 参考 MC LazyAreaLayerContext.random:
    // int i = (int)Math.floorMod(this.positionSeed >> 24, (long)bound);
    // this.positionSeed = FastRandom.mix(this.positionSeed, this.seed);
    // return i;

    if (bound <= 0) {
        return 0;
    }

    i32 result = static_cast<i32>((m_positionSeed >> 24) % bound);
    if (result < 0) {
        result += bound;
    }
    m_positionSeed = mix(m_positionSeed, m_layerSeed);
    return result;
}

i32 LayerContext::pickRandom(i32 a, i32 b) {
    return nextInt(2) == 0 ? a : b;
}

i32 LayerContext::pickRandom(i32 a, i32 b, i32 c, i32 d) {
    i32 i = nextInt(4);
    if (i == 0) return a;
    if (i == 1) return b;
    if (i == 2) return c;
    return d;
}

std::unique_ptr<IArea> LayerContext::makeArea(PixelFunc pixelFunc) {
    return std::make_unique<LazyArea>(m_cache, m_maxCacheSize, std::move(pixelFunc));
}

std::unique_ptr<IArea> LayerContext::makeArea(PixelFunc pixelFunc, std::unique_ptr<IArea> input) {
    std::vector<std::unique_ptr<IArea>> ownedAreas;
    ownedAreas.push_back(std::move(input));
    return std::make_unique<LazyArea>(m_cache, std::min(1024, m_maxCacheSize * 4), std::move(pixelFunc), std::move(ownedAreas));
}

std::unique_ptr<IArea> LayerContext::makeArea(PixelFunc pixelFunc,
                                               std::unique_ptr<IArea> input1,
                                               std::unique_ptr<IArea> input2) {
    std::vector<std::unique_ptr<IArea>> ownedAreas;
    ownedAreas.push_back(std::move(input1));
    ownedAreas.push_back(std::move(input2));
    i32 newSize = std::min(1024, m_maxCacheSize * 4);
    return std::make_unique<LazyArea>(m_cache, newSize, std::move(pixelFunc), std::move(ownedAreas));
}

std::unique_ptr<LayerContext> LayerContext::withModifier(u64 modifier) const {
    return std::make_unique<LayerContext>(m_maxCacheSize, m_worldSeed, modifier);
}

// 静态成员函数

u64 LayerContext::mix(u64 left, u64 right) {
    // 参考 MC FastRandom.mix:
    // left = left * (left * 6364136223846793005L + 1442695040888963407L);
    // return left + right;
    left = left * (left * 6364136223846793005ULL + 1442695040888963407ULL);
    return left + right;
}

u64 LayerContext::hashLayerSeed(u64 worldSeed, u64 modifier) {
    // 参考 MC LazyAreaLayerContext 构造函数:
    // this.seed = hash(seedIn, seedModifierIn);
    // 其中 hash:
    // long lvt_4_1_ = FastRandom.mix(right, right);
    // lvt_4_1_ = FastRandom.mix(lvt_4_1_, right);
    // lvt_4_1_ = FastRandom.mix(lvt_4_1_, right);
    // long lvt_6_1_ = FastRandom.mix(left, lvt_4_1_);
    // lvt_6_1_ = FastRandom.mix(lvt_6_1_, lvt_4_1_);
    // return FastRandom.mix(lvt_6_1_, lvt_4_1_);

    u64 hash = mix(modifier, modifier);
    hash = mix(hash, modifier);
    hash = mix(hash, modifier);

    u64 result = mix(worldSeed, hash);
    result = mix(result, hash);
    result = mix(result, hash);

    return result;
}

// ============================================================================
// LazyArea 实现
// ============================================================================

LazyArea::LazyArea(Long2IntLRUCache& cache, i32 maxCacheSize, PixelFunc pixelFunc)
    : m_sharedCache(&cache)
    , m_ownCache(nullptr)
    , m_pixelFunc(std::move(pixelFunc))
    , m_maxCacheSize(maxCacheSize)
{
}

LazyArea::LazyArea(i32 maxCacheSize, PixelFunc pixelFunc)
    : m_sharedCache(nullptr)
    , m_ownCache(std::make_unique<Long2IntLRUCache>(maxCacheSize))
    , m_pixelFunc(std::move(pixelFunc))
    , m_maxCacheSize(maxCacheSize)
{
}

LazyArea::LazyArea(Long2IntLRUCache& cache, i32 maxCacheSize, PixelFunc pixelFunc,
                   std::vector<std::unique_ptr<IArea>> ownedAreas)
    : m_sharedCache(&cache)
    , m_ownCache(nullptr)
    , m_pixelFunc(std::move(pixelFunc))
    , m_maxCacheSize(maxCacheSize)
    , m_ownedAreas(std::move(ownedAreas))
{
}

i32 LazyArea::getValue(i32 x, i32 z) const {
    // MC_TRACE_EVENT("world.biome", "LazyArea_GetValue", "x", x, "z", z);
    i64 key = Long2IntLRUCache::packCoords(x, z);
    i32 value;

    Long2IntLRUCache& cache = m_sharedCache ? *m_sharedCache : *m_ownCache;

    if (cache.get(key, value)) {
        return value;
    }

    // 计算并缓存
    value = m_pixelFunc(x, z);
    cache.put(key, value);
    return value;
}

// ============================================================================
// SourceFactory 实现
// ============================================================================

SourceFactory::SourceFactory(ITransformer0* transformer, std::shared_ptr<LayerContext> context)
    : m_transformer(transformer)
    , m_context(std::move(context))
{
}

std::unique_ptr<IArea> SourceFactory::create() const {
    // 捕获 shared_ptr 以保持生命周期
    ITransformer0* transformer = m_transformer;
    std::shared_ptr<LayerContext> ctx = m_context;

    PixelFunc func = [transformer, ctx](i32 x, i32 z) -> i32 {
        ctx->setPosition(x, z);
        return transformer->apply(*ctx, x, z);
    };

    return m_context->makeArea(func);
}

// ============================================================================
// TransformFactory 实现
// ============================================================================

TransformFactory::TransformFactory(ITransformer1* transformer,
                                   std::shared_ptr<LayerContext> context,
                                   std::unique_ptr<IAreaFactory> input)
    : m_transformer(transformer)
    , m_context(std::move(context))
    , m_input(std::move(input))
{
}

std::unique_ptr<IArea> TransformFactory::create() const {
    // 创建输入区域
    std::unique_ptr<IArea> inputArea = m_input->create();

    // 捕获 shared_ptr 以保持生命周期
    ITransformer1* transformer = m_transformer;
    std::shared_ptr<LayerContext> ctx = m_context;
    IArea* inputPtr = inputArea.get();

    PixelFunc func = [transformer, ctx, inputPtr](i32 x, i32 z) -> i32 {
        ctx->setPosition(x, z);
        return transformer->apply(*ctx, *inputPtr, x, z);
    };

    return m_context->makeArea(func, std::move(inputArea));
}

// ============================================================================
// MergeFactory 实现
// ============================================================================

MergeFactory::MergeFactory(ITransformer2* transformer,
                           std::shared_ptr<LayerContext> context,
                           std::unique_ptr<IAreaFactory> input1,
                           std::unique_ptr<IAreaFactory> input2)
    : m_transformer(transformer)
    , m_context(std::move(context))
    , m_input1(std::move(input1))
    , m_input2(std::move(input2))
{
}

std::unique_ptr<IArea> MergeFactory::create() const {
    std::unique_ptr<IArea> area1 = m_input1->create();
    std::unique_ptr<IArea> area2 = m_input2->create();

    // 捕获 shared_ptr 以保持生命周期
    ITransformer2* transformer = m_transformer;
    std::shared_ptr<LayerContext> ctx = m_context;
    IArea* ptr1 = area1.get();
    IArea* ptr2 = area2.get();

    PixelFunc func = [transformer, ctx, ptr1, ptr2](i32 x, i32 z) -> i32 {
        ctx->setPosition(x, z);
        return transformer->apply(*ctx, *ptr1, *ptr2, x, z);
    };

    return m_context->makeArea(func, std::move(area1), std::move(area2));
}

} // namespace mc
