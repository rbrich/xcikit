#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Color {
    vec4 fill;
    vec4 outline;
} color;

layout(location = 0) in vec2 in_border_inner;

layout(location = 0) out vec4 out_color;

void main() {
    float ri = max(abs(in_border_inner.x), abs(in_border_inner.y));
    float eps = 1e-6;
    out_color = mix(color.fill, color.outline, step(1 - eps, ri));
}
