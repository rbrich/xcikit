#version 450
#extension GL_ARB_separate_shader_objects : enable

// vertext shader: coord2.vert

layout(binding = 2) uniform Attr {
    float softness;
    float antialiasing;
} attr;

layout(binding = 1) uniform Color {
    vec4 fill;
    vec4 outline;
} color;

layout(location = 0) in vec2 in_coord;  // triangle vertices: (0, 0), (0.5, 0), (1, 1)

layout(location = 0) out vec4 out_color;

void main() {
    float d = in_coord.x * in_coord.x - in_coord.y;
    float f = 0.5 * fwidth(d) * attr.antialiasing + 0.05 * attr.softness;
    out_color = mix(color.fill, color.outline, smoothstep(-f, f, d));
}
