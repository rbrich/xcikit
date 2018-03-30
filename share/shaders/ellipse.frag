#version 330

uniform vec4 u_fill_color;
uniform vec4 u_outline_color;

in vec2 v_border_inner;
in vec2 v_border_outer;

out vec4 o_color;

void main() {
    float ri = length(v_border_inner);
    float ro = length(v_border_outer);
    float f = fwidth(ri);
    float alpha = smoothstep(1-f, 1, ri);
    o_color = mix(u_fill_color, u_outline_color, alpha);
    alpha = smoothstep(1-f, 1, ro);
    o_color = mix(o_color, vec4(0,0,0,0), alpha);
}
