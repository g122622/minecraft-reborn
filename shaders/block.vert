#version 450

// 顶点输入
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in uint inColor;
layout(location = 4) in uint inLight;

// 输出到片段着色器
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragColor;
layout(location = 4) out float fragLight;

// Uniform缓冲区 - 相机矩阵
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} camera;

// 推送常量 - 模型变换
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 chunkOffset;
} pc;

// 解码颜色 (RGBA打包在uint中)
vec4 decodeColor(uint color) {
    return vec4(
        float((color >> 0) & 0xFF) / 255.0,
        float((color >> 8) & 0xFF) / 255.0,
        float((color >> 16) & 0xFF) / 255.0,
        float((color >> 24) & 0xFF) / 255.0
    );
}

void main() {
    // 计算世界空间位置（应用区块偏移）
    vec3 worldPos = inPosition + pc.chunkOffset;

    // 输出
    fragWorldPos = worldPos;
    fragNormal = mat3(pc.model) * inNormal;
    fragTexCoord = inTexCoord;
    fragColor = decodeColor(inColor);
    fragLight = float(inLight & 0xF) / 15.0; // 光照等级 0-15 映射到 0.0-1.0

    // 计算裁剪空间位置
    gl_Position = camera.viewProjection * vec4(worldPos, 1.0);
}
