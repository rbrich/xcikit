#version 450

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform Light {
    vec4 position;
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
    float quad_attenuation;
} u_light;

layout(binding = 2) uniform Material {
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
    float shininess;
} u_mat;

layout(push_constant) uniform PushConstants {
    uint this_object_id;
    uint selected_object_id;
} push_const;

void main() {
    vec3 N = v_normal;
    vec3 L = normalize(vec3(u_light.position) - v_position);
    vec3 E = normalize(-v_position); // We are in eye coordinates, so eye is (0,0,0)
    vec3 R = normalize(-reflect(L, N));

    // Ambient light
    vec4 Iamb = u_mat.ambient_color * u_light.ambient_color;

    // Diffuse reflection
    vec4 Idiff = u_mat.diffuse_color * u_light.diffuse_color * max(dot(N, L), 0.0);
    Idiff = clamp(Idiff, 0.0, 1.0);

    // Specular reflection
    vec4 Ispec = u_mat.specular_color * u_light.specular_color
                 * pow(max(dot(R, E), 0.0), u_mat.shininess);
    Ispec = clamp(Ispec, 0.0, 1.0);

    // Attenuation
    float light_dist = distance(vec3(u_light.position), v_position);
    float att = 1.0 / (1.0 + u_light.quad_attenuation * light_dist * light_dist);

    out_color = Iamb + att * Idiff + att*att * Ispec;

    if (push_const.this_object_id == push_const.selected_object_id) {
        out_color.r += 0.5;
    }
}
