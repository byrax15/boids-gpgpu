#version 450 core

#include "uCamera.h"
#include "uSceneWalls.h"

void main(){
	gl_Position = proj * view * walls[gl_VertexID];
}