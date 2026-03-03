#version 450

// 从顶点着色器输入
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragColor;
layout(location = 4) in float fragLight;

// 输出颜色
layout(location = 0) out vec4 outColor;

// 纹理图集
layout(set = 1, binding = 0) uniform sampler2D texAtlas;

// 光照设置
layout(set = 0, binding = 1) uniform LightingUBO {
    vec3 sunDirection;
    float sunIntensity;
    vec3 ambientColor;
    float ambientIntensity;
    vec3 cameraPosition;
    vec3 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint fogMode; // 0 = none, 1 = linear, 2 = exponential
} lighting;

// 计算雾效果
vec3 applyFog(vec3 color, float distance) {
    if (lighting.fogMode == 0) {
        return color;
    } else if (lighting.fogMode == 1) {
        // 线性雾
        float factor = clamp((lighting.fogEnd - distance) / (lighting.fogEnd - lighting.fogStart), 0.0, 1.0);
        return mix(lighting.fogColor, color, factor);
    } else {
        // 指数雾
        float factor = exp(-lighting.fogDensity * distance);
        return mix(lighting.fogColor, color, factor);
    }
}

// 计算漫反射光照
float calculateDiffuse(vec3 normal) {
    float diffuse = max(dot(normalize(normal), normalize(lighting.sunDirection)), 0.0);
    return diffuse * lighting.sunIntensity;
}

void main() {
    // 采样纹理图集
    vec4 texColor = texture(texAtlas, fragTexCoord);

    // 如果纹理alpha太低，丢弃像素（透明度裁剪）
    if (texColor.a < 0.1) {
        discard;
    }

    // 计算光照
    float diffuse = calculateDiffuse(fragNormal);
    float ambient = lighting.ambientIntensity;

    // 组合光照（顶点光照 + 太阳光 + 环境光）
    // fragLight 是顶点光照（来自方块周围的环境）
    float finalLight = fragLight * (ambient + diffuse * 0.3);

    // 应用光照到纹理颜色
    vec3 litColor = texColor.rgb * fragColor.rgb * finalLight;

    // 计算到相机的距离用于雾效果
    float fogDistance = length(fragWorldPos - lighting.cameraPosition);

    // 应用雾效果
    litColor = applyFog(litColor, fogDistance);

    outColor = vec4(litColor, texColor.a * fragColor.a);
}
