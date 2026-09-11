#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(set = 0, binding = 0) uniform FrameUniforms {
    mat4 viewProj;
    vec4 cameraPos;
    vec4 lightDirAndIntensity;
    vec4 lightColor;
    vec4 ambientColor;
} frame;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 ballColor;
    vec4 material;  // x=metallic, y=roughness, z=emissive_strength, w=isScreenSpace
} push;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out float fragMetallic;
layout(location = 4) out float fragRoughness;
layout(location = 5) out float fragEmissive;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // isScreenSpace 为 1 时（准星），push.model 直接作为最终变换
    mat4 mvp;
    if (push.material.w > 0.5) {
        mvp = push.model;
    } else {
        mvp = frame.viewProj * push.model;
    }
    gl_Position = mvp * vec4(inPosition, 1.0);

    // 均匀缩放假设下，法线用 mat3(model) 近似即可
    fragNormal = mat3(push.model) * inNormal;

    fragColor = push.ballColor.rgb;
    fragMetallic = push.material.x;
    fragRoughness = push.material.y;
    fragEmissive = push.material.z;
}