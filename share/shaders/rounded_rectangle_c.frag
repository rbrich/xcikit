#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Attr {
    float softness;
    float antialiasing;
} attr;

layout(location = 0) in vec4 in_fill_color;
layout(location = 1) in vec4 in_outline_color;

layout(location = 2) in vec2 in_coord;  // normalized coords (-1,-1)..(+1,+1)
layout(location = 3) in vec2 in_size;   // size in pixels
layout(location = 4) in vec2 in_thickness_radius;  // outline thickness, corner radius (in pixels)

layout(location = 0) out vec4 out_color;

float max2(vec2 v) { return max (v.x, v.y); }

void main() {
    const float thickness = in_thickness_radius.x;
    const float r = in_thickness_radius.y;
    const vec2 is = in_size - r;

    if (all(greaterThan(abs(in_coord * in_size), is))) {
        float lro = length(in_coord * in_size - sign(in_coord) * is);
        float lri = lro + thickness;
        float lfa = 0.5 * fwidth(lro) * attr.antialiasing;
        if (lro > r + lfa)
            discard;
        if (attr.antialiasing > 0 || attr.softness > 0) {
            float f = lfa + 0.5 * attr.softness * thickness;
            out_color = mix(in_fill_color, in_outline_color, smoothstep(r-f, r+f, lri));
            out_color = mix(out_color, vec4(0,0,0,0), smoothstep(r-f, r+f, lro));
        } else {
            out_color = mix(in_fill_color, in_outline_color, step(r, lri));
        }
    } else {
        float mro = max2(abs(in_coord));
        float mri = max2(abs(in_coord) + thickness / in_size);
        float mfa = 0.5 * fwidth(mro) * attr.antialiasing;
        if (mro > 1 + mfa)
            discard;
        if (attr.antialiasing > 0 || attr.softness > 0) {
            float f = mfa + 0.5 * attr.softness * (mri - mro);
            out_color = mix(in_fill_color, in_outline_color, smoothstep(1-f, 1+f, mri));
            out_color = mix(out_color, vec4(0,0,0,0), smoothstep(1-f, 1+f, mro));
        } else {
            out_color = mix(in_fill_color, in_outline_color, step(1, mri));
        }
    }
}
