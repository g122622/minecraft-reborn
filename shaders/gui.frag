#version 450

// GUI片段着色器
// 用于渲染2D GUI元素，如文本和矩形

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

// 描述符集 0 - 字体纹理采样器
layout(set = 0, binding = 0) uniform sampler2D fontSampler;

void main() {
    // 字体纹理使用R8格式，红色通道包含灰度值
    float alpha = texture(fontSampler, fragTexCoord).r;

    // 输出: 颜色 * 灰度值作为alpha
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
