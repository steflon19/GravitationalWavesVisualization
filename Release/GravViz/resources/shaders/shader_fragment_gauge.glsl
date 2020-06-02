#version 330 core
in vec3 vertex_normal;
//in vec3 vertex_pos;
uniform float amplitude;
in vec2 TexCoord;
in vec3 camvertex_pos;
uniform sampler2D tex_gauge;
uniform sampler2D tex_indicator;
//uniform sampler2D tex2;

out vec4 color;

// https://godotengine.org/qa/41400/simple-texture-rotation-shader
vec2 rotateUV(vec2 uv, vec2 pivot, float rotation) {
    float cosa = cos(rotation);
    float sina = sin(rotation);
    uv -= pivot;
    return vec2(
        cosa * uv.x + sina * uv.y,
        cosa * uv.y - sina * uv.x 
    ) + pivot;
}
float PI = 3.14159265359;
float maxRot = (PI * 1.5);
void main()
{
	color = texture2D(tex_gauge, TexCoord);
	// calc proper amplitude..
	float ampl = amplitude * (1.9/2.5) * 2;
	if (ampl > maxRot ) ampl = maxRot;
	// base rotation -(0.75 * PI) so we start at 0 in the gauge
	vec2 indicatorTexCoord = rotateUV(TexCoord, (0.5/vec2(1,1)), -(0.75 * PI) + ampl);
	vec4 colIndicator = texture2D(tex_indicator, indicatorTexCoord);
	float threshold = 0.6;
	if(colIndicator.r < threshold && colIndicator.g < threshold && colIndicator.b < threshold && colIndicator.a > threshold) { color = colIndicator; }// = vec4(0);
	// color += colIndicator;
	//color = vec4(1,1,1,1);
	//color = texture(tex, texcoord);

}
