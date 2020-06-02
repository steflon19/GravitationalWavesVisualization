
#version 410 core
layout(location = 0) in vec3 vertPos;

uniform mat4 V;
uniform vec3 HandPos;
out vec3 vertex_pos;
out vec3 camvertex_pos;

void main()
{
    vertex_pos = (vec4(HandPos, 1.0)).xyz;
    camvertex_pos = vec3(V *  vec4(HandPos, 1.0));
	// TODO: M fine here?
    gl_Position = vec4(HandPos, 1.0);
	// Tut
    // gl_Position = vec4(vertPos, 1.0);        
}  