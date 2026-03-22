#version 450

// 方块破坏效果顶点着色器
// 用于渲染挖掘过程中的碎裂纹理覆盖层

// 顶点输入 - 简化的顶点格式（只需要位置和UV）
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

// 输出到片段着色器
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out float fragViewDistance;

// 描述符集 0 - 相机UBO
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} camera;

// 推送常量 - 方块位置
layout(push_constant) uniform PushConstants {
    vec3 blockPos;      // 方块在世界空间中的位置
    float damageStage;  // 破坏阶段 (0-9)，用于选择纹理
} pc;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // 方块顶点相对于方块位置的偏移
    // inPosition 已经是相对于方块中心的顶点（0-1范围）
    vec3 worldPos = inPosition + pc.blockPos;

    gl_Position = camera.viewProjection * vec4(worldPos, 1.0);
    fragTexCoord = inTexCoord;
    fragWorldPos = worldPos;
    fragViewDistance = length(gl_Position.xyz);
}
