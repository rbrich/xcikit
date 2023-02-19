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

// See rectangle.frag for description of these.
// For ellipses, these work similarly, but the borders
// are not used directly for the outline, but for computing
// distance from the center. Each border is used to compute
// unit circle (radius 1.0 from center) and the outline
// is bounded by these two circles.
layout(location = 0) in vec2 in_border_inner;
layout(location = 1) in vec2 in_border_outer;

layout(location = 0) out vec4 out_color;

void main() {
    float ri = length(in_border_inner);
    float ro = length(in_border_outer);
    float fa = 0.5 * fwidth(ro) * attr.antialiasing;
    if (ro > 1+fa)
        discard;
    float fi = fa + 0.5 * (ri/ro - 1) * attr.softness;
    out_color = mix(color.fill, color.outline, smoothstep(1-fi, 1+fi, ri));
    float fo = fa + 0.5 * (1 - ro/ri) * attr.softness;
    out_color = mix(out_color, vec4(0,0,0,0), smoothstep(1-fo, 1+fo, ro));
}
