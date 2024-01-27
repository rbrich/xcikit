// Object.h created on 2024-01-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_3D_OBJECT_H
#define XCI_GRAPHICS_3D_OBJECT_H

#include <xci/graphics/Primitives.h>

namespace xci::graphics {


class Object {
public:
    explicit Object(Renderer& renderer);

    /// \param size     Cube size, 1.0 for unit cube
    void create_cube(float size);

    void update(uint32_t this_object_id, uint32_t selected_object_id);

    void draw(View& view);
    void draw(CommandBuffer& cmd_buf, Attachments& attachments, View& view);

    Primitives& prim() { return m_prim; }

private:
    Primitives m_prim;
};


} // namespace xci::graphics

#endif  // XCI_GRAPHICS_3D_OBJECT_H
