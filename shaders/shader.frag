#version 450 core

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 fPosition;
layout(location = 2) in vec4 fNormal;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}