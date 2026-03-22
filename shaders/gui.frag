#version 450

// GUI片段着色器
// 支持多种渲染模式：
// 1. 字体渲染：采样字体纹理的R通道作为alpha，使用color.rgb
// 2. 物品/图标渲染：采样物品纹理图集的RGBA，与color混合
// 3. GUI纹理渲染：采样指定的GUI图集纹理
// 4. 纯色矩形：不采样纹理，直接使用color

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in uint fragAtlasSlot;

layout(location = 0) out vec4 outColor;

// 最大图集槽位数量
const uint MAX_ATLAS_SLOTS = 16;

// 描述符集 0: 多个纹理采样器
// 槽位 0: 字体纹理 (R8)
// 槽位 1: 物品纹理图集 (RGBA)
// 槽位 2-15: GUI纹理图集 (RGBA)
layout(set = 0, binding = 0) uniform sampler2D fontSampler;
layout(set = 0, binding = 1) uniform sampler2D itemSampler;
layout(set = 0, binding = 2) uniform sampler2D guiSampler0;
layout(set = 0, binding = 3) uniform sampler2D guiSampler1;
layout(set = 0, binding = 4) uniform sampler2D guiSampler2;
layout(set = 0, binding = 5) uniform sampler2D guiSampler3;
layout(set = 0, binding = 6) uniform sampler2D guiSampler4;
layout(set = 0, binding = 7) uniform sampler2D guiSampler5;
layout(set = 0, binding = 8) uniform sampler2D guiSampler6;
layout(set = 0, binding = 9) uniform sampler2D guiSampler7;
layout(set = 0, binding = 10) uniform sampler2D guiSampler8;
layout(set = 0, binding = 11) uniform sampler2D guiSampler9;
layout(set = 0, binding = 12) uniform sampler2D guiSampler10;
layout(set = 0, binding = 13) uniform sampler2D guiSampler11;
layout(set = 0, binding = 14) uniform sampler2D guiSampler12;
layout(set = 0, binding = 15) uniform sampler2D guiSampler13;

// 使用纹理数组来简化选择逻辑
vec4 sampleAtlas(uint slot, vec2 uv) {
    // 根据槽位ID选择对应的采样器
    switch (slot) {
        case 0u: return vec4(1.0, 1.0, 1.0, texture(fontSampler, uv).r); // 字体：R通道作为alpha
        case 1u: return texture(itemSampler, uv);                     // 物品图集
        case 2u: return texture(guiSampler0, uv);
        case 3u: return texture(guiSampler1, uv);
        case 4u: return texture(guiSampler2, uv);
        case 5u: return texture(guiSampler3, uv);
        case 6u: return texture(guiSampler4, uv);
        case 7u: return texture(guiSampler5, uv);
        case 8u: return texture(guiSampler6, uv);
        case 9u: return texture(guiSampler7, uv);
        case 10u: return texture(guiSampler8, uv);
        case 11u: return texture(guiSampler9, uv);
        case 12u: return texture(guiSampler10, uv);
        case 13u: return texture(guiSampler11, uv);
        case 14u: return texture(guiSampler12, uv);
        case 15u: return texture(guiSampler13, uv);
        default: return vec4(1.0, 0.0, 1.0, 1.0); // 品红色表示错误
    }
}

void main() {
    vec4 color = fragColor;

    // 负UV标记：纯色矩形模式
    if (fragTexCoord.x < 0.0 || fragTexCoord.y < 0.0) {
        outColor = color;
        return;
    }

    // 根据槽位ID采样对应的纹理
    vec4 texColor = sampleAtlas(fragAtlasSlot, fragTexCoord);

    // 槽位0是字体纹理：R通道作为alpha
    if (fragAtlasSlot == 0u) {
        outColor = vec4(color.rgb, texColor.a * color.a);
    } else {
        // 其他槽位：纹理颜色 * 顶点颜色（支持着色）
        outColor = vec4(texColor.rgb * color.rgb, texColor.a * color.a);
    }
}
