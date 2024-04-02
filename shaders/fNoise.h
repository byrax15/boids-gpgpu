#ifndef FNOISE
#define FNOISE

#include "uTime.h"

float noise(float x)
{
    return fract(sin(x + deltaTime) * (20.f + deltaTime));
}

vec3 noise(vec3 x)
{
    return vec3(noise(x.x), noise(x.y), noise(x.z));
}

vec3 centered_noise(vec3 x)
{
    return vec3(noise(x.x), noise(x.y), noise(x.z)) -.5f;
}

#endif