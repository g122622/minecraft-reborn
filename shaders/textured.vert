#version 450

// 纹理测试顶点着色器

// 顶点输入
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in float inLight;

// 输出到片段着色器
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

// 推送常量 - 模型矩阵
layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

// 描述符集 0 - 相机UBO
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} camera;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = camera.viewProjection * pc.model * vec4(inPosition, 1.0);
    fragColor = inColor.rgb;
    fragTexCoord = inTexCoord;
}
