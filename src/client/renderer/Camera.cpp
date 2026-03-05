#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace mr::client {

// ============================================================================
// Camera 实现
// ============================================================================

Camera::Camera(const CameraConfig& config)
    : m_config(config)
{
    updateVectors();
    updateProjectionMatrix();
}

void Camera::update(f32 deltaTime) {
    (void)deltaTime; // 暂时不使用

    if (m_viewDirty) {
        updateViewMatrix();
    }
    if (m_projectionDirty) {
        updateProjectionMatrix();
    }
    if (m_viewDirty || m_projectionDirty) {
        updateViewProjectionMatrix();
        m_dirty = true;
        m_viewDirty = false;
        m_projectionDirty = false;
    }
}

void Camera::setPosition(const glm::vec3& position) {
    m_position = position;
    m_viewDirty = true;
}

void Camera::setPosition(f32 x, f32 y, f32 z) {
    m_position = glm::vec3(x, y, z);
    m_viewDirty = true;
}

void Camera::setRotation(const glm::vec3& rotation) {
    m_rotation = rotation;
    // 限制俯仰角
    m_rotation.x = math::clamp(m_rotation.x, -m_config.pitchLimit, m_config.pitchLimit);
    updateVectors();
    m_viewDirty = true;
}

void Camera::setRotation(f32 pitch, f32 yaw, f32 roll) {
    m_rotation = glm::vec3(pitch, yaw, roll);
    m_rotation.x = math::clamp(m_rotation.x, -m_config.pitchLimit, m_config.pitchLimit);
    updateVectors();
    m_viewDirty = true;
}

void Camera::setPitch(f32 pitch) {
    m_rotation.x = math::clamp(pitch, -m_config.pitchLimit, m_config.pitchLimit);
    updateVectors();
    m_viewDirty = true;
}

void Camera::setYaw(f32 yaw) {
    m_rotation.y = yaw;
    updateVectors();
    m_viewDirty = true;
}

void Camera::setRoll(f32 roll) {
    m_rotation.z = roll;
    updateVectors();
    m_viewDirty = true;
}

glm::vec3 Camera::forward() const {
    return m_forward;
}

glm::vec3 Camera::right() const {
    return m_right;
}

glm::vec3 Camera::up() const {
    return m_up;
}

void Camera::moveForward(f32 distance) {
    // 水平移动（忽略Y分量）
    m_position.x += m_forward.x * distance;
    m_position.z += m_forward.z * distance;
    m_viewDirty = true;
}

void Camera::moveRight(f32 distance) {
    m_position += m_right * distance;
    m_viewDirty = true;
}

void Camera::moveUp(f32 distance) {
    m_position.y += distance;
    m_viewDirty = true;
}

void Camera::rotate(f32 pitchDelta, f32 yawDelta) {
    m_rotation.x = math::clamp(m_rotation.x + pitchDelta, -m_config.pitchLimit, m_config.pitchLimit);
    m_rotation.y += yawDelta;
    updateVectors();
    m_viewDirty = true;
}

void Camera::look(f32 mouseDeltaX, f32 mouseDeltaY) {
    // 应用鼠标灵敏度和方向
    // 鼠标右移 -> yaw 增大 -> 视角右转
    f32 yawDelta = mouseDeltaX * m_config.mouseSensitivity;
    // 鼠标上移 -> pitch 增大 -> 视角上抬
    f32 pitchDelta = -mouseDeltaY * m_config.mouseSensitivity;

    rotate(pitchDelta, yawDelta);
}

void Camera::setProjectionMode(ProjectionMode mode) {
    m_config.projectionMode = mode;
    m_projectionDirty = true;
}

void Camera::setFOV(f32 fov) {
    m_config.fov = fov;
    m_projectionDirty = true;
}

void Camera::setAspectRatio(f32 aspectRatio) {
    m_config.aspectRatio = aspectRatio;
    m_projectionDirty = true;
}

void Camera::setNearFar(f32 nearPlane, f32 farPlane) {
    m_config.nearPlane = nearPlane;
    m_config.farPlane = farPlane;
    m_projectionDirty = true;
}

void Camera::setOrthoSize(f32 size) {
    m_config.orthoSize = size;
    m_projectionDirty = true;
}

void Camera::setConfig(const CameraConfig& config) {
    m_config = config;
    m_viewDirty = true;
    m_projectionDirty = true;
}

/**
 * @brief 更新方向向量
 *
 * 使用Minecraft坐标系约定：
 * - yaw=0: 看向 +Z 方向
 * - yaw=90: 看向 -X 方向
 * - yaw=180: 看向 -Z 方向
 * - yaw=270: 看向 +X 方向
 *
 * 这与Entity.getVectorForRotation()一致：
 *   forward.x = -sin(yaw) * cos(pitch)
 *   forward.z = cos(yaw) * cos(pitch)
 *
 * 参考MC源码: Entity.java:1387-1394
 */
void Camera::updateVectors() {
    // 从欧拉角计算方向向量
    f32 pitchRad = math::toRadians(m_rotation.x);
    f32 yawRad = math::toRadians(m_rotation.y);

    // 前向向量 - MC坐标系
    // MC: yaw=0 看向 +Z, yaw=90 看向 -X
    m_forward.x = -std::sin(yawRad) * std::cos(pitchRad);
    m_forward.y = std::sin(pitchRad);
    m_forward.z = std::cos(yawRad) * std::cos(pitchRad);
    m_forward = glm::normalize(m_forward);

    // 右向量和上向量
    // 假设世界上方向为Y轴正方向
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    m_right = glm::normalize(glm::cross(m_forward, worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

void Camera::updateViewMatrix() {
    // 视图矩阵：将世界坐标转换到相机空间
    m_viewMatrix = glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::updateProjectionMatrix() {
    if (m_config.projectionMode == ProjectionMode::Perspective) {
        m_projectionMatrix = glm::perspective(
            math::toRadians(m_config.fov),
            m_config.aspectRatio,
            m_config.nearPlane,
            m_config.farPlane
        );
    } else {
        // 正交投影
        f32 halfWidth = m_config.orthoSize * m_config.aspectRatio * 0.5f;
        f32 halfHeight = m_config.orthoSize * 0.5f;
        m_projectionMatrix = glm::ortho(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            m_config.nearPlane,
            m_config.farPlane
        );
    }

    // Vulkan 使用右手坐标系，Y 轴向下，需要翻转投影矩阵的 Y 轴
    m_projectionMatrix[1][1] *= -1.0f;
}

void Camera::updateViewProjectionMatrix() {
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

// ============================================================================
// CameraController 实现
// ============================================================================

CameraController::CameraController(Camera* camera)
    : m_camera(camera)
{
}

void CameraController::setCamera(Camera* camera) {
    m_camera = camera;
}

void CameraController::handleKeyboardInput(i32 key, i32 action) {
    if (!m_camera) return;

    bool pressed = (action == 1); // GLFW_PRESS

    if (key == m_moveForwardKey) {
        m_moveForward = pressed;
    } else if (key == m_moveBackwardKey) {
        m_moveBackward = pressed;
    } else if (key == m_moveLeftKey) {
        m_moveLeft = pressed;
    } else if (key == m_moveRightKey) {
        m_moveRight = pressed;
    } else if (key == m_moveUpKey) {
        m_moveUp = pressed;
    } else if (key == m_moveDownKey) {
        m_moveDown = pressed;
        m_sprinting = pressed;
    } else if (key == m_sneakKey) {
        m_sneaking = pressed;
    }
}

void CameraController::handleMouseMove(f64 deltaX, f64 deltaY) {
    if (!m_camera) return;

    m_camera->look(static_cast<f32>(deltaX), static_cast<f32>(deltaY));
}

void CameraController::handleScroll(f64 deltaY) {
    if (!m_camera) return;

    // 滚轮可以用来调整FOV或缩放
    f32 fov = m_camera->fov();
    fov -= static_cast<f32>(deltaY) * 2.0f;
    fov = math::clamp(fov, 10.0f, 120.0f);
    m_camera->setFOV(fov);
}

void CameraController::update(f32 deltaTime) {
    if (!m_camera) return;

    // 计算移动速度
    f32 speed = m_camera->moveSpeed();
    if (m_sprinting) {
        speed *= m_camera->config().sprintMultiplier;
    }
    if (m_sneaking) {
        speed *= m_camera->config().sneakMultiplier;
    }

    f32 distance = speed * deltaTime;

    // 移动
    if (m_moveForward) {
        m_camera->moveForward(distance);
    }
    if (m_moveBackward) {
        m_camera->moveForward(-distance);
    }
    if (m_moveRight) {
        m_camera->moveRight(distance);
    }
    if (m_moveLeft) {
        m_camera->moveRight(-distance);
    }
    if (m_moveUp) {
        m_camera->moveUp(distance);
    }
    if (m_moveDown) {
        m_camera->moveUp(-distance);
    }

    // 更新相机
    m_camera->update(deltaTime);

    // 更新状态
    m_moving = m_moveForward || m_moveBackward || m_moveLeft || m_moveRight
            || m_moveUp || m_moveDown;
}

} // namespace mr::client
