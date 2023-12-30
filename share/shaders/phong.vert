#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;

layout(binding = 0) uniform MVP {
    mat4 modelview;
    mat4 normal;
    mat4 projection;
} u_matrix;

void main() {
    vec4 vertex = vec4(position, 1.0);
    v_position = vec3(u_matrix.modelview * vertex);
    v_normal = normalize(mat3(u_matrix.normal) * normal);
    gl_Position = u_matrix.projection * u_matrix.modelview * vertex;
}
