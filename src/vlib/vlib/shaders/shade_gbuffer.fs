#version 430
out vec4 outputColor;

uniform sampler2D sampler_albedo;
void main()
{
   outputColor = vec4(1.f,1.f,1.f,1.0f);
}