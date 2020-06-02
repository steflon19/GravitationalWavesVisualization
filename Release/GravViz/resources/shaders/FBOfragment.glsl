#version 410 core
out vec4 color;
in vec2 frag_tex;
uniform sampler2D tex;
uniform vec2 texoff;
void main()
{
vec4 tcol = texture(tex, frag_tex+texoff);
color = tcol;
color.a=1;
}
