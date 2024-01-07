#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Uniform { mat4 mvp; } uni;

layout(location = 0) in vec2 in_position;

void main() {
    gl_Position = uni.mvp * vec4(in_position, 0.0, 1.0);
}
