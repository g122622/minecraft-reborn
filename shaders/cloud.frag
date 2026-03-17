#version 450

// Texture sampler
layout(set = 0, binding = 0) uniform sampler2D cloudTexture;

// Uniform Buffer (set = 0, binding = 1)
layout(set = 0, binding = 1) uniform CloudUBO {
    vec4 cloudColor;
    float cloudHeight;
    float time;
    float textureScale;
    float cameraY;
} ubo;

// Input from Vertex Shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragBrightness;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    vec2 texSize = vec2(textureSize(cloudTexture, 0));
    vec2 wrappedUv = fract(fragTexCoord);
    vec2 texelUv = (floor(wrappedUv * texSize) + 0.5) / texSize;

    // 使用中心点采样，避免线性插值导致的边缘发黑与拖影
    vec4 texColor = texture(cloudTexture, texelUv);

    // 使用硬阈值，复刻 MC 云块边界
    if (texColor.a < 0.5) {
        discard;
    }

    // MC 风格：使用统一云色 + 面向亮度，不使用纹理 RGB 参与着色
    vec3 color = ubo.cloudColor.rgb * fragBrightness;

    // MC 云整体透明度
    float alpha = 0.8 * ubo.cloudColor.a;

    outColor = vec4(color, alpha);
}
