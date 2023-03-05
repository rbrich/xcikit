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

layout(location = 0) in vec2 in_coord;  // normalized coords (-1,-1)..(+1,+1)

layout(location = 0) out vec4 out_color;

void main() {
    float r = length(in_coord);
    float f = 0.5 * fwidth(r) * attr.antialiasing + 0.05 * attr.softness;
    if (r > 1 + f)
        discard;
    out_color = mix(color.fill, color.outline, smoothstep(0.9-f, 0.9+f, r));
    out_color = mix(out_color, vec4(0,0,0,0), smoothstep(1-f, 1+f, r));
}
