#version 430

in vec3 vs2fs_normal;
in vec2 vs2fs_uv;

uniform sampler2D in_albedo;

void main()
{
    gl_FragData[0] = texture(in_albedo, vs2fs_uv);
    gl_FragData[1] = vec4(normalize(vs2fs_normal), 1.0f);
}