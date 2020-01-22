#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 texcoord;
// in float angle?
uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{

	color = texture(tex, texcoord);

}
