#version 430 core

layout (location = 0) in vec4 vs_pos;
layout (location = 1) in vec4 vs_fg;
layout (location = 2) in vec3 vs_tex;
layout (location = 3) in vec4 vs_bg;
out VS_OUT { vec3 tex; vec4 fg; vec4 bg; } vs_out;

void main()
{
  gl_Position = vs_pos;
  vs_out.tex = vs_tex;
  vs_out.fg = vs_fg;
  vs_out.bg = vs_bg;
}
