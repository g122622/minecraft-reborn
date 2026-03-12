#include "EntityRendererManager.hpp"
#include "AnimalRenderers.hpp"
#include "../../world/entity/ClientEntity.hpp"
#include "../../../common/entity/Entity.hpp"
#include "../../../common/math/MathUtils.hpp"
#include <spdlog/spdlog.h>

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

void EntityRendererManager::renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f32 partialTicks) {
    if (!m_pipeline) {
        return;
    }

    // 获取或创建网格
    EntityMesh* mesh = getOrCreateMesh(entity);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 绑定管线
    m_pipeline->bind(cmd);

    // 绑定纹理描述符
    m_pipeline->bindTextureDescriptor(cmd);

    // 计算模型矩阵（单位矩阵，旋转由实体yaw/pitch控制）
    std::array<f32, 16> modelMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // 应用实体旋转（yaw）
    f32 yaw = entity.getInterpolatedYaw(partialTicks);
    f32 yawRad = math::toRadians(yaw);
    f32 cosYaw = std::cos(yawRad);
    f32 sinYaw = std::sin(yawRad);

    // 旋转矩阵（绕Y轴）
    modelMatrix[0] = cosYaw;
    modelMatrix[2] = sinYaw;
    modelMatrix[8] = -sinYaw;
    modelMatrix[10] = cosYaw;

    // 获取插值位置
    Vector3 posInterp = entity.getInterpolatedPosition(partialTicks);
    Vector3f pos(posInterp.x, posInterp.y, posInterp.z);

    // 绘制网格
    m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos);
}

EntityMesh* EntityRendererManager::getOrCreateMesh(ClientEntity& entity) {
    EntityId id = entity.id();
    auto it = m_meshes.find(id);

    if (it != m_meshes.end()) {
        return &it->second;
    }

    // 生成新网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (!generateModelMesh(entity.typeId(), vertices, indices)) {
        return nullptr;
    }

    // 创建GPU网格
    if (!m_pipeline) {
        return nullptr;
    }

    auto result = m_pipeline->createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("Failed to create mesh for entity {}: {}", id, result.error().toString());
        return nullptr;
    }

    EntityMesh mesh = std::move(result.value());
    mesh.posX = entity.x();
    mesh.posY = entity.y();
    mesh.posZ = entity.z();

    m_meshes[id] = std::move(mesh);
    return &m_meshes[id];
}

void EntityRendererManager::updateMesh(ClientEntity& entity) {
    EntityId id = entity.id();
    auto it = m_meshes.find(id);

    if (it == m_meshes.end()) {
        return;
    }

    // 重新生成网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (!generateModelMesh(entity.typeId(), vertices, indices)) {
        return;
    }

    (void)m_pipeline->updateMesh(it->second, vertices, indices);
}

void EntityRendererManager::removeMesh(EntityId entityId) {
    auto it = m_meshes.find(entityId);
    if (it != m_meshes.end()) {
        if (m_pipeline) {
            m_pipeline->destroyMesh(it->second);
        }
        m_meshes.erase(it);
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

    // 兼容性：同时注册不带命名空间的ID
    registerRenderer("pig", []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<PigRenderer>();
    });
    registerRenderer("cow", []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<CowRenderer>();
    });
    registerRenderer("sheep", []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<SheepRenderer>();
    });
    registerRenderer("chicken", []() -> std::unique_ptr<EntityRenderer> {
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

bool EntityRendererManager::generateModelMesh(const String& typeId,
                                               std::vector<ModelVertex>& vertices,
                                               std::vector<u32>& indices) {
    // 根据实体类型创建模型并生成网格
    if (typeId == "minecraft:pig" || typeId == "pig") {
        PigModel model;
        model.generateMesh(vertices, indices);
        return true;
    }
    else if (typeId == "minecraft:cow" || typeId == "cow") {
        CowModel model;
        model.generateMesh(vertices, indices);
        return true;
    }
    else if (typeId == "minecraft:sheep" || typeId == "sheep") {
        SheepModel model;
        model.generateMesh(vertices, indices);
        return true;
    }
    else if (typeId == "minecraft:chicken" || typeId == "chicken") {
        ChickenModel model;
        model.generateMesh(vertices, indices);
        return true;
    }

    // 未知实体类型
    spdlog::debug("Unknown entity type for mesh generation: {}", typeId);
    return false;
}

} // namespace mr::client::renderer
