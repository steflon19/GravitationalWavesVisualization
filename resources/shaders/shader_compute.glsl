#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
layout(local_size_x = 1, local_size_y = 1) in;
// TODO this has to be added properly to the shader or the programm will crash
//layout(location = 0) in vec3 vertPos;
//uniform mat4 M;
//uniform mat4 Ry;
//uniform sampler2D tex2;
//uniform vec2 bi_star_facts;

layout (std430, binding=0) volatile buffer shader_data
{ 
  vec4 io[512];
};

float hash(float n) { return fract(sin(n) * 753.5453123); }
float snoise(vec3 x)
	{
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);

	float n = p.x + p.y * 157.0 + 113.0 * p.z;
	return mix(mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
		mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
		mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
			mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
	}

float noise(vec3 position, int octaves, float frequency, float persistence) {
	float total = 0.0;
	float maxAmplitude = 0.0;
	float amplitude = 1.0;
	for (int i = 0; i < octaves; i++) {
		total += snoise(position * frequency) * amplitude;
		frequency *= 2.0;
		maxAmplitude += amplitude;
		amplitude *= persistence;
		}
	return total / maxAmplitude;
}



void main() 
{
	uint index = gl_GlobalInvocationID.x;		
	vec3 data = io[index].xyz;

//	vec4 spiralcolor = texture(tex2, tpos.xz * 0.5);
//
//	float amplitudefact = bi_star_facts.x*bi_star_facts.y;
//    float gw_force = spiralcolor.r*amplitudefact;
//    
//    vec3 normVertPos = normalize(vertPos);
//    float f = dot(normalize(vec2(length(normVertPos.xz), normVertPos.y)), vec2(1,0));
//    float y = sign(vertPos.y);
//    attractedPosition.y += y*(0.02 * f * gw_force);
//    
//    float c = 0.02;
//    vec3 v = normalize(-vertPos);
//    v *= c * gw_force * f;

	float height = noise(data.xzy*10, 11, 0.03, 0.6);
	float baseheight = noise(data.xzy*10, 4, 0.004, 0.3);
	baseheight = pow(baseheight, 5)*3;
	height = baseheight*height;
	height*=30.0;
	height-=5.0;

	io[index].w = height;
	
}