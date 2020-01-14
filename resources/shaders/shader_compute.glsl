#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
layout(local_size_x = 1, local_size_y = 1) in;
uniform vec3 HandPos;
uniform mat4 Ry;
uniform sampler2D tex2;
uniform vec2 bi_star_facts;

layout (std430, binding=0) volatile buffer shader_data
{ 
  vec4 io[512]; // was 512
};

void main() 
{
	uint index = gl_GlobalInvocationID.x;		
	vec3 data = io[index].xyz;


	//vec3 vertPos = vec3(M[3]);
    vec4 tpos =  Ry * vec4(HandPos, 1.0);
	vec4 spiralcolor = texture(tex2, tpos.xz * 0.5);

	float amplitudefact = bi_star_facts.x * bi_star_facts.y;
    float gw_force = spiralcolor.r * amplitudefact;
    
    vec3 normvertpos = normalize(HandPos);
    float f = dot(normalize(vec2(length(normvertpos.xz), normvertpos.y)), vec2(1,0));
	//f *= gw_force;
    f *= gw_force;

	io[index].w = f;// bi_star_facts.x + bi_star_facts.y;
}