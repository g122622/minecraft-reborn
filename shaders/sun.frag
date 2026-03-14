#version 450

// 太阳片段着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染太阳圆盘，使用程序化圆盘而非纹理

// 从顶点着色器输入
layout(location = 0) in vec2 fragTexCoord;

// 输出颜色
layout(location = 0) out vec4 outColor;

void main() {
    // 计算到中心的距离
    vec2 center = fragTexCoord - 0.5;
    float dist = length(center) * 2.0; // 归一化到 [0, 1]

    // 圆盘半径 0.5 (UV 空间中)
    // 使用平滑边缘
    float radius = 0.5;
    float edgeSoftness = 0.05;

    // 平滑的圆盘 alpha
    float alpha = 1.0 - smoothstep(radius - edgeSoftness, radius, dist);

    // 如果完全透明则丢弃
    if (alpha < 0.01) {
        discard;
    }

    // 太阳颜色 - 明亮的黄色/白色
    // MC 中的太阳纹理是正方形，中心是白色/黄色
    vec3 sunColorInner = vec3(1.0, 0.95, 0.8); // 略带黄白色
    vec3 sunColorEdge = vec3(1.0, 0.85, 0.6);  // 边缘更黄

    // 径向渐变
    vec3 color = mix(sunColorInner, sunColorEdge, dist);

    outColor = vec4(color, alpha);
}
