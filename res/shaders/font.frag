#version 430 core

in VS_OUT { vec3 tex; vec4 fg; vec4 bg; } fs_in;
uniform sampler2DArray fs_tex;
out vec4 fs_rgba;

void main()
{
  vec4 tex = texture(fs_tex, fs_in.tex) * fs_in.fg;
  fs_rgba = fs_in.bg.a == 0 ? tex :
    (tex * tex.a) + (fs_in.bg * (1 - tex.a));
}
