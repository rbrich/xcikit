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

namespace xci {

using xci::core::Vec3;
namespace fs = std::filesystem;


namespace obj {

struct Index {
    unsigned vertex = -1u;
    unsigned tex_coord = -1u;
    unsigned normal = -1u;

    unsigned& operator[] (unsigned i) {
        switch (i) {
            case 0: return vertex;
            case 1: return tex_coord;
            case 2: return normal;
        }
        XCI_UNREACHABLE;
    }

    bool has_tex_coord() const { return tex_coord != -1u; }
    bool has_normal() const { return normal != -1u; }
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
/// See: https://en.wikipedia.org/wiki/Wavefront_.obj_file
struct ObjParser {
    obj::Content content;

    bool parse_file(const fs::path& path);
    bool parse_string(const std::string& str);
};


} // namespace xci

#endif  // XCI_SCENE_OBJ_PARSER_H
