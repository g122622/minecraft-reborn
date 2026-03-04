#version 450

// 纹理测试片段着色器

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// 描述符集 1 - 纹理采样器
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    // 混合顶点颜色和纹理颜色
    outColor = vec4(fragColor * texColor.rgb, texColor.a);
}
