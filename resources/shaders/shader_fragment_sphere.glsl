#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 frag_pos;
//uniform vec3 campos;

uniform sampler2D tex_sphere;

void main()
{
vec3 n = normalize(vertex_normal);
vec3 lp=vec3(-10,2,-10);
vec3 ld = normalize(vertex_pos - lp);
float diffuse = dot(n,ld);

color = texture(tex_sphere, vertex_tex) * diffuse;
color.a = 1;//0.2;
 
// "cut"  half of the sphere away
// if(frag_pos.z<0) color.a = 0;

/*color *= diffuse*0.7;


vec3 cd = normalize(vertex_pos - campos);
vec3 h = normalize(cd+ld);
float spec = dot(n,h);
spec = clamp(spec,0,1);
spec = pow(spec,20);
color += vec4(vec3(1,1,1)*spec*3,1.0);
 */

}
