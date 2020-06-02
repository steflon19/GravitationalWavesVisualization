#version 330 core
layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

in vec3 vertex_pos[];
in vec3 scolor[];
in vec3 camvertex_pos[];
out vec3 camvertex_pos_g;
out vec2 texcoord;
out vec3 fcolor;

void main() {
    vec2 vert_pos_one = vertex_pos[0].xy;
    vec2 vert_pos_two = vertex_pos[1].xy;
    
    vec3 camvert_pos_one = camvertex_pos[0];
    vec3 camvert_pos_two = camvertex_pos[1];
    float width = 0.002f;
    
    // vertical
    gl_Position = gl_in[0].gl_Position + vec4(-width, 0.0, 0.0, -0.0);
    camvertex_pos_g = camvert_pos_one;
    texcoord = vec2(0,1);
    fcolor = scolor[0];
    EmitVertex();

    gl_Position = gl_in[1].gl_Position + vec4(-width, 0.0, 0.0, -0.0);
    camvertex_pos_g = camvert_pos_two;
    texcoord = vec2(1,1);
    fcolor = scolor[0];
    EmitVertex();
    
    gl_Position = gl_in[0].gl_Position + vec4( width, 0.0, 0.0, 0.0);
    camvertex_pos_g = camvert_pos_one;
    texcoord = vec2(0,0);
    fcolor = scolor[0];
    EmitVertex();
    
    gl_Position = gl_in[1].gl_Position + vec4( width, 0.0, 0.0, 0.0);
    camvertex_pos_g = camvert_pos_two;
    texcoord = vec2(1,0);
    fcolor = scolor[0];
    EmitVertex();
    
    EndPrimitive();
}
