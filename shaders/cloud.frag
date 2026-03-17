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
    // 几何阶段已基于云掩码生成，此处不再按纹理 alpha 打孔，避免棋盘格碎块感。
    // 保留少量天空色调影响，但整体更接近 MC 洁白云。
    vec3 whitenedCloudColor = mix(vec3(1.0), ubo.cloudColor.rgb, 0.22);
    vec3 color = whitenedCloudColor * fragBrightness;

    // 提升不透明度，贴近原版观感。
    float alpha = 0.95 * ubo.cloudColor.a;

    outColor = vec4(color, alpha);
}
