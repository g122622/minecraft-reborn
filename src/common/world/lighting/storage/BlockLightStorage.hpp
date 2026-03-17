#pragma once

#include "SectionLightStorage.hpp"
#include "LightDataMap.hpp"
#include <unordered_map>

namespace mc {

/**
 * @brief 方块光照数据映射
 *
 * 管理方块光照数据的简单映射。
 * 参考: net.minecraft.world.lighting.BlockLightStorage.StorageMap
 */
class BlockLightDataMap : public LightDataMap<BlockLightDataMap> {
public:
    BlockLightDataMap() = default;
    explicit BlockLightDataMap(std::unordered_map<i64, NibbleArray> arrays)
        : LightDataMap(std::move(arrays)) {}

    /**
     * @brief 复制数据映射
     */
    [[nodiscard]] BlockLightDataMap copy() const {
        std::unordered_map<i64, NibbleArray> copiedArrays;
        copiedArrays.reserve(m_arrays.size());
        for (const auto& [pos, array] : m_arrays) {
            copiedArrays[pos] = array.copy();
        }
        return BlockLightDataMap(std::move(copiedArrays));
    }
};

/**
 * @brief 方块光照存储
 *
 * 管理方块光照的数据存储和区块段状态。
 * 参考: net.minecraft.world.lighting.BlockLightStorage
 */
class BlockLightStorage : public SectionLightStorage<BlockLightDataMap> {
public:
    /**
     * @brief 构造函数
     * @param provider 区块光照提供者
     */
    explicit BlockLightStorage(IChunkLightProvider* provider);

    // ========================================================================
    // 光照访问
    // ========================================================================

    /**
     * @brief 获取指定位置的光照等级，如果不存在返回0
     */
    [[nodiscard]] u8 getLightOrDefault(i64 worldPos) const override;

    /**
     * @brief 获取指定位置的实际光照等级
     */
    [[nodiscard]] u8 getLight(i64 worldPos) const;

    /**
     * @brief 设置指定位置的光照等级
     */
    void setLight(i64 worldPos, u8 light);

private:
    /**
     * @brief 获取区块的光照数组
     */
    [[nodiscard]] const NibbleArray* getArrayInSection(i64 sectionPos, bool useCache) const;
};

} // namespace mc
