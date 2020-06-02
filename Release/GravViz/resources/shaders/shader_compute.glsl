#version 450
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba8, binding = 0) uniform image2D img_input;
// needed?
// layout(rgba8, binding = 1) uniform image2D img_output;
uniform vec3 HandPos;
uniform mat4 Ry;
uniform sampler2D tex;
uniform vec2 bi_star_facts;

layout (std430, binding=0) volatile buffer shader_data
{ 
  vec4 io[512]; // was 512
};

void main() 
{
	uint index = gl_GlobalInvocationID.x;		
	vec3 data = io[index].xyz;
	int image_dimension = 999;

	//vec3 vertPos = vec3(M[3]);
    vec4 tpos =  Ry * vec4(HandPos, 1.0);
    tpos/=5*bi_star_facts.x;
    tpos.xz += vec2(1,1);
	vec4 spiralcolor = imageLoad(img_input, ivec2(tpos.xz * image_dimension * 0.5)); // texture(tex, tpos.xz * 0.5);
	vec4 spiralcolorUL = imageLoad(img_input, ivec2(999,999));// texture(tex, ivec2(1000,1000));
	vec4 spiralcolorLL = imageLoad(img_input, ivec2(0,0));// texture(tex, ivec2(0,1));
	float amplitudefact = bi_star_facts.x * bi_star_facts.y;
    float gw_force = spiralcolor.r * amplitudefact;
    
    vec3 normvertpos = normalize(HandPos);
    float f = dot(normalize(vec2(length(normvertpos.xz), normvertpos.y)), vec2(1,0));
	//f *= gw_force;
    f *= gw_force;
	io[index].y = spiralcolorUL.r;
	io[index].z = spiralcolorLL.r;
	io[index].w = f;// bi_star_facts.x + bi_star_facts.y;
}