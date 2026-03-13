#version 450

// 实体片段着色器

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragLight;

layout(location = 0) out vec4 outColor;

// 描述符集 1 - 纹理采样器
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // 如果纹理alpha太低，丢弃片段（透明）
    if (texColor.a < 0.1) {
        discard;
    }

    // 应用光照
    vec3 finalColor = texColor.rgb * fragLight * fragColor.rgb;

    outColor = vec4(finalColor, texColor.a * fragColor.a);
}
