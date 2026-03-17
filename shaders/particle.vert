#version 450

// 粒子顶点着色器
// 参考 MC 1.16.5 粒子渲染

// 顶点输入
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;
layout(location = 3) in float inSize;
layout(location = 4) in float inAlpha;

// Uniform 缓冲区
layout(set = 0, binding = 0) uniform ParticleUBO {
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
    float partialTick;
} ubo;

// 输出到片段着色器
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

void main() {
    fragTexCoord = inTexCoord;
    fragColor = vec4(inColor.rgb, inColor.a * inAlpha);

    gl_Position = ubo.projection * ubo.view * vec4(inPosition, 1.0);
}
