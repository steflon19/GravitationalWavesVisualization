#version 330 core
layout(location = 0) in vec3 vertPos;


//uniform vec3 SpherePos;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

uniform float angle;

out vec3 vertex_pos;
out vec2 texcoord;

/*vec4 getAttractedPosition() {
    float maxDistance = 5.;
    float minDistance = 0.02;
    float earthMass = 1;
    
    vec4 pos = M * vec4(vertPos,1);
    vec3 dir = SpherePos - pos.xyz;
    float d = length(dir);
    //if(d > maxDistance) return vec4(vertPos, 1.0);
    //if(d < minDistance) d = minDistance;
    
    // TODO: improve formula for the force to actually represent proper gravitational force distribution
    float a = 0.00004;
    float force = (a) / pow(d,2);// clamp(a / pow(d,3), 0.,1.);
    //force = pow(force, 1/12);
    force = pow(force, 7/10.);
    pos.xyz = clamp(pos.xyz + normalize(dir) * force, pos.xyz, (SpherePos-normalize(dir)*0.02)); // 0.02 is the scale factor of the sphere

    return pos;
}*/

void main()
{
    //vec3 attractedPosition = vertPos;
    //vec4 attractedPosition = getAttractedPosition();
    
    mat2 rotation_matrix=mat2(  vec2(sin(angle),-cos(angle)),
                                vec2(cos(angle),sin(angle))
                            );
    texcoord = vertPos.xy;
    // +0.5 X to shift it into origin
    texcoord += vec2(0.5,0.);
    texcoord *= rotation_matrix;
    // +0.5 X and Y to shift texture center from origin to vecPos
    texcoord += vec2(0.5,-0.5);
    vec4 tpos =  M * vec4(vertPos, 1.0);
    vertex_pos = (M * vec4(vertPos, 1.0)).xyz;
    gl_Position = P * V * tpos;

}
