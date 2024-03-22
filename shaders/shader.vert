#version 450 core

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fPosition;

layout(location = 0) in vec3 iColor;
layout(location = 1) in vec3 vPosition;

#include "uCamera.h"
#include "uPhysics.h"

void main() {
    gl_Position = proj * view * vec4(positions[gl_InstanceID].xyz + vPosition, 1.0);
    fPosition = gl_Position.xyz;
    fragColor = iColor;
}