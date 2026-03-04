#version 450

// 区块片段着色器

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragLight;

layout(location = 0) out vec4 outColor;

// 描述符集 1 - 纹理采样器
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // fragLight 已经是归一化值（UNORM格式将0-255映射到0.0-1.0）
    // 所以直接使用它作为光照因子
    vec3 finalColor = texColor.rgb * fragLight;
    outColor = vec4(finalColor, 1.0);
}
