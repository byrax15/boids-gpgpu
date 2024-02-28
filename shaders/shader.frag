#version 450 core

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(std140, binding = 1) uniform uColors {
    vec3 color;
};

void main() {
    outColor = vec4(0.5 * color + fragColor, 1.0);
}