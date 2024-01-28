#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler2D u_tex_sampler;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
    const float strength = 0.15;  // 0.0 - 1.0  TODO: push constant
    const float steps = strength * 200;
    const vec2 center = vec2(0.5, 0.5);
    out_color = vec4(0,0,0,0);
    for (float i = 1.0 - strength; i < 1.0; i += strength / steps) {
        out_color += texture(u_tex_sampler, mix(center, in_tex_coord, i));
    }
    out_color /= steps;
}
