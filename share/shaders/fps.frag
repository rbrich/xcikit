#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D u_tex_sampler;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
    float t = texture(u_tex_sampler, in_tex_coord).r;
    if (in_tex_coord.y < t) {
        out_color = vec4(0.0, 0.2, 0.2, 1.0);
    } else {
        out_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
