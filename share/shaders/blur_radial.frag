#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler2D u_tex_sampler;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants {
    vec2 center;
    float radius;  // 0.0 - 1.0
    float resolution;
} c;

void main() {
    out_color = vec4(0,0,0,0);
    const int steps = int(c.radius * c.resolution);
    const float step = 1.0 / c.resolution;
    for (int i = steps; i != 0; --i) {
        const vec4 color = texture(u_tex_sampler, mix(in_tex_coord, c.center, i * step));
        if (color.a == 0)
            break;  // optimization, possible only when bluring a centered convex shape
        out_color += color;
    }
    out_color /= steps;
}
