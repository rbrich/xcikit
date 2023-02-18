#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Attr {
    float softness;
    float antialiasing;
} attr;

layout(location = 0) in vec4 in_fill_color;
layout(location = 1) in vec4 in_outline_color;
layout(location = 2) in vec2 in_border_inner;

layout(location = 0) out vec4 out_color;

void main() {
    float r = in_border_inner.y;
    float f = fwidth(r) * attr.antialiasing + attr.softness;
    float alpha = smoothstep(0-f/2, 0+f/2, r);
    out_color = mix(in_fill_color, in_outline_color, alpha);
    alpha = smoothstep(1-f/2, 1+f/2, r);
    out_color = mix(out_color, in_fill_color, alpha);
}
