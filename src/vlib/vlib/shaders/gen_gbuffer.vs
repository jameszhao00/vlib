#version 430

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
uniform mat4 view_proj;
out vec3 vs2fs_normal;
void main()
{
    gl_Position = view_proj * vec4(in_position.xyz, 1);
    vs2fs_normal = in_normal;
}