#version 460 core

layout(location = 0) out vec3 fragColor;
layout(location = 0) in vec3 color;
layout(location = 1) in vec2 position;

const vec2 positions[3] = vec2[](
vec2(0.0, -0.2),
vec2(0.2, 0.2),
vec2(-0.2, 0.2)
);


void main() {
    gl_Position = vec4(position + positions[gl_VertexID], 0.0, 1.0);
    fragColor = color;
}