#version 450

// 实体顶点着色器

// 顶点输入 - 与ModelVertex结构匹配
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

// 输出到片段着色器
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out float fragLight;

// 推送常量 - 模型矩阵
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 entityPos;      // 实体世界位置
    float scale;         // 缩放因子
} pc;

// 描述符集 0 - 相机UBO
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} camera;

// 描述符集 1 - 光照UBO（需与 UniformBuffer.hpp::LightingUBO 保持一致）
layout(set = 0, binding = 1) uniform LightingUBO {
    vec3 sunDirection;
    float sunIntensity;

    vec3 ambientColor;
    float ambientIntensity;

    vec3 cameraPosition;
    float padding1;

    vec3 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint fogMode;

    float celestialAngle;
    float skyBrightness;
    int moonPhase;
    float starBrightness;
} lighting;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // 计算世界位置：先应用模型旋转/局部变换，再加实体世界坐标
    vec3 localPos = vec3(pc.model * vec4(inPosition * pc.scale, 1.0));
    vec3 worldPos = localPos + pc.entityPos;

    gl_Position = camera.viewProjection * vec4(worldPos, 1.0);

    // 变换法线
    mat3 normalMatrix = mat3(transpose(inverse(pc.model)));
    fragNormal = normalize(normalMatrix * inNormal);

    fragTexCoord = inTexCoord;
    fragColor = vec4(1.0);  // 白色，可以通过uniform覆盖

    // 计算光照：环境光 + 太阳方向漫反射
    // 使用最低亮度保护，避免因UBO异常或极端角度导致实体全黑
    vec3 sunDir = normalize(lighting.sunDirection);
    float diffuse = max(dot(fragNormal, sunDir), 0.0) * max(lighting.sunIntensity, 0.0);
    float ambient = clamp(lighting.ambientIntensity, 0.15, 1.0);
    fragLight = clamp(max(ambient + diffuse, 0.22), 0.0, 1.0);
}
