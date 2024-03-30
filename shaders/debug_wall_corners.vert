#version 460 core

#include "uCamera.h"
#include "uSceneWalls.h"

void main() {
	gl_Position = vec4(scene_size, 1);
	switch (gl_InstanceID) {
	case 1:
		gl_Position.y *= -1;
		break;
	}
	switch(gl_VertexID){
	case 0:
		gl_Position.xyz *= vec3(1, 1, 1);
		break;
	case 1:
		gl_Position.xyz *= vec3(1, 1, -1);
		break;
	case 2:
		gl_Position.xyz *= vec3(-1, 1, -1);
		break;
	case 3:
		gl_Position.xyz *= vec3(-1, 1, 1);
		break;
	}
	gl_Position = proj * view * gl_Position;
}