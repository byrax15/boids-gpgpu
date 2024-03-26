#version 450 core

#include "sPhysics.h"
#include "uCamera.h"

void main(){
	if (gl_VertexID == 0)
		gl_Position = proj * view * vec4(positions[gl_InstanceID].xyz, 1);
	else 
		gl_Position = proj * view * vec4(positions[gl_InstanceID].xyz + velocities[gl_InstanceID].xyz, 1);
}