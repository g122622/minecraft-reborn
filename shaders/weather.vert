#version 450

// 天气顶点着色器
// 参考 MC 1.16.5 WorldRenderer.renderRainSnow()

// 顶点输入
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inLightmap;

// Uniform 缓冲区
layout(set = 0, binding = 0) uniform WeatherUBO {
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
    float partialTick;
    float rainStrength;
    float thunderStrength;
} ubo;

// 输出到片段着色器
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 fragLightmap;

void main() {
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragLightmap = inLightmap;

    // 相对于相机的位置
    vec3 worldPos = inPosition + ubo.cameraPos;

    gl_Position = ubo.projection * ubo.view * vec4(worldPos, 1.0);
}
