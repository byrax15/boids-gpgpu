
layout(std430, binding = 0) buffer sPositions
{
    vec4 positions[];
};

layout(std430, binding = 1) buffer sVelocities
{
    vec4 velocities[];
};