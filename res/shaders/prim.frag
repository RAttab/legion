#version 430 core

in VS_OUT { vec4 rgba; } fs_in;
out vec4 fs_rgba;

void main()
{
  fs_rgba = fs_in.rgba;
}
