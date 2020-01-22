#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec3 vertex_pos;
out vec3 frag_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;

// TODO: move rotation to CPU
mat4 rotationX( float angle ) {
    return mat4(    1.0,              0,              0,        0,
                      0,     cos(angle),    -sin(angle),        0,
                      0,     sin(angle),     cos(angle),        0,
                      0,              0,              0,        1.);
}
mat4 rotationY( in float angle ) {
    return mat4(     cos(angle),          0,        sin(angle),    0,
                              0,        1.0,                 0,    0,
                    -sin(angle),          0,        cos(angle),    0,
                              0,          0,                 0,    1.);
}

void main()
{
    vec4 newPos = vec4(vertPos,1.0);
    
    newPos = newPos * rotationX(3.14159265359/2.) * rotationY(3.14159265359);
    
	vertex_normal = vec4(M * vec4(vertNor,0.0)).xyz;
    frag_pos = vec4(M * vec4(newPos)).xyz;
    
    
    vec4 tpos =  M *  vec4(newPos.xyz, 1.0);
	vertex_pos = tpos.xyz;
	gl_Position = P * V * tpos;
	vertex_tex = vertTex;
}
