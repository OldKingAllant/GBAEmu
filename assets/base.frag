#version 330 core 

in vec2 tex_coords;

out vec4 color;

uniform sampler2D frame;

void main() {
	color = vec4(texture(frame, tex_coords.xy).rgb, 1.0);
}