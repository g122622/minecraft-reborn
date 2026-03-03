#version 450

// 简单顶点着色器 - 用于调试

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 chunkOffset;
} pc;

void main() {
    vec3 worldPos = inPosition + pc.chunkOffset;
    gl_Position = camera.viewProjection * vec4(worldPos, 1.0);

    // 使用法线作为颜色进行可视化
    fragColor = normalize(inNormal) * 0.5 + 0.5;
}
