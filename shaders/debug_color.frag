#version 450 core

layout(location=0) out vec4 oColor;

layout(constant_id = 0) const uint debug_r = 1;
layout(constant_id = 1) const uint debug_g = 0;
layout(constant_id = 2) const uint debug_b = 1;


void main() {
	oColor = uvec4(debug_r, debug_g, debug_b, 255) / 255.f;
}