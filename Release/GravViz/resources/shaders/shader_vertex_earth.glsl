#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 Ry;
out vec3 vertex_pos;
out vec3 frag_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;
uniform vec2 bi_star_facts;
uniform sampler2D tex_spiral;

void main()
{
    vec4 newPos = vec4(vertPos,1.0);
    
	vertex_normal = vec4(M * vec4(vertNor,0.0)).xyz;
    frag_pos = vec4(M * newPos).xyz;
    
    //vec4 tpos =  M * vec4(newPos);// vec4(newPos, 1.0);
    vec4 tpos =  Ry * M * vec4(newPos);
	vertex_pos = newPos.xyz;
    
     tpos/=5*bi_star_facts.x;
     tpos.xz += vec2(1,1);
     //tpos/=1.0;
     vec4 spiralcolor = texture(tex_spiral, tpos.xz * 0.5);
	// spiralcolor = vec4(0);
    float amplitudefact = bi_star_facts.x*bi_star_facts.y;
    float gw_force = spiralcolor.r*amplitudefact;

     newPos = M * newPos;
     vec3 normVertPos = normalize(newPos.xyz);
     float f = dot(normalize(vec2(length(normVertPos.xz), tpos.y)), vec2(1,0));
     float sign = sign(newPos.y);
     newPos.y *= 1+ (0.52 * f * gw_force);
     
     float c = 0.02;
     vec3 v = normalize(-vec3(newPos.xyz));
     v *= c * spiralcolor.r * f;
     newPos += vec4(v, 0.);
     //newPos.xz *=f;
     gl_Position = P * V * newPos;
    vertex_pos = newPos.xyz;
	//gl_Position = P * V * tpos;
	vertex_tex = vertTex;
}
