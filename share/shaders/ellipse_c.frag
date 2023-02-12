#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Attr {
    float softness;
    float antialiasing;
} attr;

layout(location = 0) in vec4 in_fill_color;
layout(location = 1) in vec4 in_outline_color;

// See rectangle.frag for description of these.
// For ellipses, these work similarly, but the borders
// are not used directly for the outline, but for computing
// distance from the center. Each border is used to compute
// unit circle (radius 1.0 from center) and the outline
// is bounded by these two circles.
layout(location = 2) in vec2 in_border_inner;
layout(location = 3) in vec2 in_border_outer;

layout(location = 0) out vec4 out_color;

void main() {
    float ri = length(in_border_inner);
    float ro = length(in_border_outer);
    float f = fwidth(ri) * attr.antialiasing + (ri/ro - 1) * attr.softness;
    float alpha = smoothstep(1-f/2, 1+f/2, ri);
    out_color = mix(in_fill_color, in_outline_color, alpha);
    f = fwidth(ro) * attr.antialiasing + (1 - ro/ri) * attr.softness;
    alpha = smoothstep(1-f/2, 1+f/2, ro);
    out_color = mix(out_color, vec4(0,0,0,0), alpha);
}
