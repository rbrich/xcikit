#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Color {
    vec4 fill;
    vec4 outline;
} color;

layout(binding = 2) uniform Attr {
    float softness;
    float antialiasing;
} attr;

layout(location = 0) in vec3 in_barycentric;

layout(location = 0) out vec4 out_color;

void main() {
    const float t = 1.0;  // outline threshold
    float r = in_barycentric.z;
    if (attr.antialiasing > 0 || attr.softness > 0) {
        float f = fwidth(r) * attr.antialiasing + t * attr.softness;
        float alpha = smoothstep(t-f/2, t+f/2, r);
        out_color = mix(color.outline, color.fill, alpha);
        alpha = smoothstep(0-f/2, 0+f/2, r);
        out_color = mix(vec4(0), out_color, alpha);
    } else {
        // if (r < t) { out_color = color.outline; } else { out_color = color.fill; }
        out_color = mix(color.outline, color.fill, step(t, r));
    }
}
