#version 410 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;
out vec2 frag_tex;
void main()
{
	vec4 pos = vec4(vertPos,1.0);
	frag_tex = vertTex;
	gl_Position = pos;
	
}
