#version 430

out vec4 outputColor;

uniform sampler2D in_diffuse;
uniform sampler2D in_normal;

uniform vec3 in_light_incident;

in vec4 gl_FragCoord;

const float EPSILON = 0.001f;
void main()
{
	vec3 normal = texelFetch(in_normal, ivec2(gl_FragCoord.xy), 0).xyz;
	vec3 diffuse = texelFetch(in_diffuse, ivec2(gl_FragCoord.xy), 0).xyz;
	if(abs(length(normal) - 1) > EPSILON) 
	{
		// error state...
		outputColor = vec4(1,0,1,1);
		return;
	}
	outputColor = vec4((diffuse) * vec3(dot(normal, -in_light_incident)), 1);
}