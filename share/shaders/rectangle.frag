#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Outline {
    vec4 color;
} outline;

layout(binding = 2) uniform Attr {
    float softness;
    float antialiasing;
} u;

layout(location = 0) in vec4 in_fill_color;

// The colored outline is bounded by inner and outer border.
// Each border is defined by rectangle (-1,-1)..(+1,+1)
// in its corresponding lineary interpolated coordinates.
layout(location = 1) in vec2 in_border_inner;
layout(location = 2) in vec2 in_border_outer;

layout(location = 0) out vec4 o_color;

void main() {
    float ri = max(abs(in_border_inner.x), abs(in_border_inner.y));
    float ro = max(abs(in_border_outer.x), abs(in_border_outer.y));
    if (u.antialiasing > 0 || u.softness > 0) {
        float f = fwidth(ri) * u.antialiasing + (ri/ro - 1) * u.softness;
        float alpha = smoothstep(1-f/2, 1+f/2, ri);
        o_color = mix(in_fill_color, outline.color, alpha);
        f = fwidth(ro) * u.antialiasing + (1 - ro/ri) * u.softness;
        alpha = smoothstep(1-f/2, 1+f/2, ro);
        o_color = mix(o_color, vec4(0,0,0,0), alpha);
    } else {
        float eps = 1e-6;
        o_color = mix(in_fill_color, outline.color, step(1 - eps, ri));
        o_color = mix(o_color, vec4(0,0,0,0), step(1, ro));
    }
}
