#version 450

// GUI片段着色器
// 支持三种渲染模式：
// 1. 字体渲染：采样字体纹理的R通道作为alpha，使用color.rgb
// 2. 物品/图标渲染：采样物品纹理图集的RGBA，与color混合
// 3. 纯色矩形：不采样纹理，直接使用color

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

// 描述符集 0
layout(set = 0, binding = 0) uniform sampler2D fontSampler;   // 字体纹理 (R8)
layout(set = 0, binding = 1) uniform sampler2D itemSampler;   // 物品纹理图集 (RGBA)

void main() {
    vec4 color = fragColor;

    // 负UV标记：纯色矩形模式
    if (fragTexCoord.x < 0.0 || fragTexCoord.y < 0.0) {
        outColor = color;
        return;
    }

    // 检查alpha的最高位来判断使用哪个纹理
    // alpha >= 254/255 (约0.996) 且 < 255/255 时使用物品纹理
    // alpha == 255/255 (1.0) 时使用字体纹理
    // alpha < 254/255 时使用物品纹理（保持实际alpha值）
    float alpha255 = color.a * 255.0;

    if (alpha255 >= 254.5) {
        // 字体模式：alpha来自纹理R通道
        float fontAlpha = texture(fontSampler, fragTexCoord).r;
        outColor = vec4(color.rgb, fontAlpha);
    } else {
        // 物品模式：采样RGBA纹理，与颜色混合
        vec4 texColor = texture(itemSampler, fragTexCoord);
        // 纹理颜色 * 顶点颜色（支持着色），保留顶点alpha
        outColor = vec4(texColor.rgb * color.rgb, texColor.a * color.a);
    }
}
