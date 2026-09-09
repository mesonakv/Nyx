#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec4 ballColor;
    float metallic;
    float roughness;
    float emissive_strength;
    float pad;
    vec4 lightDirAndIntensity;
    vec4 lightColorAndPad;
} push;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out float fragMetallic;
layout(location = 3) out float fragRoughness;
layout(location = 4) out float fragEmissive;

void main() {
    gl_Position = push.viewProj * vec4(inPosition, 1.0);
    fragColor = push.ballColor.rgb;
    fragNormal = inNormal;
    fragMetallic = push.metallic;
    fragRoughness = push.roughness;
    fragEmissive = push.emissive_strength;
}