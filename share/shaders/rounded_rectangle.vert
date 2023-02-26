#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Uniform { mat4 mvp; } uni;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_coord;
layout(location = 2) in vec2 in_size;
layout(location = 3) in vec2 in_thickness_radius;

layout(location = 0) out vec2 out_coord;
layout(location = 1) out vec2 out_size;
layout(location = 2) out vec2 out_thickness_radius;

void main() {
    gl_Position = uni.mvp * vec4(in_position, 0.0, 1.0);
    out_coord = in_coord;
    out_size = in_size;
    out_thickness_radius = in_thickness_radius;
}
