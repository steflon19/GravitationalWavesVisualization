#version 330 core
out vec4 color;
//in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 texcoord;
//in vec3 camvertex_pos_g;
uniform sampler2D tex;

void main()
{
    color = texture(tex, texcoord);//vec4(vertex_pos, 1);
}
