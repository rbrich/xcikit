#version 330

in vec4 v_fill_color;
uniform vec4 u_outline_color;
uniform float u_softness;
uniform float u_antialiasing;

// The colored outline is bounded by inner and outer border.
// Each border is defined by rectangle (-1,-1)..(+1,+1)
// in its corresponding lineary interpolated coordinates.
in vec2 v_border_inner;
in vec2 v_border_outer;

out vec4 o_color;

void main() {
    float ri = max(abs(v_border_inner.x), abs(v_border_inner.y));
    float ro = max(abs(v_border_outer.x), abs(v_border_outer.y));
    if (u_antialiasing > 0 || u_softness > 0) {
        float f = fwidth(ri) * u_antialiasing + (ri/ro - 1) * u_softness;
        float alpha = smoothstep(1-f/2, 1+f/2, ri);
        o_color = mix(v_fill_color, u_outline_color, alpha);
        f = fwidth(ro) * u_antialiasing + (1 - ro/ri) * u_softness;
        alpha = smoothstep(1-f/2, 1+f/2, ro);
        o_color = mix(o_color, vec4(0,0,0,0), alpha);
    } else {
        float eps = 1e-6;
        o_color = mix(v_fill_color, u_outline_color, step(1 - eps, ri));
        o_color = mix(o_color, vec4(0,0,0,0), step(1, ro));
    }
}
