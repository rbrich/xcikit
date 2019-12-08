#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Uniform {
    mat4 mvp;
} uni;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_border_inner;

out vec2 v_border_inner;

void main() {
    gl_Position = uni.mvp * vec4(a_position, 0.0, 1.0);
    v_border_inner = a_border_inner;
}
