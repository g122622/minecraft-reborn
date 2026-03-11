#pragma once

#include "EntityRenderer.hpp"
#include "../../../common/core/Types.hpp"
#include <unordered_map>
#include <memory>
#include <functional>

namespace mr {

class Entity;

namespace client::renderer {

/**
 * @brief 实体渲染器管理器
 *
 * 管理所有实体渲染器，根据实体类型分派渲染。
 *
 * 参考 MC 1.16.5 EntityRendererManager
 */
class EntityRendererManager {
public:
    using RendererCreator = std::function<std::unique_ptr<EntityRenderer>()>;

    EntityRendererManager();
    ~EntityRendererManager() = default;

    // ========== 渲染器管理 ==========

    /**
     * @brief 注册渲染器
     * @param typeId 实体类型ID
     * @param creator 渲染器创建函数
     */
    void registerRenderer(const String& typeId, RendererCreator creator);

    /**
     * @brief 获取渲染器
     * @param typeId 实体类型ID
     * @return 渲染器指针，如果未注册返回nullptr
     */
    [[nodiscard]] EntityRenderer* getRenderer(const String& typeId);

    /**
     * @brief 渲染实体
     * @param entity 要渲染的实体
     * @param partialTicks 部分tick
     */
    void render(Entity& entity, f32 partialTicks);

    // ========== 渲染设置 ==========

    /**
     * @brief 设置是否渲染阴影
     */
    void setRenderShadows(bool render) { m_renderShadows = render; }

    /**
     * @brief 获取是否渲染阴影
     */
    [[nodiscard]] bool renderShadows() const { return m_renderShadows; }

    /**
     * @brief 设置是否渲染名称标签
     */
    void setRenderNameTags(bool render) { m_renderNameTags = render; }

    /**
     * @brief 获取是否渲染名称标签
     */
    [[nodiscard]] bool renderNameTags() const { return m_renderNameTags; }

    // ========== 初始化 ==========

    /**
     * @brief 初始化默认渲染器
     */
    void initializeDefaults();

private:
    std::unordered_map<String, std::unique_ptr<EntityRenderer>> m_renderers;
    std::unordered_map<String, RendererCreator> m_creators;

    bool m_renderShadows = true;
    bool m_renderNameTags = true;

    /**
     * @brief 创建或获取渲染器
     */
    [[nodiscard]] EntityRenderer* getOrCreateRenderer(const String& typeId);
};

} // namespace client::renderer
} // namespace mr
