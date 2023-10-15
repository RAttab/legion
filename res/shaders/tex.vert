#version 430 core

layout (location = 0) in vec4 vs_pos;
layout (location = 1) in vec4 vs_fg;
layout (location = 2) in vec2 vs_tex;
out VS_OUT { vec2 tex; vec4 fg; } vs_out;

void main()
{
  gl_Position = vs_pos;
  vs_out.tex = vs_tex;
  vs_out.fg = vs_fg;
}
