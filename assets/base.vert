#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 in_tex_coords;

out vec2 tex_coords;

uniform sampler2D frame;

void main() {
	gl_Position = vec4(position, 0.0, 1.0);
	tex_coords = in_tex_coords;
}