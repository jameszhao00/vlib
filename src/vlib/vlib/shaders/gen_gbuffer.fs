#version 430

out vec4 outputColor;
in vec3 vs2fs_normal;
void main()
{
   outputColor = vec4(vs2fs_normal, 1.0f);
}