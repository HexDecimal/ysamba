#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in float tid;
layout (location = 3) in vec4 color;

out DATA
{
  vec2 uv;
  float tid;
  vec4 color;
  vec2 pos;
} v_out;

uniform mat4 view;
uniform mat4 projection;

void main()
{
  gl_Position = projection * view * vec4(position, 1.0);

  v_out.uv = vec2(uv.x, uv.y);
  v_out.tid = tid;
  v_out.color = color.wzyx;
  v_out.pos = position.xy;
}
