#pragma once

#include "Layer.hpp"
#include <vector>
#include <mutex>
#include <random>

namespace mr {

/**
 * @brief 延迟计算的区域实现
 *
 * 缓存采样结果以提高性能。
 * 使用 Lazy evaluation 模式，只在需要时计算。
 *
 * 参考 MC 1.16.5 LazyArea
 */
class LazyArea : public IArea {
public:
    /**
     * @brief 构造延迟区域
     * @param transformer 变换器
     * @param context 区域上下文
     * @param input 输入区域（可以为空）
     * @param width 宽度
     * @param height 高度
     */
    LazyArea(std::unique_ptr<IAreaTransformer> transformer,
             std::shared_ptr<IAreaContext> context,
             std::shared_ptr<IArea> input,
             i32 width, i32 height);

    ~LazyArea() override = default;

    [[nodiscard]] i32 getValue(i32 x, i32 z) const override;
    [[nodiscard]] i32 getWidth() const override { return m_width; }
    [[nodiscard]] i32 getHeight() const override { return m_height; }

    /**
     * @brief 获取缓存的值（如果已计算）
     * @param x X 坐标
     * @param z Z 坐标
     * @param outValue 输出值
     * @return 是否已缓存
     */
    [[nodiscard]] bool getCachedValue(i32 x, i32 z, i32& outValue) const;

    /**
     * @brief 清除缓存
     */
    void clearCache();

private:
    std::unique_ptr<IAreaTransformer> m_transformer;
    std::shared_ptr<IAreaContext> m_context;
    std::shared_ptr<IArea> m_input;
    i32 m_width;
    i32 m_height;

    // 缓存
    mutable std::vector<i32> m_cache;
    mutable std::vector<bool> m_cached;
    mutable std::mutex m_mutex;

    /**
     * @brief 计算并缓存值
     */
    [[nodiscard]] i32 computeValue(i32 x, i32 z) const;

    /**
     * @brief 将坐标转换为缓存索引
     */
    [[nodiscard]] i32 getIndex(i32 x, i32 z) const;
};

/**
 * @brief 简单区域上下文实现
 */
class SimpleAreaContext : public IAreaContext {
public:
    explicit SimpleAreaContext(u64 seed = 0);

    void initRandom(u64 seed) override;
    [[nodiscard]] i32 nextInt(i32 bound) override;
    [[nodiscard]] i32 nextIntWithMod(i32 bound) override;

private:
    std::mt19937_64 m_rng;
    u64 m_baseSeed;
    u64 m_currentSeed;
};

} // namespace mr
