// ObjParser.h created on 2023-11-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCENE_OBJ_PARSER_H
#define XCI_SCENE_OBJ_PARSER_H

#include <xci/math/Vec3.h>
#include <xci/math/Vec2.h>

#include <filesystem>
#include <vector>
#include <string>
#include <string_view>

namespace xci {

namespace fs = std::filesystem;


namespace obj {

struct Index {
    unsigned vertex = -1u;
    unsigned tex_coord = -1u;
    unsigned normal = -1u;

    bool has_tex_coord() const { return tex_coord != -1u; }
    bool has_normal() const { return normal != -1u; }

    bool operator==(const Index&) const noexcept = default;
};

struct Face {
    std::vector<Index> index;
};

struct Group {
    std::string name;
    std::string usemtl;
    std::vector<unsigned> faces;  // indices into ObjParser::face
    bool active = true;  // any following face will be added to this group
};

struct Object {
    std::string name;
    std::vector<Group> group;
    unsigned first_face_index = 0;
};

struct Content {
    std::vector<Vec3<float>> vertex;
    std::vector<Vec3<float>> tex_coord;
    std::vector<Vec3<float>> normal;
    std::vector<obj::Face> face;
    std::vector<obj::Object> object;
    std::vector<std::string> mtllib;
};

}  // namespace obj


/// Loads Wavefront .obj file into memory structures
/// See:
/// * https://en.wikipedia.org/wiki/Wavefront_.obj_file
/// * https://fegemo.github.io/cefet-cg/attachments/obj-spec.pdf
struct ObjParser {
    obj::Content content;

    bool parse_file(const fs::path& path);
    bool parse_string(std::string_view sv);
};


} // namespace xci


// Specialize std::hash for obj::Index
template<>
struct std::hash<xci::obj::Index>
{
    std::size_t operator()(const xci::obj::Index& v) const noexcept {
        std::size_t h1 = size_t(v.vertex);
        std::size_t h2 = size_t(v.tex_coord);
        std::size_t h3 = size_t(v.normal);
#if SIZE_MAX == UINT64_MAX
        return h1 ^ (h2 << 21) ^ (h3 << 42);
#else
        return h1 ^ (h2 << 11) ^ (h3 << 22);
#endif
    }
};


#endif  // XCI_SCENE_OBJ_PARSER_H
