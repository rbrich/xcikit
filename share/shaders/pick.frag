#version 450

layout(location = 0) out uint o_id;

layout(push_constant) uniform PushConstants {
    uint this_object_id;
    uint selected_object_id;
} push_const;

void main() {
    o_id = push_const.this_object_id;
}
