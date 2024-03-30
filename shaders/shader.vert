#version 450 core

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fPosition;
layout(location = 2) out vec4 fNormal;

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;

#include "uCamera.h"
#include "sPhysics.h"

void main() {
    const vec3 rot_axis = normalize(cross(vec3(0,1,0), velocities[gl_InstanceID].xyz));
    const float sep_angle = acos( dot(vec3(0,1,0), velocities[gl_InstanceID].xyz) / length(velocities[gl_InstanceID].xyz) );
    const vec4 aligned_vertex = vec4(
        cos(sep_angle) * vPosition.xyz
            + sin(sep_angle)*cross(rot_axis, vPosition.xyz) 
            + (1-cos(sep_angle)) * dot(rot_axis,vPosition.xyz) * rot_axis, 
        1.f);

    fPosition = view * (positions[gl_InstanceID] + aligned_vertex);
    fNormal = view * vNormal;
    fragColor = colors[gl_InstanceID];

    gl_Position = proj * fPosition;
}