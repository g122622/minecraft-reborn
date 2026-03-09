#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/math/MathUtils.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mr::client {

// 相机投影模式
enum class ProjectionMode : u8 {
    Perspective,
    Orthographic
};

// 相机配置
struct CameraConfig {
    // 视角设置
    f32 fov = 70.0f;                    // 视野角度（度）
    f32 aspectRatio = 16.0f / 9.0f;     // 宽高比
    f32 nearPlane = 0.1f;               // 近裁剪面
    f32 farPlane = 1000.0f;             // 远裁剪面

    // 正交投影设置
    f32 orthoSize = 10.0f;              // 正交投影大小

    // 移动设置
    f32 moveSpeed = 5.0f;               // 移动速度（单位/秒）
    f32 sprintMultiplier = 2.0f;        // 冲刺倍率
    f32 sneakMultiplier = 0.3f;         // 潜行倍率

    // 视角设置
    f32 mouseSensitivity = 0.1f;        // 鼠标灵敏度
    f32 pitchLimit = 89.0f;             // 俯仰角限制（度）

    // 默认投影模式
    ProjectionMode projectionMode = ProjectionMode::Perspective;
};

// 相机类 - 第一人称视角
class Camera {
public:
    Camera() = default;
    explicit Camera(const CameraConfig& config);

    // 更新
    void update(f32 deltaTime);

    // 位置
    void setPosition(const glm::vec3& position);
    void setPosition(f32 x, f32 y, f32 z);
    [[nodiscard]] const glm::vec3& position() const { return m_position; }

    // 旋转（欧拉角，度）
    void setRotation(const glm::vec3& rotation);
    void setRotation(f32 pitch, f32 yaw, f32 roll = 0.0f);
    [[nodiscard]] const glm::vec3& rotation() const { return m_rotation; }

    // 俯仰和偏航（更常用）
    [[nodiscard]] f32 pitch() const { return m_rotation.x; }
    [[nodiscard]] f32 yaw() const { return m_rotation.y; }
    [[nodiscard]] f32 roll() const { return m_rotation.z; }

    void setPitch(f32 pitch);
    void setYaw(f32 yaw);
    void setRoll(f32 roll);

    // 方向向量
    [[nodiscard]] glm::vec3 forward() const;
    [[nodiscard]] glm::vec3 right() const;
    [[nodiscard]] glm::vec3 up() const;

    // 移动
    void moveForward(f32 distance);
    void moveRight(f32 distance);
    void moveUp(f32 distance);

    // 旋转
    void rotate(f32 pitchDelta, f32 yawDelta);

    // 视角控制（通过鼠标增量）
    void look(f32 mouseDeltaX, f32 mouseDeltaY);

    // 投影
    void setProjectionMode(ProjectionMode mode);
    void setFOV(f32 fov);
    void setAspectRatio(f32 aspectRatio);
    void setNearFar(f32 nearPlane, f32 farPlane);
    void setOrthoSize(f32 size);

    [[nodiscard]] ProjectionMode projectionMode() const { return m_config.projectionMode; }
    [[nodiscard]] f32 fov() const { return m_config.fov; }
    [[nodiscard]] f32 aspectRatio() const { return m_config.aspectRatio; }
    [[nodiscard]] f32 nearPlane() const { return m_config.nearPlane; }
    [[nodiscard]] f32 farPlane() const { return m_config.farPlane; }

    // 矩阵
    [[nodiscard]] const glm::mat4& viewMatrix() const { return m_viewMatrix; }
    [[nodiscard]] const glm::mat4& projectionMatrix() const { return m_projectionMatrix; }
    [[nodiscard]] const glm::mat4& viewProjectionMatrix() const { return m_viewProjectionMatrix; }

    // 配置
    void setConfig(const CameraConfig& config);
    [[nodiscard]] const CameraConfig& config() const { return m_config; }

    // 移动速度
    void setMoveSpeed(f32 speed) { m_config.moveSpeed = speed; }
    [[nodiscard]] f32 moveSpeed() const { return m_config.moveSpeed; }

    // 鼠标灵敏度
    void setMouseSensitivity(f32 sensitivity) { m_config.mouseSensitivity = sensitivity; }
    [[nodiscard]] f32 mouseSensitivity() const { return m_config.mouseSensitivity; }

    // 脏标记
    [[nodiscard]] bool isDirty() const { return m_dirty; }
    void markClean() { m_dirty = false; }

private:
    void updateVectors();
    void updateViewMatrix();
    void updateProjectionMatrix();
    void updateViewProjectionMatrix();

    CameraConfig m_config;

    // 位置和旋转
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::vec3 m_rotation{0.0f, 0.0f, 0.0f}; // pitch, yaw, roll (度)

    // 方向向量
    glm::vec3 m_forward{0.0f, 0.0f, -1.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};

    // 矩阵
    glm::mat4 m_viewMatrix{1.0f};
    glm::mat4 m_projectionMatrix{1.0f};
    glm::mat4 m_viewProjectionMatrix{1.0f};

    // 脏标记
    bool m_dirty = true;
    bool m_viewDirty = true;
    bool m_projectionDirty = true;
};

// 相机控制器 - 处理输入
class CameraController {
public:
    CameraController() = default;
    explicit CameraController(Camera* camera);

    // 设置控制的相机
    void setCamera(Camera* camera);
    [[nodiscard]] Camera* camera() { return m_camera; }

    // 输入处理
    void handleKeyboardInput(i32 key, i32 action);
    void handleMouseMove(f32 deltaX, f32 deltaY);
    void handleScroll(f32 deltaY);

    // 更新
    void update(f32 deltaTime);

    // 设置按键映射
    void setMoveForwardKey(i32 key) { m_moveForwardKey = key; }
    void setMoveBackwardKey(i32 key) { m_moveBackwardKey = key; }
    void setMoveLeftKey(i32 key) { m_moveLeftKey = key; }
    void setMoveRightKey(i32 key) { m_moveRightKey = key; }
    void setMoveUpKey(i32 key) { m_moveUpKey = key; }
    void setMoveDownKey(i32 key) { m_moveDownKey = key; }
    void setSprintKey(i32 key) { m_sprintKey = key; }
    void setSneakKey(i32 key) { m_sneakKey = key; }

    // 状态
    [[nodiscard]] bool isMoving() const { return m_moving; }
    [[nodiscard]] bool isSprinting() const { return m_sprinting; }
    [[nodiscard]] bool isSneaking() const { return m_sneaking; }

private:
    Camera* m_camera = nullptr;

    // 按键状态
    bool m_moveForward = false;
    bool m_moveBackward = false;
    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_moveUp = false;
    bool m_moveDown = false;
    bool m_sprinting = false;
    bool m_sneaking = false;

    // 按键映射 (默认WASD + Space + Shift)
    i32 m_moveForwardKey = 87;   // W
    i32 m_moveBackwardKey = 83;  // S
    i32 m_moveLeftKey = 65;      // A
    i32 m_moveRightKey = 68;     // D
    i32 m_moveUpKey = 32;        // Space
    i32 m_moveDownKey = 340;     // Left Shift
    i32 m_sprintKey = 340;       // Left Shift (与moveDown共享)
    i32 m_sneakKey = 341;        // Left Ctrl

    // 状态
    bool m_moving = false;
};

} // namespace mr::client
