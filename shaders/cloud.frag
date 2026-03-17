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
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in float fragBrightness;
layout(location = 3) in float fragFogFactor;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // Sample cloud texture
    vec4 texColor = texture(cloudTexture, fragTexCoord);

    // Discard fully transparent pixels (no cloud here)
    if (texColor.a < 0.1) {
        discard;
    }

    // Apply brightness based on face direction
    // Top faces: brighter (normal.y = 1)
    // Bottom faces: darker (normal.y = -1)
    // Side faces: medium brightness
    vec3 color = texColor.rgb * fragBrightness;

    // Apply cloud color tint
    color *= ubo.cloudColor.rgb;

    // Cloud transparency - MC uses 0.8 alpha for clouds
    float alpha = texColor.a * 0.8 * ubo.cloudColor.a;

    // Output final color
    outColor = vec4(color, alpha);
}
