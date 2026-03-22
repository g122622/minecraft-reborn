#version 450

// 方块破坏效果片段着色器
// 使用特殊的叠加混合模式（DST_COLOR * SRC_COLOR）
// 破坏纹理为黑色裂纹，alpha通道控制可见度

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in float fragViewDistance;

layout(location = 0) out vec4 outColor;

// 描述符集 1 - 破坏纹理图集采样器
// 图集布局：2行5列，共10个阶段（destroy_stage_0 ~ destroy_stage_9）
layout(set = 1, binding = 0) uniform sampler2D breakAtlas;

// 描述符集 2 - 雾 UBO
layout(set = 2, binding = 0) uniform FogUBO {
    float fogStart;
    float fogEnd;
    float fogDensity;
    int fogMode;
    vec4 fogColor;
} fog;

// 推送常量 - 与顶点着色器共享
layout(push_constant) uniform PushConstants {
    vec3 blockPos;
    float damageStage;  // 0-9
} pc;

void main() {
    // 计算破坏阶段对应的UV坐标
    // 图集布局：2行5列
    // 阶段 0-4 在第一行，阶段 5-9 在第二行
    int stage = int(pc.damageStage);
    int col = stage % 5;
    int row = stage / 5;

    // 每个单元的UV尺寸
    float cellWidth = 1.0 / 5.0;
    float cellHeight = 1.0 / 2.0;

    // 计算该阶段的UV起点
    float u0 = float(col) * cellWidth;
    float v0 = float(row) * cellHeight;

    // 计算最终UV坐标
    vec2 uv = vec2(u0 + fragTexCoord.x * cellWidth, v0 + fragTexCoord.y * cellHeight);

    // 采样破坏纹理
    vec4 breakColor = texture(breakAtlas, uv);

    // 破坏纹理是黑色裂纹，alpha表示可见度
    // 最终颜色 = 原色 * (1 - alpha) + 黑色 * alpha
    // 但我们使用叠加混合模式，所以输出黑色 + alpha

    // 雾计算
    vec3 finalColor = breakColor.rgb;

    if (fog.fogMode == 1) {
        // 线性雾
        float distance = fragViewDistance;
        float fogFactor = clamp((fog.fogEnd - distance) / (fog.fogEnd - fog.fogStart), 0.0, 1.0);
        finalColor = mix(fog.fogColor.rgb, finalColor, fogFactor);
    } else if (fog.fogMode == 2) {
        // 指数雾
        float fogFactor = 1.0 - exp(-fog.fogDensity * fragViewDistance);
        finalColor = mix(fog.fogColor.rgb, finalColor, clamp(fogFactor, 0.0, 1.0));
    }

    // 输出颜色用于叠加混合
    // 混合公式：dst.rgb = dst.rgb * src.rgb (DST_COLOR * SRC_COLOR)
    // 所以我们需要输出 (1 - breakAlpha) 来保持原色，或输出 0 来变暗
    // 实际上，MC 使用的是：dst = dst * src
    // 破坏纹理是白色裂纹 + alpha，所以输出 vec3(1 - alpha) 来变暗
    // 但如果纹理是黑色裂纹 + alpha，我们输出 vec3(1 - alpha * brightness)

    // 简化处理：使用 alpha 来控制变暗程度
    // 最终混合后：dst.rgb = dst.rgb * (1 - breakColor.a)
    // 所以输出 vec3(1.0 - breakColor.a)
    float darkenFactor = breakColor.a;
    outColor = vec4(vec3(1.0 - darkenFactor), 1.0);
}
