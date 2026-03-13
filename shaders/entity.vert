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

// 描述符集 1 - 光照UBO
layout(set = 0, binding = 1) uniform LightingUBO {
    float ambientStrength;
    float sunAngle;
    float moonAngle;
    int dayTime;
} lighting;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // 计算世界位置
    // 模型顶点已经是相对于实体原点的，需要加上实体位置
    vec3 worldPos = vec3(pc.model * vec4(inPosition * pc.scale + pc.entityPos, 1.0));

    gl_Position = camera.viewProjection * vec4(worldPos, 1.0);

    // 变换法线
    mat3 normalMatrix = mat3(transpose(inverse(pc.model)));
    fragNormal = normalize(normalMatrix * inNormal);

    fragTexCoord = inTexCoord;
    fragColor = vec4(1.0);  // 白色，可以通过uniform覆盖

    // 计算光照 - 简单的方向光
    // 法线朝上 (0,1,0) 时最亮
    float light = max(dot(fragNormal, vec3(0.0, 1.0, 0.0)), 0.0);
    fragLight = lighting.ambientStrength + light * (1.0 - lighting.ambientStrength);
}
