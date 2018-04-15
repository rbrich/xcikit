#version 330

uniform sampler2D u_texture;

in vec2 v_tex_coord;

out vec4 o_color;

void main() {
    float t = texture(u_texture, v_tex_coord).r;
    if (v_tex_coord.y <= t) {
        o_color = vec4(0.0, 0.2, 0.2, 1.0);
    } else {
        o_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
