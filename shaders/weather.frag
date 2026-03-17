#version 450

// 天气片段着色器
// 参考 MC 1.16.5 WorldRenderer.renderRainSnow()

// 从顶点着色器输入
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec2 fragLightmap;

// 纹理采样器
layout(set = 0, binding = 1) uniform sampler2D texSampler;

// 输出颜色
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // 应用顶点颜色和透明度
    outColor = texColor * fragColor;

    // 如果 alpha 太低，丢弃片段
    if (outColor.a < 0.01) {
        discard;
    }

    // 应用光照（简化处理，实际需要采样光照贴图）
    float light = (fragLightmap.x + fragLightmap.y) * 0.5 / 240.0;
    light = clamp(light, 0.3, 1.0);
    outColor.rgb *= light;
}
