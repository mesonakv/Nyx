#version 450

layout(location = 0) in vec2 fragUV;

layout(push_constant) uniform SkyPush {
    vec4 topColor;
    vec4 bottomColor;
} push;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = mix(push.bottomColor.rgb, push.topColor.rgb, fragUV.y);
    outColor = vec4(color, 1.0);
}