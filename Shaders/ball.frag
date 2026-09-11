#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in float fragMetallic;
layout(location = 4) in float fragRoughness;
layout(location = 5) in float fragEmissive;

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
    vec4 material;
} push;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(frame.lightDirAndIntensity.xyz);
    float lightIntensity = frame.lightDirAndIntensity.w;
    vec3 lightColor = frame.lightColor.rgb;

    // 正确的视线方向：从表面指向相机
    vec3 viewDir = normalize(frame.cameraPos.xyz - fragWorldPos);
    vec3 halfVec = normalize(lightDir + viewDir);

    float diffuse = max(dot(normal, lightDir), 0.0);
    float specular = 0.0;
    if (diffuse > 0.0) {
        float spec = pow(max(dot(normal, halfVec), 0.0), 32.0 * (1.0 - fragRoughness) + 1.0);
        specular = spec * (1.0 - fragRoughness);
    }

    vec3 specColor = mix(vec3(1.0), fragColor, fragMetallic);
    // 环境光跟随天空颜色（T2.4）
    vec3 ambient = fragColor * frame.ambientColor.rgb;
    vec3 diffuseColor = fragColor * diffuse * 0.6 * lightColor * lightIntensity;
    vec3 specularColor = specColor * specular * 0.8 * lightColor * lightIntensity;
    vec3 emissiveColor = fragColor * fragEmissive;

    vec3 finalColor = ambient + diffuseColor + specularColor + emissiveColor;

    outColor = vec4(finalColor, 1.0);
}