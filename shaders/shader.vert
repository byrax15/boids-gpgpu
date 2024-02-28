#version 450 core

layout(location = 0) out vec3 fragColor;

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 position;

layout(std140, binding = 0) uniform uCamera {
    mat4 view;
    mat4 proj;
};

const vec2 positions[3] = vec2[3](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);


void main() {
    gl_Position = proj * view * vec4(position + positions[gl_VertexID], 0.0, 1.0);
    fragColor = color;
}