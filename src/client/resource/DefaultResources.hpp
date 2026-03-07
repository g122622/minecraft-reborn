#pragma once

#include "../common/core/Types.hpp"
#include "../common/resource/ResourceLocation.hpp"
#include "../renderer/MeshTypes.hpp"
#include "ResourceManager.hpp"
#include <vector>
#include <map>
#include <memory>

namespace mr {

/**
 * @brief 内置默认资源
 *
 * 提供程序化生成的默认模型和纹理，在没有资源包时作为 fallback。
 * 类似 Minecraft 的 "missing" 模型（紫黑方块）。
 *
 * 使用示例:
 * @code
 * // 创建默认资源
 * DefaultResources::initialize();
 *
 * // 获取缺失纹理（紫黑方块）
 * const auto& missingTexture = DefaultResources::getMissingTexturePixels();
 *
 * // 获取缺失模型外观
 * const BlockAppearance* appearance = DefaultResources::getMissingAppearance();
 * @endcode
 */
class DefaultResources {
public:
    /**
     * @brief 初始化默认资源
     *
     * 生成缺失纹理、缺失模型等。
     */
    static void initialize();

    /**
     * @brief 清理默认资源
     */
    static void shutdown();

    // ========================================================================
    // 缺失纹理
    // ========================================================================

    /**
     * @brief 获取缺失纹理像素数据
     *
     * 返回 16x16 的紫黑方块纹理（RGBA 格式）。
     *
     * @return 像素数据指针（RGBA 格式，16x16）
     */
    [[nodiscard]] static const std::vector<u8>& getMissingTexturePixels();

    /**
     * @brief 获取缺失纹理宽度
     */
    [[nodiscard]] static constexpr u32 getMissingTextureWidth() { return 16; }

    /**
     * @brief 获取缺失纹理高度
     */
    [[nodiscard]] static constexpr u32 getMissingTextureHeight() { return 16; }

    /**
     * @brief 获取缺失纹理资源位置
     */
    [[nodiscard]] static ResourceLocation getMissingTextureLocation();

    // ========================================================================
    // 缺失模型
    // ========================================================================

    /**
     * @brief 获取缺失模型外观
     *
     * 返回一个紫黑方块的 BlockAppearance。
     *
     * @return 缺失模型外观指针
     */
    [[nodiscard]] static const BlockAppearance* getMissingAppearance();

    /**
     * @brief 获取缺失模型资源位置
     */
    [[nodiscard]] static ResourceLocation getMissingModelLocation();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

private:
    // 生成缺失纹理
    static void generateMissingTexture();

    // 生成缺失模型
    static void generateMissingAppearance();

    // 已初始化
    static bool s_initialized;

    // 缺失纹理数据（16x16 RGBA）
    static std::vector<u8> s_missingTexturePixels;

    // 缺失模型外观
    static std::unique_ptr<BlockAppearance> s_missingAppearance;
};

} // namespace mr
