#version 450

// 天空穹顶片段着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染天空颜色，在地平线附近应用雾颜色渐变

// 从顶点着色器输入
layout(location = 0) in vec3 fragWorldPos;

// 输出颜色
layout(location = 0) out vec4 outColor;

// 描述符集 0 - Uniform 缓冲区
layout(set = 0, binding = 0) uniform SkyUBO {
    vec4 skyColor;
    vec4 fogColor;
    float celestialAngle;
    float starBrightness;
    int moonPhase;
    float padding;
} sky;

void main() {
    // 计算与地平线的距离因子
    // 网格在 Y=16 平面，相机在 Y≈0
    // fragWorldPos.y = 16.0 对于所有顶点
    // 使用 XZ 距离计算雾混合因子

    float dist = length(fragWorldPos.xz);
    float maxDist = 384.0; // 网格半径

    // 距离越远，雾颜色越明显
    float fogFactor = clamp(dist / maxDist, 0.0, 1.0);

    // 混合天空颜色和雾颜色
    vec3 color = mix(sky.skyColor.rgb, sky.fogColor.rgb, fogFactor);

    outColor = vec4(color, 1.0);
}
