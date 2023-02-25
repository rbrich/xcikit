#version 450
#extension GL_ARB_separate_shader_objects : enable

// Vertex shader: polygon_c.vert

layout(binding = 1) uniform Attr {
    float softness;
    float antialiasing;
} attr;

layout(location = 0) in vec4 in_fill_color;
layout(location = 1) in vec4 in_outline_color;
layout(location = 2) in vec3 in_barycentric;

layout(location = 0) out vec4 out_color;

void main() {
    float r = min(min(in_barycentric.x, in_barycentric.y), in_barycentric.z);
    if (attr.antialiasing > 0 || attr.softness > 0) {
        float f = fwidth(r) * attr.antialiasing + attr.softness;
        float alpha = smoothstep(1-f/2, 1+f/2, r);
        out_color = mix(in_outline_color, in_fill_color, alpha);
        alpha = smoothstep(0-f/2, 0+f/2, r);
        out_color = mix(vec4(0), out_color, alpha);
    } else {
        // if (r < t) { out_color = in_outline_color; } else { out_color = in_fill_color; }
        out_color = mix(in_outline_color, in_fill_color, step(1, r));
    }
}
