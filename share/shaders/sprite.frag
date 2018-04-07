#version 330

uniform sampler2D u_texture;
uniform vec4 u_color;

in vec2 v_tex_coord;

out vec4 o_color;

void main() {
    float alpha = texture(u_texture, v_tex_coord).r;
    o_color = vec4(u_color.rgb, u_color.a * alpha);
}
