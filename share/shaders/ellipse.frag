#version 330

uniform vec4 u_fill_color;
uniform vec4 u_outline_color;
uniform float u_softness;
uniform float u_antialiasing;

// See rectangle.frag for description of these.
// For ellipses, these work similarly, but the borders
// are not used directly for the outline, but for computing
// distance from the center. Each border is used to compute
// unit circle (radius 1.0 from center) and the outline
// is bounded by these two circles.
in vec2 v_border_inner;
in vec2 v_border_outer;

out vec4 o_color;

void main() {
    float ri = length(v_border_inner);
    float ro = length(v_border_outer);
    float f = fwidth(ri) * u_antialiasing + (ri/ro - 1) * u_softness;
    float alpha = smoothstep(1-f/2, 1+f/2, ri);
    o_color = mix(u_fill_color, u_outline_color, alpha);
    f = fwidth(ro) * u_antialiasing + (1 - ro/ri) * u_softness;
    alpha = smoothstep(1-f/2, 1+f/2, ro);
    o_color = mix(o_color, vec4(0,0,0,0), alpha);
}
