#pragma once

#include "LightDataMap.hpp"
#include "../LightType.hpp"
#include "../IChunkLightProvider.hpp"
#include "../engine/LightEngineUtils.hpp"
#include "../../chunk/ChunkPos.hpp"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <climits>

namespace mc {

/**
 * @brief 区块段光照存储基类
 *
 * 管理区块段级别的光照数据存储和更新。
 * 提供光照数组的管理、区块段状态追踪等功能。
 *
 * 参考: net.minecraft.world.lighting.SectionLightStorage
 */
template<typename M>
class SectionLightStorage {
public:
    /**
     * @brief 构造函数
     *
     * @param type 光照类型
     * @param provider 区块光照提供者
     * @param dataMap 数据映射
     */
    SectionLightStorage(LightType type, IChunkLightProvider* provider, M&& dataMap)
        : m_type(type)
        , m_chunkProvider(provider)
        , m_cachedLightData(std::move(dataMap)) {
        m_uncachedLightData = m_cachedLightData.copy();
        m_uncachedLightData.disableCaching();
    }

    virtual ~SectionLightStorage() = default;

    // ========================================================================
    // 区块段管理
    // ========================================================================

    /**
     * @brief 检查区块段是否存在
     */
    [[nodiscard]] bool hasSection(i64 sectionPos) const {
        return getArray(sectionPos, true) != nullptr;
    }

    /**
     * @brief 获取区块段的光照数组
     *
     * @param sectionPos 区块段位置
     * @param useCache 是否使用缓存
     * @return 光照数组指针
     */
    [[nodiscard]] NibbleArray* getArray(i64 sectionPos, bool useCache) {
        if (useCache) {
            return m_cachedLightData.getArray(sectionPos);
        }
        return m_uncachedLightData.getArray(sectionPos);
    }

    [[nodiscard]] const NibbleArray* getArray(i64 sectionPos, bool useCache) const {
        if (useCache) {
            return m_cachedLightData.getArray(sectionPos);
        }
        return m_uncachedLightData.getArray(sectionPos);
    }

    /**
     * @brief 获取区块段的光照数组（公共接口）
     */
    [[nodiscard]] NibbleArray* getArray(i64 sectionPos) {
        NibbleArray* newArray = m_newArrays.getArray(sectionPos);
        if (newArray != nullptr) {
            return newArray;
        }
        return getArray(sectionPos, false);
    }

    /**
     * @brief 设置区块段的光照数据
     */
    virtual void setData(i64 sectionPos, NibbleArray* array, bool retain) {
        if (array != nullptr) {
            m_newArrays.setArray(sectionPos, array->copy());
            if (!retain) {
                m_dirtyNewArrays.insert(sectionPos);
            }
        } else {
            m_newArrays.removeArray(sectionPos);
        }
    }

    /**
     * @brief 更新区块段状态
     *
     * @param sectionPos 区块段位置
     * @param isEmpty 是否为空（没有方块）
     */
    void updateSectionStatus(i64 sectionPos, bool isEmpty) {
        bool isActive = m_activeLightSections.count(sectionPos) > 0;

        if (!isActive && !isEmpty) {
            m_addedActiveSections.insert(sectionPos);
        }

        if (isActive && isEmpty) {
            m_addedEmptySections.insert(sectionPos);
        }
    }

    /**
     * @brief 设置区块列是否启用
     *
     * @param columnPos 区块列位置
     * @param enabled 是否启用
     */
    virtual void setColumnEnabled(i64 columnPos, bool enabled) {
        if (enabled) {
            m_enabledColumns.insert(columnPos);
        } else {
            m_enabledColumns.erase(columnPos);
        }
    }

    // ========================================================================
    // 光照访问
    // ========================================================================

    /**
     * @brief 获取指定位置的光照等级
     */
    [[nodiscard]] virtual u8 getLightOrDefault(i64 worldPos) const = 0;

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 检查是否有待处理的区块段更新
     */
    [[nodiscard]] virtual bool hasSectionsToUpdate() const {
        return !m_noLightSections.empty() || !m_newArrays.empty();
    }

    /**
     * @brief 处理所有级别更新
     */
    void processAllLevelUpdates() {
        // 处理区块段状态变化
        if (!m_addedActiveSections.empty()) {
            for (i64 sectionPos : m_addedActiveSections) {
                if (m_activeLightSections.insert(sectionPos).second) {
                    addSection(sectionPos);
                }
            }
            m_addedActiveSections.clear();
        }

        if (!m_addedEmptySections.empty()) {
            for (i64 sectionPos : m_addedEmptySections) {
                if (m_activeLightSections.erase(sectionPos) > 0) {
                    removeSection(sectionPos);
                }
            }
            m_addedEmptySections.clear();
        }

        // 提交待写入的光照数组到缓存数据
        if (!m_newArrays.empty()) {
            std::vector<i64> newSectionPositions;
            newSectionPositions.reserve(m_newArrays.size());

            for (const auto& [sectionPos, array] : m_newArrays) {
                m_cachedLightData.setArray(sectionPos, array.copy());
                m_dirtyCachedSections.insert(sectionPos);
                m_changedLightPositions.insert(sectionPos);
                newSectionPositions.push_back(sectionPos);
            }

            for (i64 sectionPos : newSectionPositions) {
                m_newArrays.removeArray(sectionPos);
            }
        }

        // 处理待移除的无光照区块段
        if (!m_noLightSections.empty()) {
            for (i64 sectionPos : m_noLightSections) {
                m_cachedLightData.removeArray(sectionPos);
                m_dirtyCachedSections.insert(sectionPos);
                m_changedLightPositions.insert(sectionPos);
            }
            m_noLightSections.clear();
        }

        m_dirtyNewArrays.clear();
    }

    /**
     * @brief 更新缓存和通知
     */
    void updateAndNotify() {
        if (!m_dirtyCachedSections.empty()) {
            M copied = m_cachedLightData.copy();
            copied.disableCaching();
            m_uncachedLightData = std::move(copied);
            m_dirtyCachedSections.clear();
        }

        if (!m_changedLightPositions.empty()) {
            for (i64 pos : m_changedLightPositions) {
                m_chunkProvider->markLightChanged(m_type, SectionPos::fromLong(pos));
            }
            m_changedLightPositions.clear();
        }
    }

protected:
    LightType m_type;
    IChunkLightProvider* m_chunkProvider;
    M m_cachedLightData;
    M m_uncachedLightData;
    M m_newArrays;

    // 活跃的光照区块段（包含需要光照更新的方块）
    std::unordered_set<i64> m_activeLightSections;

    // 新增的空区块段
    std::unordered_set<i64> m_addedEmptySections;

    // 新增的活跃区块段
    std::unordered_set<i64> m_addedActiveSections;

    // 无光照区块段（可以移除）
    std::unordered_set<i64> m_noLightSections;

    // 启用的区块列
    std::unordered_set<i64> m_enabledColumns;

    // 脏缓存区块段
    std::unordered_set<i64> m_dirtyCachedSections;

    // 变更的光照位置
    std::unordered_set<i64> m_changedLightPositions;

    // 新数组标记为脏
    std::unordered_set<i64> m_dirtyNewArrays;

    // 区块数据保留
    std::unordered_set<i64> m_chunksToRetain;

    // ========================================================================
    // 辅助方法
    // ========================================================================

    /**
     * @brief 获取或创建光照数组
     */
    [[nodiscard]] NibbleArray getOrCreateArray(i64 sectionPos) {
        NibbleArray* newArray = m_newArrays.getArray(sectionPos);
        if (newArray != nullptr) {
            return newArray->copy();
        }
        return NibbleArray();
    }

    /**
     * @brief 标记区块段为活跃
     */
    void markActive(i64 sectionPos) {
        m_activeLightSections.insert(sectionPos);
        m_addedActiveSections.erase(sectionPos);
    }

    /**
     * @brief 标记区块段为非活跃
     */
    void markInactive(i64 sectionPos) {
        m_activeLightSections.erase(sectionPos);
        m_addedEmptySections.erase(sectionPos);
    }

    /**
     * @brief 世界位置转区块段位置
     */
    [[nodiscard]] static i64 worldToSectionPos(i64 worldPos) {
        return LightEngineUtils::worldToSectionPos(worldPos);
    }

    /**
     * @brief 添加区块段
     */
    virtual void addSection(i64 sectionPos) {
        // 子类可重写
    }

    /**
     * @brief 移除区块段
     */
    virtual void removeSection(i64 sectionPos) {
        // 子类可重写
    }
};

} // namespace mc
