#version 330

uniform vec4 u_fill_color;
uniform vec4 u_outline_color;
uniform float u_softness;
uniform float u_antialiasing;

in vec2 v_border_inner;

out vec4 o_color;

void main() {
    float r = v_border_inner.y;
    float f = fwidth(r) * u_antialiasing + u_softness;
    float alpha = smoothstep(0-f/2, 0+f/2, r);
    o_color = mix(u_fill_color, u_outline_color, alpha);
    alpha = smoothstep(1-f/2, 1+f/2, r);
    o_color = mix(o_color, u_fill_color, alpha);
}
