#version 450

// 区块片段着色器

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragLight;
layout(location = 4) in vec3 fragWorldPos;
layout(location = 5) in float fragViewDistance;

layout(location = 0) out vec4 outColor;

// 描述符集 1 - 纹理采样器
layout(set = 1, binding = 0) uniform sampler2D texSampler;

// 描述符集 2 - 雾 UBO
layout(set = 2, binding = 0) uniform FogUBO {
    float fogStart;
    float fogEnd;
    float fogDensity;
    int fogMode;
    vec4 fogColor;
} fog;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // fragLight 已经是归一化值（UNORM格式将0-255映射到0.0-1.0）
    // 所以直接使用它作为光照因子
    vec3 finalColor = texColor.rgb * fragLight;

    // 雾计算
    // fogMode: 0 = 禁用, 1 = 线性雾, 2 = 指数雾
    if (fog.fogMode == 1) {
        // 线性雾（陆地）
        float distance = fragViewDistance;
        float fogFactor = clamp((fog.fogEnd - distance) / (fog.fogEnd - fog.fogStart), 0.0, 1.0);
        finalColor = mix(fog.fogColor.rgb, finalColor, fogFactor);
    } else if (fog.fogMode == 2) {
        // 指数雾（水中/岩浆）
        float fogFactor = 1.0 - exp(-fog.fogDensity * fragViewDistance);
        finalColor = mix(fog.fogColor.rgb, finalColor, clamp(fogFactor, 0.0, 1.0));
    }

    outColor = vec4(finalColor, 1.0);
}
