#version 430
out vec4 outputColor;

uniform sampler2D albedo;
uniform sampler2D normal;
in vec4 gl_FragCoord;
void main()
{
	outputColor = texelFetch(normal, ivec2(gl_FragCoord.xy), 0).xyzz;
   //outputColor = gl_FragCoord / 1000.f;//vec4(1.f,0.f,1.f,1.0f);
}