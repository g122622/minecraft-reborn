#version 450

// GUI顶点着色器
// 用于渲染2D GUI元素，如文本和矩形

// 顶点输入
layout(location = 0) in vec2 inPosition;  // 屏幕坐标
layout(location = 1) in vec2 inTexCoord;  // 纹理坐标
layout(location = 2) in vec4 inColor;     // 颜色 (ARGB)

// 输出到片段着色器
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

// 推送常量 - 屏幕尺寸
layout(push_constant) uniform PushConstants {
    vec2 screenSize;
    vec2 padding;
} pc;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // 转换为标准化设备坐标 (-1到1)
    vec2 ndc = (inPosition / pc.screenSize) * 2.0 - 1.0;
    // Vulkan 视口(正高度)下，NDC Y=-1 对应屏幕顶部，NDC Y=+1 对应屏幕底部。
    // 输入坐标已使用屏幕左上角为原点（Y向下），不需要额外翻转。

    gl_Position = vec4(ndc, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
