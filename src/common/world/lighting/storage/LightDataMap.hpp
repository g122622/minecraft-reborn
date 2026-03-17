#pragma once

#include "../../../util/NibbleArray.hpp"
#include <unordered_map>
#include <memory>

namespace mc {

/**
 * @brief 光照数据映射基类
 *
 * 管理区块段的光照数据存储。使用NibbleArray存储每个方块的光照等级。
 *
 * 参考: net.minecraft.world.lighting.LightDataMap
 */
template<typename Derived>
class LightDataMap {
public:
    LightDataMap() = default;
    explicit LightDataMap(std::unordered_map<i64, NibbleArray> arrays)
        : m_arrays(std::move(arrays)) {}

    virtual ~LightDataMap() = default;

    // 禁止拷贝
    LightDataMap(const LightDataMap&) = delete;
    LightDataMap& operator=(const LightDataMap&) = delete;

    // 允许移动
    LightDataMap(LightDataMap&&) = default;
    LightDataMap& operator=(LightDataMap&&) = default;

    // ========================================================================
    // 数据访问
    // ========================================================================

    /**
     * @brief 获取区块段的光照数组
     *
     * @param sectionPos 区块段位置编码
     * @return 光照数组指针，如果不存在返回nullptr
     */
    [[nodiscard]] NibbleArray* getArray(i64 sectionPos) {
        auto it = m_arrays.find(sectionPos);
        return it != m_arrays.end() ? &it->second : nullptr;
    }

    [[nodiscard]] const NibbleArray* getArray(i64 sectionPos) const {
        auto it = m_arrays.find(sectionPos);
        return it != m_arrays.end() ? &it->second : nullptr;
    }

    /**
     * @brief 检查区块段是否有光照数据
     */
    [[nodiscard]] bool hasArray(i64 sectionPos) const {
        return m_arrays.find(sectionPos) != m_arrays.end();
    }

    /**
     * @brief 设置区块段的光照数组
     */
    void setArray(i64 sectionPos, NibbleArray array) {
        m_arrays[sectionPos] = std::move(array);
    }

    /**
     * @brief 移除区块段的光照数组
     */
    NibbleArray removeArray(i64 sectionPos) {
        auto it = m_arrays.find(sectionPos);
        if (it != m_arrays.end()) {
            NibbleArray result = std::move(it->second);
            m_arrays.erase(it);
            return result;
        }
        return NibbleArray();
    }

    /**
     * @brief 获取光照等级
     *
     * @param worldPos 世界位置编码
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] u8 getLight(i64 worldPos) const {
        i64 sectionPos = worldToSection(worldPos);
        const NibbleArray* array = getArray(sectionPos);
        if (array == nullptr || array->isEmpty()) {
            return 0;
        }

        i32 x = static_cast<i32>((worldPos >> 38) & 0xF);
        i32 y = static_cast<i32>((worldPos >> 0) & 0xFFF);
        i32 z = static_cast<i32>((worldPos >> 26) & 0xF);

        // Y坐标转换为区块段内坐标
        i32 localY = y & 0xF;

        return array->get(x, localY, z);
    }

    /**
     * @brief 设置光照等级
     *
     * @param worldPos 世界位置编码
     * @param light 光照等级 (0-15)
     */
    void setLight(i64 worldPos, u8 light) {
        i64 sectionPos = worldToSection(worldPos);
        NibbleArray* array = getArray(sectionPos);
        if (array == nullptr) {
            return;
        }

        i32 x = static_cast<i32>((worldPos >> 38) & 0xF);
        i32 y = static_cast<i32>((worldPos >> 0) & 0xFFF);
        i32 z = static_cast<i32>((worldPos >> 26) & 0xF);

        // Y坐标转换为区块段内坐标
        i32 localY = y & 0xF;

        array->set(x, localY, z, light);
    }

    // ========================================================================
    // 复制和缓存
    // ========================================================================

    /**
     * @brief 复制整个映射
     */
    [[nodiscard]] Derived copy() const {
        std::unordered_map<i64, NibbleArray> copiedArrays;
        copiedArrays.reserve(m_arrays.size());
        for (const auto& [pos, array] : m_arrays) {
            copiedArrays[pos] = array.copy();
        }
        return Derived(std::move(copiedArrays));
    }

    /**
     * @brief 复制指定区块段的数据
     */
    void copyArray(i64 sectionPos) {
        auto it = m_arrays.find(sectionPos);
        if (it != m_arrays.end()) {
            it->second = it->second.copy();
        }
    }

    /**
     * @brief 禁用缓存（用于线程安全访问）
     */
    void disableCaching() {
        // 默认无操作，子类可重写
    }

    /**
     * @brief 使缓存失效
     */
    void invalidateCaches() {
        // 默认无操作，子类可重写
    }

    // ========================================================================
    // 迭代器
    // ========================================================================

    auto begin() { return m_arrays.begin(); }
    auto end() { return m_arrays.end(); }
    auto begin() const { return m_arrays.begin(); }
    auto end() const { return m_arrays.end(); }

    /**
     * @brief 获取区块段数量
     */
    [[nodiscard]] size_t size() const { return m_arrays.size(); }

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool empty() const { return m_arrays.empty(); }

protected:
    std::unordered_map<i64, NibbleArray> m_arrays;

    /**
     * @brief 世界位置转区块段位置
     *
     * 编码格式:
     * - X: 高22位 (pos >> 42)
     * - Y: 中20位 ((pos << 44) >> 44)
     * - Z: 低22位 ((pos << 22) >> 42)
     */
    [[nodiscard]] static i64 worldToSection(i64 worldPos) {
        i32 x = static_cast<i32>(worldPos >> 42);
        i32 y = static_cast<i32>((worldPos << 44) >> 44);
        i32 z = static_cast<i32>((worldPos << 22) >> 42);
        return sectionPosToLong(x >> 4, y >> 4, z >> 4);
    }

    /**
     * @brief 区块段坐标转长整型
     */
    [[nodiscard]] static i64 sectionPosToLong(i32 x, i32 y, i32 z) {
        i64 lx = static_cast<i64>(x) & 0x3FFFFFLL;
        i64 lz = static_cast<i64>(z) & 0x3FFFFFLL;
        i64 ly = static_cast<i64>(y) & 0xFFFFFLL;
        return (lx << 42) | (lz << 20) | ly;
    }
};

} // namespace mc
