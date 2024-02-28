#version 450 core

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fPosition;

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec3 vPosition;
layout(location = 2) in vec3 iPosition;

layout(std140, binding = 0) uniform uCamera {
    mat4 view;
    mat4 proj;
};



void main() {
    gl_Position = proj * view * vec4(iPosition + vPosition, 1.0);
    fPosition = gl_Position.xyz;
    fragColor = vColor;
}