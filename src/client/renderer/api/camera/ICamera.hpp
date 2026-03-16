#pragma once

#include "CameraConfig.hpp"
#include "../../../../common/math/MathUtils.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mc::client::renderer::api {

/**
 * @brief 相机接口
 *
 * 平台无关的相机抽象接口。
 * 提供视图和投影矩阵计算。
 */
class ICamera {
public:
    virtual ~ICamera() = default;

    // 更新
    virtual void update(f32 deltaTime) = 0;

    // 位置
    virtual void setPosition(const glm::vec3& position) = 0;
    virtual void setPosition(f32 x, f32 y, f32 z) = 0;
    [[nodiscard]] virtual const glm::vec3& position() const = 0;

    // 旋转（欧拉角，度）
    virtual void setRotation(const glm::vec3& rotation) = 0;
    virtual void setRotation(f32 pitch, f32 yaw, f32 roll = 0.0f) = 0;
    [[nodiscard]] virtual const glm::vec3& rotation() const = 0;

    // 俯仰和偏航
    [[nodiscard]] virtual f32 pitch() const = 0;
    [[nodiscard]] virtual f32 yaw() const = 0;
    [[nodiscard]] virtual f32 roll() const = 0;

    virtual void setPitch(f32 pitch) = 0;
    virtual void setYaw(f32 yaw) = 0;
    virtual void setRoll(f32 roll) = 0;

    // 方向向量
    [[nodiscard]] virtual glm::vec3 forward() const = 0;
    [[nodiscard]] virtual glm::vec3 right() const = 0;
    [[nodiscard]] virtual glm::vec3 up() const = 0;

    // 移动
    virtual void moveForward(f32 distance) = 0;
    virtual void moveRight(f32 distance) = 0;
    virtual void moveUp(f32 distance) = 0;

    // 旋转
    virtual void rotate(f32 pitchDelta, f32 yawDelta) = 0;
    virtual void look(f32 mouseDeltaX, f32 mouseDeltaY) = 0;

    // 投影
    virtual void setProjectionMode(ProjectionMode mode) = 0;
    virtual void setFOV(f32 fov) = 0;
    virtual void setAspectRatio(f32 aspectRatio) = 0;
    virtual void setNearFar(f32 nearPlane, f32 farPlane) = 0;
    virtual void setOrthoSize(f32 size) = 0;

    [[nodiscard]] virtual ProjectionMode projectionMode() const = 0;
    [[nodiscard]] virtual f32 fov() const = 0;
    [[nodiscard]] virtual f32 aspectRatio() const = 0;
    [[nodiscard]] virtual f32 nearPlane() const = 0;
    [[nodiscard]] virtual f32 farPlane() const = 0;

    // 矩阵
    [[nodiscard]] virtual const glm::mat4& viewMatrix() const = 0;
    [[nodiscard]] virtual const glm::mat4& projectionMatrix() const = 0;
    [[nodiscard]] virtual const glm::mat4& viewProjectionMatrix() const = 0;

    // 配置
    virtual void setConfig(const CameraConfig& config) = 0;
    [[nodiscard]] virtual const CameraConfig& config() const = 0;

    // 移动速度
    virtual void setMoveSpeed(f32 speed) = 0;
    [[nodiscard]] virtual f32 moveSpeed() const = 0;

    // 鼠标灵敏度
    virtual void setMouseSensitivity(f32 sensitivity) = 0;
    [[nodiscard]] virtual f32 mouseSensitivity() const = 0;

    // 脏标记
    [[nodiscard]] virtual bool isDirty() const = 0;
    virtual void markClean() = 0;
};

} // namespace mc::client::renderer::api
