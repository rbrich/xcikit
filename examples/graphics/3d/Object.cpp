// Object.cpp created on 2024-01-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Object.h"

namespace {
struct FragPushConstants {
    uint32_t this_object_id = 0;
    uint32_t selected_object_id = 0;
};
} // namespace


namespace xci::graphics {


Object::Object(Renderer& renderer)
        : m_prim(renderer, VertexFormat::V3n3, PrimitiveType::TriList)
{
    m_prim.set_depth_test(DepthTest::Less);
    m_prim.reserve_push_constants(sizeof(FragPushConstants));
}


void Object::create_cube(float size)
{
    const float s0 = -0.5 * size;
    const float s1 = +0.5 * size;
    m_prim.add_vertex({s0, s1, s0}).normal({0, 1, 0});
    m_prim.add_vertex({s0, s1, s1}).normal({0, 1, 0});
    m_prim.add_vertex({s1, s1, s1}).normal({0, 1, 0});
    m_prim.add_vertex({s1, s1, s0}).normal({0, 1, 0});
    m_prim.add_triangle_face({0, 1, 2});
    m_prim.add_triangle_face({0, 2, 3});

    m_prim.add_vertex({s0, s1, s0}).normal({-1, 0, 0});
    m_prim.add_vertex({s0, s0, s0}).normal({-1, 0, 0});
    m_prim.add_vertex({s0, s0, s1}).normal({-1, 0, 0});
    m_prim.add_vertex({s0, s1, s1}).normal({-1, 0, 0});
    m_prim.add_triangle_face({4, 5, 6});
    m_prim.add_triangle_face({4, 6, 7});

    m_prim.add_vertex({s1, s1, s1}).normal({1, 0, 0});
    m_prim.add_vertex({s1, s0, s1}).normal({1, 0, 0});
    m_prim.add_vertex({s1, s0, s0}).normal({1, 0, 0});
    m_prim.add_vertex({s1, s1, s0}).normal({1, 0, 0});
    m_prim.add_triangle_face({8, 9, 10});
    m_prim.add_triangle_face({8, 10, 11});

    m_prim.add_vertex({s1, s1, s0}).normal({0, 0, -1});
    m_prim.add_vertex({s1, s0, s0}).normal({0, 0, -1});
    m_prim.add_vertex({s0, s0, s0}).normal({0, 0, -1});
    m_prim.add_vertex({s0, s1, s0}).normal({0, 0, -1});
    m_prim.add_triangle_face({12, 13, 14});
    m_prim.add_triangle_face({12, 14, 15});

    m_prim.add_vertex({s0, s1, s1}).normal({0, 0, 1});
    m_prim.add_vertex({s0, s0, s1}).normal({0, 0, 1});
    m_prim.add_vertex({s1, s0, s1}).normal({0, 0, 1});
    m_prim.add_vertex({s1, s1, s1}).normal({0, 0, 1});
    m_prim.add_triangle_face({16, 17, 18});
    m_prim.add_triangle_face({16, 18, 19});

    m_prim.add_vertex({s0, s0, s1}).normal({0, -1, 0});
    m_prim.add_vertex({s0, s0, s0}).normal({0, -1, 0});
    m_prim.add_vertex({s1, s0, s0}).normal({0, -1, 0});
    m_prim.add_vertex({s1, s0, s1}).normal({0, -1, 0});
    m_prim.add_triangle_face({20, 21, 22});
    m_prim.add_triangle_face({20, 22, 23});
}


void Object::update(uint32_t this_object_id, uint32_t selected_object_id)
{
    FragPushConstants push_constants {
        .this_object_id = this_object_id,
        .selected_object_id = selected_object_id,
    };
    m_prim.set_push_constants_data(&push_constants, sizeof(push_constants));
    m_prim.update();
}


void Object::draw(View& view)
{
    m_prim.draw(view, PrimitiveDrawFlags::None);
}


void Object::draw(CommandBuffer& cmd_buf, Attachments& attachments, View& view)
{
    m_prim.draw(cmd_buf, attachments, view, PrimitiveDrawFlags::None);
}


} // namespace xci::graphics
