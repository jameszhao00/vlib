#version 430

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

uniform mat4 view_proj;
out vec3 vs2fs_normal;
out vec2 vs2fs_uv;

void main()
{
    gl_Position = view_proj * vec4(in_position.xyz, 1);
    vs2fs_normal = in_normal;
	vs2fs_uv = in_uv;
}