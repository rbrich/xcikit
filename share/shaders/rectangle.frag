#version 330

uniform vec4 u_fill_color;
uniform vec4 u_outline_color;
uniform float u_softness;

// The colored outline is bounded by inner and outer border.
// Each border is defined by rectangle (-1,-1)..(+1,+1)
// in its corresponding lineary interpolated coordinates.
in vec2 v_border_inner;
in vec2 v_border_outer;

out vec4 o_color;

void main() {
    float ri = max(abs(v_border_inner.x), abs(v_border_inner.y));
    float ro = max(abs(v_border_outer.x), abs(v_border_outer.y));
    float f = fwidth(ri) * u_softness;
    float alpha = smoothstep(1-f, 1, ri);
    o_color = mix(u_fill_color, u_outline_color, alpha);
    f = fwidth(ro) * u_softness;
    alpha = smoothstep(1-f, 1, ro);
    o_color = mix(o_color, vec4(0,0,0,0), alpha);
}
