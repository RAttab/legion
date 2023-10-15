#version 430 core

layout (location = 0) in vec4 vs_pos;
layout (location = 1) in vec4 vs_rgba;
out VS_OUT { vec4 rgba; } vs_out;

void main()
{
  gl_Position = vs_pos;
  vs_out.rgba = vs_rgba;
}
