#version 330

uniform sampler2D u_texture;

in vec4 v_color;
in vec2 v_tex_coord;

out vec4 o_color;

void main() {
    float alpha = texture(u_texture, v_tex_coord).r;
    o_color = vec4(v_color.rgb, v_color.a * alpha);
}
