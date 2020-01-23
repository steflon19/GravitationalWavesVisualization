#version 330 core
in vec3 vertex_normal;
//in vec3 vertex_pos;
in vec2 TexCoord;
in vec3 camvertex_pos;
//// in float angle?
uniform sampler2D tex_gauge;
uniform sampler2D tex_indicator;
//uniform sampler2D tex2;

out vec4 color;

void main()
{
//	color = vec4(1);
// TODO: either make two shaders or find a way to distinguish when to render gauge and when indicator.
	color = texture2D(tex_gauge, TexCoord);
	if (color.r == 0 && color.g == 0 && color.b == 0) {
        //discard;
    }
	//color = texture(tex, texcoord);

}
