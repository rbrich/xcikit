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

layout(location = 0) in vec2 in_border_inner;

layout(location = 0) out vec4 out_color;

void main() {
    float r = in_border_inner.y;
    float f = fwidth(r) * attr.antialiasing + attr.softness;
    float alpha = smoothstep(0-f/2, 0+f/2, r);
    out_color = mix(color.fill, color.outline, alpha);
    alpha = smoothstep(1-f/2, 1+f/2, r);
    out_color = mix(out_color, color.fill, alpha);
}
