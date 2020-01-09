#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
layout(local_size_x = 1, local_size_y = 1) in;
uniform mat4 M;
uniform mat4 Ry;
uniform sampler2D tex2;
uniform vec2 bi_star_facts;

layout (std430, binding=3) volatile buffer shader_data
{ 
  vec4 io[]; // was 512
};

void main() 
{
	uint index = gl_GlobalInvocationID.x;		
	vec3 data = io[index].xyz;


	vec3 vertPos = vec3(M[3]);
    vec4 tpos =  Ry * M * vec4(vertPos, 1.0);
	vec4 spiralcolor = texture(tex2, tpos.xz * 0.5);

	float amplitudefact = bi_star_facts.x * bi_star_facts.y;
    float gw_force = spiralcolor.r * amplitudefact;
    
    vec3 normvertpos = normalize(vertPos);
    float f = dot(normalize(vec2(length(normvertpos.xz), normvertpos.y)), vec2(1,0));
	f *= gw_force;
//    float y = sign(vertPos.y);
//    float c = 0.02;
//    vec3 v = normalize(-vertPos);
//    v *= c * gw_force * f;

	io[index].w = f;// bi_star_facts.x + bi_star_facts.y;
}