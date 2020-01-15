#version 330 core
layout(location = 0) in vec3 vertPos;


//uniform vec3 SpherePos;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

uniform float angle;

out vec3 vertex_pos;
out vec2 texcoord;

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
