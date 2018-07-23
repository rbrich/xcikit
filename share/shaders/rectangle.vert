#version 330

uniform mat4 u_mvp;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec4 a_fill_color;
layout(location = 2) in vec2 a_border_inner;
layout(location = 3) in vec2 a_border_outer;

out vec4 v_fill_color;
out vec2 v_border_inner;
out vec2 v_border_outer;

void main() {
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
    v_fill_color = a_fill_color;
    v_border_inner = a_border_inner;
    v_border_outer = a_border_outer;
}
