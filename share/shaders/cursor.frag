#version 330

uniform vec4 u_fill_color;
uniform vec4 u_outline_color;

in vec2 v_border_inner;

out vec4 o_color;

void main() {
    float ri = max(abs(v_border_inner.x), abs(v_border_inner.y));
    float eps = 1e-6;
    o_color = mix(u_fill_color, u_outline_color, step(1 - eps, ri));
}
