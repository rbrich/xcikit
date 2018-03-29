#version 330

uniform vec4 u_fill_color;
uniform vec4 u_outline_color;

in vec2 v_border_inner;

out vec4 o_color;

void main() {
    // >1 = outline, <1 = fill
    float r = max(abs(v_border_inner.x), abs(v_border_inner.y));
    float alpha = step(1, r);
    o_color = mix(u_fill_color, u_outline_color, alpha);
}
