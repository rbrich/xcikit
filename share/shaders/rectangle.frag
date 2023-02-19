#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Color {
    vec4 fill;
    vec4 outline;
} color;

layout(binding = 2) uniform Attr {
    float softness;
    float antialiasing;
} attr;

// The colored outline is bounded by inner and outer border.
// Each border is defined by rectangle (-1,-1)..(+1,+1)
// in its corresponding lineary interpolated coordinates.
layout(location = 0) in vec2 in_border_inner;
layout(location = 1) in vec2 in_border_outer;

layout(location = 0) out vec4 out_color;

void main() {
    float ri = max(abs(in_border_inner.x), abs(in_border_inner.y));
    float ro = max(abs(in_border_outer.x), abs(in_border_outer.y));
    if (attr.antialiasing > 0 || attr.softness > 0) {
        float f = fwidth(ri) * attr.antialiasing + (ri/ro - 1) * attr.softness;
        float alpha = smoothstep(1-f/2, 1+f/2, ri);
        out_color = mix(color.fill, color.outline, alpha);
        f = fwidth(ro) * attr.antialiasing + (1 - ro/ri) * attr.softness;
        alpha = smoothstep(1-f/2, 1+f/2, ro);
        out_color = mix(out_color, vec4(0,0,0,0), alpha);
    } else {
        out_color = mix(color.fill, color.outline, step(1, ri));
        out_color = mix(out_color, vec4(0,0,0,0), step(1, ro));
    }
}
