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

layout(binding = 3) uniform Outline {
    float thickness;
} outline;

layout(location = 0) in vec3 in_barycentric;

layout(location = 0) out vec4 out_color;

void main() {
    if (attr.antialiasing > 0 || attr.softness > 0) {
        float r = in_barycentric.z;
        float f = fwidth(r) * attr.antialiasing + outline.thickness * attr.softness;
        float alpha = smoothstep(outline.thickness-f/2, outline.thickness+f/2, r);
        out_color = mix(color.outline, color.fill, alpha);
        alpha = smoothstep(0-f/2, 0+f/2, r);
        out_color = mix(vec4(0), out_color, alpha);
    } else {
        // if (in_barycentric.z < outline.thickness) { out_color = color.outline; } else { out_color = color.fill; }
        out_color = mix(color.outline, color.fill, step(outline.thickness, in_barycentric.z));
    }
}
