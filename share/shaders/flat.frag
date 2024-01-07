#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Uniform { vec4 color; } uni;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = uni.color;
}
