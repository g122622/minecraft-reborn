#include "EntityRendererManager.hpp"
#include "AnimalRenderers.hpp"
#include "../../../common/entity/Entity.hpp"

namespace mr::client::renderer {

EntityRendererManager::EntityRendererManager()
{
    initializeDefaults();
}

void EntityRendererManager::registerRenderer(const String& typeId, RendererCreator creator) {
    m_creators[typeId] = std::move(creator);
}

EntityRenderer* EntityRendererManager::getRenderer(const String& typeId) {
    auto it = m_renderers.find(typeId);
    if (it != m_renderers.end()) {
        return it->second.get();
    }
    return nullptr;
}

void EntityRendererManager::render(Entity& entity, f32 partialTicks) {
    // 获取实体类型ID并查找渲染器
    String typeId = entity.getTypeId();
    EntityRenderer* renderer = getOrCreateRenderer(typeId);
    if (renderer) {
        renderer->render(entity, partialTicks);
        if (m_renderShadows) {
            renderer->renderShadow(entity, partialTicks);
        }
        if (m_renderNameTags) {
            renderer->renderNameTag(entity);
        }
    }
}

void EntityRendererManager::initializeDefaults() {
    // 注册动物渲染器
    registerRenderer("minecraft:pig", []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<PigRenderer>();
    });
    registerRenderer("minecraft:cow", []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<CowRenderer>();
    });
    registerRenderer("minecraft:sheep", []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<SheepRenderer>();
    });
    registerRenderer("minecraft:chicken", []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<ChickenRenderer>();
    });
}

EntityRenderer* EntityRendererManager::getOrCreateRenderer(const String& typeId) {
    // 先查找已创建的渲染器
    auto it = m_renderers.find(typeId);
    if (it != m_renderers.end()) {
        return it->second.get();
    }

    // 查找创建函数
    auto creatorIt = m_creators.find(typeId);
    if (creatorIt == m_creators.end()) {
        return nullptr;
    }

    // 创建渲染器
    auto renderer = creatorIt->second();
    EntityRenderer* ptr = renderer.get();
    m_renderers[typeId] = std::move(renderer);
    return ptr;
}

} // namespace mr::client::renderer
