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
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out float fragBrightness;
layout(location = 3) out float fragFogFactor;

// Uniform Buffer (set = 0, binding = 1)
layout(set = 0, binding = 1) uniform CloudUBO {
    vec4 cloudColor;
    float cloudHeight;
    float time;
    float textureScale;
    float cameraY;
} ubo;

// Constants for fog calculation
const float FOG_NEAR = 0.0;
const float FOG_FAR = 1.0;

void main() {
    // Transform position
    vec4 worldPos = vec4(inPosition, 1.0);
    gl_Position = pc.viewProjection * worldPos;

    // Pass texture coordinates
    fragTexCoord = inTexCoord;

    // Pass normal for lighting
    fragNormal = inNormal;

    // Calculate brightness based on normal direction
    // Top faces are brighter, bottom faces are darker
    // This creates the 3D cloud effect
    float normalBrightness = dot(inNormal, vec3(0.0, 1.0, 0.0));
    fragBrightness = normalBrightness * 0.3 + 0.7;

    // Simple distance fog calculation
    // Fog increases with distance from camera
    float distance = length(gl_Position.xyz);
    fragFogFactor = clamp((FOG_FAR - distance) / (FOG_FAR - FOG_NEAR), 0.0, 1.0);
}
