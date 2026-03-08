#pragma once

#include "Biome.hpp"
#include <vector>

namespace mr {

/**
 * @brief 生物群系注册表
 *
 * 管理所有注册的生物群系定义。
 *
 * 使用方法：
 * @code
 * const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
 * @endcode
 */
class BiomeRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static BiomeRegistry& instance();

    /**
     * @brief 初始化注册表（注册所有默认生物群系）
     */
    void initialize();

    /**
     * @brief 注册生物群系
     * @param biome 生物群系定义
     */
    void registerBiome(const Biome& biome);

    /**
     * @brief 获取生物群系定义
     * @param id 生物群系ID
     * @return 生物群系定义，如果不存在返回默认生物群系
     */
    [[nodiscard]] const Biome& get(BiomeId id) const;

    /**
     * @brief 检查生物群系是否已注册
     */
    [[nodiscard]] bool hasBiome(BiomeId id) const;

    /**
     * @brief 获取所有已注册的生物群系
     */
    [[nodiscard]] const std::vector<Biome>& allBiomes() const { return m_biomes; }

private:
    BiomeRegistry();
    std::vector<Biome> m_biomes;
    Biome m_defaultBiome;

    void registerDefaultBiomes();
};

// ============================================================================
// 生物群系工厂函数（参考 MC BiomeMaker）
// ============================================================================

namespace BiomeFactory {

/**
 * @brief 创建平原生物群系
 */
Biome createPlains();

/**
 * @brief 创建沙漠生物群系
 */
Biome createDesert();

/**
 * @brief 创建山地生物群系
 */
Biome createMountains();

/**
 * @brief 创建森林生物群系
 */
Biome createForest();

/**
 * @brief 创建海洋生物群系
 */
Biome createOcean();

/**
 * @brief 创建深海生物群系
 */
Biome createDeepOcean();

/**
 * @brief 创建泰加林生物群系
 */
Biome createTaiga();

/**
 * @brief 创建丛林生物群系
 */
Biome createJungle();

/**
 * @brief 创建热带草原生物群系
 */
Biome createSavanna();

/**
 * @brief 创建恶地生物群系
 */
Biome createBadlands();

/**
 * @brief 创建海滩生物群系
 */
Biome createBeach();

/**
 * @brief 创建沼泽生物群系
 */
Biome createSwamp();

/**
 * @brief 创建河流生物群系
 */
Biome createRiver();

/**
 * @brief 创建繁茂丘陵生物群系
 */
Biome createWoodedHills();

/**
 * @brief 创建山地边缘生物群系
 */
Biome createMountainEdge();

/**
 * @brief 创建石岸生物群系
 */
Biome createStoneShore();

/**
 * @brief 创建积雪沙滩生物群系
 */
Biome createSnowyBeach();

/**
 * @brief 创建积雪平原生物群系
 */
Biome createSnowyPlains();

/**
 * @brief 创建黑森林生物群系
 */
Biome createDarkForest();

/**
 * @brief 创建桦木森林生物群系
 */
Biome createBirchForest();

/**
 * @brief 创建巨型针叶林生物群系
 */
Biome createGiantTreeTaiga();

/**
 * @brief 创建繁茂山地生物群系
 */
Biome createWoodedMountains();

/**
 * @brief 创建热带高原生物群系
 */
Biome createSavannaPlateau();

/**
 * @brief 创建恶地高原生物群系
 */
Biome createBadlandsPlateau();

/**
 * @brief 创建繁茂恶地高原生物群系
 */
Biome createWoodedBadlandsPlateau();

/**
 * @brief 创建风蚀恶地生物群系
 */
Biome createErodedBadlands();

/**
 * @brief 创建破碎热带草原生物群系
 */
Biome createShatteredSavanna();

/**
 * @brief 创建积雪泰加林生物群系
 */
Biome createSnowyTaiga();

} // namespace BiomeFactory

} // namespace mr
