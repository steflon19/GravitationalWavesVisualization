#version 330 core
layout(location = 0) in vec3 vertPos;

uniform vec3 SpherePos;
uniform vec3 MoonPos;
uniform vec3 BPosOne;
uniform vec3 BPosTwo;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 Ry;
uniform float earthScale;
out vec3 vertex_pos;
out vec3 camvertex_pos;
out vec3 scolor;
uniform sampler2D tex2;
uniform vec2 bi_star_facts;

vec4 getAttractedPosition() {
    
    vec4 pos = M * vec4(vertPos,1);
    vec3 dir = SpherePos - pos.xyz;
    float d = length(dir);
	float max_d = d - earthScale;
	if (max_d < 0) return pos;
    
    float a = 0.0001;
    float force = (a) / pow(d,2);
    force = pow(force, 5.5/10.);
	if (force > max_d) force = max_d;

    //pos.xyz = clamp(pos.xyz + normalize(dir) * force, pos.xyz, (SpherePos-normalize(dir)*earthScale));
	pos.xyz = pos.xyz + normalize(dir) * force;

    return pos;
}

vec4 getAttractedPositionMoon(vec4 npos) {

    vec4 pos = M * npos;
    vec3 dir = MoonPos - pos.xyz;
    float d = length(dir);
	float max_d = d - (earthScale * 0.27);
	if (max_d < 0) return pos;
    
    float a = 0.000005;
    float force = (a) / pow(d,2);
    force = pow(force, 5.5/10.);
	if (force > max_d) force = max_d;
    pos.xyz = pos.xyz + normalize(dir) * force;// clamp(pos.xyz + normalize(dir) * force, pos.xyz, (MoonPos-normalize(dir)*(earthScale*0.27)));
    
    return pos;
}

vec4 getAttractedPositionBinary(vec4 gpos, vec3 bpos) {
    vec4 pos = M * gpos;
    vec3 dir = bpos - pos.xyz;
    float d = length(dir);
	float max_d = d - 0.02f;
	//if (max_d < 0) return pos;
    
    float a = 0.0001;
    float force = (a) / pow(d,2);
    force = pow(force, 5.5/10.);
	//if (force > max_d) force = max_d;
    //pos.xyz = pos.xyz * normalize(dir) * force; // 
	pos.xyz = clamp(pos.xyz + normalize(dir) * force, pos.xyz, (bpos-normalize(dir)*(0.02)));
    
    return pos;
}

void main()
{
    //vec3 pulledPos = vertPos * distance(vertPos, SpherePos);
    vec4 attractedPosition = getAttractedPosition();
    attractedPosition = getAttractedPositionMoon(attractedPosition);
	// attractedPosition = getAttractedPositionBinary(attractedPosition, BPosOne);
	// attractedPosition = getAttractedPositionBinary(attractedPosition, BPosTwo);
    vec4 tpos =  Ry * M * vec4(vertPos, 1.0);
    vertex_pos = (M * vec4(vertPos, 1.0)).xyz; // Rotate this by 90° ??
    camvertex_pos = vec3(V * attractedPosition);
    gl_Position = P * V * attractedPosition;
    
    // simulated Gravitational waves from texture
    //tpos -> texcoord
    //tpos.z*=-1;
    
    // 5 would be the "correct" gridSize value, but because a turned square "repeats" around the edged this can be avoided by "scaling" the texture->increasing the divisor here.
    tpos/= 5 * bi_star_facts.x;
    tpos.xz += vec2(1,1);
    //tpos/=1.0;
    vec4 spiralcolor = texture(tex2, tpos.xz * 0.5);

	float amplitudefact = bi_star_facts.x*bi_star_facts.y;
    float gw_force = spiralcolor.r*amplitudefact;
    
    vec3 normVertPos = normalize(vertPos);
    float f = dot(normalize(vec2(length(normVertPos.xz), normVertPos.y)), vec2(1,0));
    float y = sign(vertPos.y);
    attractedPosition.y += y*(0.02 * f * gw_force);
    
    float c = 0.02;
    vec3 v = normalize(-vertPos);
    v *= c * gw_force * f;
    attractedPosition += vec4(v, 0.);
   //attractedPosition.xz *=f;
    gl_Position = P * V * attractedPosition;
    scolor = spiralcolor.rgb;

}
