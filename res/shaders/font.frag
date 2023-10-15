#version 430 core

in VS_OUT { vec3 tex; vec4 fg; vec4 bg; } fs_in;
uniform sampler2DArray fs_tex;
out vec4 fs_rgba;

void main()
{
  vec4 tex = texture(fs_tex, fs_in.tex) * fs_in.fg;
  float sum = tex.a + fs_in.bg.a;
  fs_rgba = tex * (tex.a / sum) + fs_in.bg * (fs_in.bg.a / sum);
}
