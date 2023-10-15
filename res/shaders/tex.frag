#version 430 core

in VS_OUT { vec2 tex; vec4 fg; } fs_in;
uniform sampler2D fs_tex;
out vec4 fs_rgba;

void main()
{
  fs_rgba = texture(fs_tex, fs_in.tex) * fs_in.fg;
}
