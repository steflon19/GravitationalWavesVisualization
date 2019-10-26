#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 texcoord;
in vec3 camvertex_pos_g;
in vec3 fcolor;
uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{
float f=0.25;

float depth = 1. + camvertex_pos_g.z*f;
if(depth<0)depth=0;

depth = pow(depth,7);
//depth*=2;
vec4 color4= texture(tex, texcoord);
//if(color4.a < .1) discard;
color4.a *= depth*2;
// color = vec4(texcoord.rg,0.,a);//vec4(color3,depth);
color = color4;
color.a = color.b*depth*2;
//color.a *= depth*2;
//color.rgb=fcolor;
//color.a=1;

}
