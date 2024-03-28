#version 450 core

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fPosition;

layout(location = 0) in vec3 vPosition;

#include "uCamera.h"
#include "sPhysics.h"

void main() {
    const vec3 rot_axis = normalize(cross(vec3(0,1,0), velocities[gl_InstanceID].xyz));
    const float sep_angle = acos( dot(vec3(0,1,0), velocities[gl_InstanceID].xyz) / length(velocities[gl_InstanceID].xyz) );
    const vec3 aligned_vertex 
        = cos(sep_angle) * vPosition 
        + sin(sep_angle)*cross(rot_axis, vPosition)
        + (1-cos(sep_angle)) * dot(rot_axis,vPosition) * rot_axis;

    gl_Position = proj * view * vec4(positions[gl_InstanceID].xyz+aligned_vertex, 1.0);
    fPosition = gl_Position.xyz;
    fragColor = colors[gl_InstanceID].xyz;
}