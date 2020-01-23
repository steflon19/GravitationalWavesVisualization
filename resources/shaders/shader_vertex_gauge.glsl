
#version 410 core
layout(location = 0) in vec3 vertPos;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 RotM;
uniform vec3 HandPos;
out vec3 vertex_pos;
out vec3 camvertex_pos;

void main()
{
    vertex_pos = (M * vec4(HandPos, 1.0)).xyz;
    camvertex_pos = vec3(V *  vec4(HandPos, 1.0));
	// TODO: M fine here?
    gl_Position = P * V * M * vec4(HandPos, 1.0);
	// Tut
    // gl_Position = vec4(vertPos, 1.0);        
}  