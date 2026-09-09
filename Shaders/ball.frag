#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in float fragMetallic;
layout(location = 3) in float fragRoughness;
layout(location = 4) in float fragEmissive;

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

layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(push.lightDirAndIntensity.xyz);
    float lightIntensity = push.lightDirAndIntensity.w;
    vec3 lightColor = push.lightColorAndPad.rgb;

    vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0));
    vec3 halfVec = normalize(lightDir + viewDir);

    float diffuse = max(dot(normal, lightDir), 0.0);
    float specular = 0.0;
    if (diffuse > 0.0) {
        float spec = pow(max(dot(normal, halfVec), 0.0), 32.0 * (1.0 - fragRoughness) + 1.0);
        specular = spec * (1.0 - fragRoughness);
    }

    vec3 specColor = mix(vec3(1.0), fragColor, fragMetallic);
    vec3 ambient = fragColor * 0.4;
    vec3 diffuseColor = fragColor * diffuse * 0.6 * lightColor * lightIntensity;
    vec3 specularColor = specColor * specular * 0.8 * lightColor * lightIntensity;
    vec3 emissiveColor = fragColor * fragEmissive;

    vec3 finalColor = ambient + diffuseColor + specularColor + emissiveColor;

    outColor = vec4(finalColor, 1.0);
}