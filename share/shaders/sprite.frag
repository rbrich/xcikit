#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Uniform { vec4 color; } uni;
layout(binding = 2) uniform sampler2D u_tex_sampler;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
    float alpha = texture(u_tex_sampler, in_tex_coord).r;
    out_color = vec4(uni.color.rgb, uni.color.a * alpha);
}
