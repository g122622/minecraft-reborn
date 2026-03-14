#include "EntityRendererManager.hpp"
#include "AnimalRenderers.hpp"
#include "../../world/entity/ClientEntity.hpp"
#include "../../../common/entity/EntityRegistry.hpp"
#include "../../../common/math/MathUtils.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::renderer {

namespace {

/**
 * @brief 规范化实体类型ID
 *
 * 将实体类型ID转换为标准格式（带命名空间前缀）
 * 例如："pig" -> "minecraft:pig", "minecraft:cow" -> "minecraft:cow"
 */
String normalizeEntityTypeId(const String& typeId) {
    // 如果已有命名空间前缀，直接返回
    if (typeId.find(':') != String::npos) {
        return typeId;
    }
    // 添加默认命名空间
    return "minecraft:" + typeId;
}

} // anonymous namespace

EntityRendererManager::EntityRendererManager()
{
}

EntityRendererManager::~EntityRendererManager() {
    // 销毁所有实体网格的Vulkan资源
    clearMeshes();
}

void EntityRendererManager::clearMeshes() {
    if (m_pipeline) {
        for (auto& [id, mesh] : m_meshes) {
            m_pipeline->destroyMesh(mesh);
        }
    }
    m_meshes.clear();
}

void EntityRendererManager::registerRenderer(const String& typeId, RendererCreator creator) {
    m_creators[typeId] = std::move(creator);
}

EntityRenderer* EntityRendererManager::getRenderer(const String& typeId) {
    String normalizedId = normalizeEntityTypeId(typeId);
    auto it = m_renderers.find(normalizedId);
    if (it != m_renderers.end()) {
        return it->second.get();
    }
    return nullptr;
}

void EntityRendererManager::render(Entity& entity, f32 partialTicks) {
    // 获取实体类型ID并查找渲染器（已在 getOrCreateRenderer 中规范化）
    EntityRenderer* renderer = getOrCreateRenderer(entity.getTypeId());
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
    // 使用 EntityTypes 常量注册渲染器，避免重复注册
    // 所有注册都使用规范化的命名空间格式
    namespace ET = entity::EntityTypes;

    registerRenderer(ET::PIG, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<PigRenderer>();
    });
    registerRenderer(ET::COW, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<CowRenderer>();
    });
    registerRenderer(ET::SHEEP, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<SheepRenderer>();
    });
    registerRenderer(ET::CHICKEN, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<ChickenRenderer>();
    });
}

EntityRenderer* EntityRendererManager::getOrCreateRenderer(const String& typeId) {
    // 规范化实体类型ID
    String normalizedId = normalizeEntityTypeId(typeId);

    // 先查找已创建的渲染器
    auto it = m_renderers.find(normalizedId);
    if (it != m_renderers.end()) {
        return it->second.get();
    }

    // 查找创建函数
    auto creatorIt = m_creators.find(normalizedId);
    if (creatorIt == m_creators.end()) {
        return nullptr;
    }

    // 创建渲染器
    auto renderer = creatorIt->second();
    EntityRenderer* ptr = renderer.get();
    m_renderers[normalizedId] = std::move(renderer);
    return ptr;
}

bool EntityRendererManager::generateModelMesh(const String& typeId,
                                               std::vector<ModelVertex>& vertices,
                                               std::vector<u32>& indices) {
    // 规范化实体类型ID，统一使用命名空间格式进行比较
    String normalizedId = normalizeEntityTypeId(typeId);

    // 使用 EntityTypes 常量进行比较
    namespace ET = entity::EntityTypes;

    if (normalizedId == ET::PIG) {
        PigModel model;
        model.generateMesh(vertices, indices);
        return true;
    }
    if (normalizedId == ET::COW) {
        CowModel model;
        model.generateMesh(vertices, indices);
        return true;
    }
    if (normalizedId == ET::SHEEP) {
        SheepModel model;
        model.generateMesh(vertices, indices);
        return true;
    }
    if (normalizedId == ET::CHICKEN) {
        ChickenModel model;
        model.generateMesh(vertices, indices);
        return true;
    }

    // 未知实体类型
    spdlog::debug("Unknown entity type for mesh generation: {}", normalizedId);
    return false;
}

} // namespace mc::client::renderer
