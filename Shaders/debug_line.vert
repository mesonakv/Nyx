#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(set = 0, binding = 0) uniform FrameUniforms {
    mat4 viewProj;
    vec4 cameraPos;
    vec4 lightDirAndIntensity;
    vec4 lightColor;
    vec4 ambientColor;
} frame;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = frame.viewProj * vec4(inPosition, 1.0);
    fragColor = inColor;
}