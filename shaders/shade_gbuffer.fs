#version 430

out vec4 outputColor;

uniform sampler2D in_albedo;
uniform sampler2D in_normal;

uniform vec3 in_light_incident;

in vec4 gl_FragCoord;

const float EPSILON = 0.0001f;
void main()
{
	vec3 normal = texelFetch(in_normal, ivec2(gl_FragCoord.xy), 0).xyz;
	vec3 albedo = texelFetch(in_albedo, ivec2(gl_FragCoord.xy), 0).xyz;
	if(length(normal) - 1 > EPSILON) 
	{
		// error state...
		outputColor = vec4(1,0,1,1) + albedo.xyzz;
		return;
	}
	outputColor = vec4(vec3(dot(normal, -in_light_incident)), 1);
}