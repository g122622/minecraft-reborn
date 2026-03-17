#version 450

// Push Constants
layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
} pc;

// Vertex Attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

// Output to Fragment Shader
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out float fragBrightness;

// Uniform Buffer (set = 0, binding = 1)
layout(set = 0, binding = 1) uniform CloudUBO {
    vec4 cloudColor;
    float cloudHeight;
    float time;
    float textureScale;
    float cameraY;
} ubo;

void main() {
    // Transform position
    vec4 worldPos = vec4(inPosition, 1.0);
    gl_Position = pc.viewProjection * worldPos;

    // Pass texture coordinates
    fragTexCoord = inTexCoord;

    // 按 MC 风格固定面亮度：顶面最亮，底面最暗，X/Z 侧面次之
    if (inNormal.y > 0.5) {
        fragBrightness = 1.0;
    } else if (inNormal.y < -0.5) {
        fragBrightness = 0.7;
    } else if (abs(inNormal.x) > 0.5) {
        fragBrightness = 0.9;
    } else {
        fragBrightness = 0.8;
    }
}
