#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
uniform mat4 V;
uniform mat4 P;

out vec2 TexCoord;

void main() {
	float size = 0.1f;


    vec4 VPos = V * vec4(gl_in[0].gl_Position.xyz,1);
	VPos += (vec4(-0.5, 0.2, 0, 0) * size);

    gl_Position = P * (VPos + vec4(1,0,0, 0)*size);
    TexCoord = vec2(1.0, 1.0);
    EmitVertex();

    gl_Position = P * (VPos + vec4(1,1,0, 0)*size);
    TexCoord = vec2(1.0, 0.0);
    EmitVertex();

    gl_Position = P * (VPos + vec4(0,0,0, 0)*size);
    TexCoord = vec2(0.0, 1.0);
    EmitVertex();

    gl_Position = P * (VPos + vec4(0,1,0, 0)*size);
    TexCoord = vec2(0.0, 0.0);
    EmitVertex();

	EndPrimitive();
}
