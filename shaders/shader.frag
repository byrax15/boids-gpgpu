#version 450 core

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fPosition;

layout(location = 0) out vec4 outColor;

layout(std140, binding = 1) uniform uColors {
    vec3 color;
};

void main() {
    outColor = vec4(color + fragColor, 1.0);
}