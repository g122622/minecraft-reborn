#pragma once

#include "../../../../common/core/Types.hpp"

namespace mc::client::renderer::api {

/**
 * @brief 相机投影模式
 */
enum class ProjectionMode : u8 {
    Perspective,   // 透视投影
    Orthographic   // 正交投影
};

/**
 * @brief 相机配置
 */
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

} // namespace mc::client::renderer::api
