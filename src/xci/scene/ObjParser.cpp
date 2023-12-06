// ObjParser.cpp created on 2023-11-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ObjParser.h"
#include <xci/core/log.h>

#include <tao/pegtl.hpp>

#include <cmath>  // NAN

namespace xci {

using namespace xci::core;


namespace obj::parser {

using namespace tao::pegtl;

// ----------------------------------------------------------------------------
// Grammar

// Spaces and comments
struct Comment: seq< one<'#'>, until<eolf> > {};
struct SkipWS: star<sor<space, Comment>> {};
struct EndLine: seq<star<blank>, opt<Comment>, eolf> {};
struct Sep: seq<plus<blank>, opt<one<'\\'>, eol, star<blank>>> {};

// Primitives
struct Sign: one<'-','+'> {};
struct Integer: seq<opt<Sign>, plus<digit>> {};

// Command types and args
struct Type: identifier {};
struct Arg: until<at<sor<blank, eolf>>> {};
struct FloatArg: seq<opt<Sign>, plus<digit>, one<'.'>, star<digit>> {};
struct IndexArg: seq<Integer, opt<one<'/'>, Integer>, opt<one<'/'>, Integer>> {};

// Items
struct Vertex: seq<one<'v'>, at<blank>, rep<3, must<Sep, FloatArg>>, opt<Sep, FloatArg>> {};
struct TexCoord: seq<string<'v', 't'>, at<blank>, must<Sep, FloatArg>, opt<Sep, FloatArg>, opt<Sep, FloatArg>> {};
struct Normal: seq<string<'v', 'n'>, at<blank>, rep<3, must<Sep, FloatArg>>> {};
struct Face: seq<string<'f'>, at<blank>, rep<3, must<Sep, IndexArg>>, star<IndexArg>> {};
struct GenericItem: seq<Type, plus<Sep, Arg>> {};
struct Item: sor<Vertex, TexCoord, Normal, Face, GenericItem> {};

// File
struct Line: seq<SkipWS, if_must<at<identifier_first>, Item>, EndLine> {};
struct FileContent: seq<star<Line>, SkipWS, must<eof>> {};

// ----------------------------------------------------------------------------
// Actions

struct State {
    obj::Content& content;
    Vec3<float> float_args {NAN, NAN, NAN};
    Vec3<int> index_arg;
    std::vector<Index> indices;
    std::string usemtl;
    uint32_t n_args = 0;
    bool default_object = true;
    bool default_group = true;

    void reset_float_args() { n_args = 0; float_args = {NAN, NAN, NAN}; }
    void reset_index_arg() { n_args = 0; index_arg = {}; }

    obj::Object& current_object() {
        if (content.object.empty())
            content.object.emplace_back();  // default object
        return content.object.back();
    }
};

struct TypeAndArgs {
    std::string type;
    std::vector<std::string> args;
};

template<typename Rule>
struct Action : nothing<Rule> {};

template<>
struct Action<Type> {
    template<typename Input>
    static void apply(const Input& in, TypeAndArgs& item) {
        item.type = in.string();
    }
};

template<>
struct Action<Arg> {
    template<typename Input>
    static void apply(const Input& in, TypeAndArgs& item) {
        item.args.push_back(in.string());
    }
};

template<>
struct Action<FloatArg> {
    template<typename Input>
    static void apply(const Input& in, State& state) {
        const unsigned i = state.n_args++;
        if (state.n_args > 3)
            return;  // only three floating point args can be remembered
        state.float_args[i] = std::stof(in.string());
    }
};

template<>
struct Action<Vertex> {
    template<typename Input>
    static void apply(const Input& in, State& state) {
        state.content.vertex.push_back(state.float_args);  // optional `w` coord ignored
        state.reset_float_args();
    }
};

template<>
struct Action<TexCoord> {
    template<typename Input>
    static void apply(const Input& in, State& state) {
        state.content.tex_coord.push_back(state.float_args);
        state.reset_float_args();
    }
};

template<>
struct Action<Normal> {
    template<typename Input>
    static void apply(const Input& in, State& state) {
        state.content.normal.push_back(state.float_args);
        state.reset_float_args();
    }
};


template<>
struct Action<Integer> {
    template<typename Input>
    static void apply(const Input& in, State& state) {
        const unsigned i = state.n_args++;
        assert(state.n_args <= 3);
        state.index_arg[i] = std::stoi(in.string());
    }
};

template<>
struct Action<IndexArg> {
    template<typename Input>
    static void apply(const Input& in, State& state) {
        // Convert reference numbers to 0-based global indices
        int ref_v = state.index_arg.x;
        int ref_vt = state.index_arg.y;
        int ref_vn = state.index_arg.z;
        Index idx;
        if (ref_v < 0)
            idx.vertex = unsigned(state.content.vertex.size() - ref_v);
        else if (ref_v > 0)
            idx.vertex = unsigned(ref_v - 1);
        if (ref_vt < 0)
            idx.tex_coord = unsigned(state.content.tex_coord.size() - ref_vt);
        else if (ref_vt > 0)
            idx.tex_coord = unsigned(ref_vt - 1);
        if (ref_vn < 0)
            idx.normal = unsigned(state.content.normal.size() - ref_vn);
        else if (ref_vn > 0)
            idx.normal = unsigned(ref_vn - 1);
        state.indices.push_back(idx);
        state.reset_index_arg();
    }
};

template<>
struct Action<Face> {
    template<typename Input>
    static void apply(const Input& in, State& state) {
        // Assign face to all active groups
        if (!state.content.object.empty()) {
            auto& object = state.content.object.back();
            for (auto& group : object.group) {
                if (group.active)
                    group.faces.push_back(state.content.face.size());
            }
        }
        // Add the face
        auto& face = state.content.face.emplace_back();
        face.index = std::move(state.indices);
        assert(state.indices.empty());
    }
};


template<>
struct Action<GenericItem> : change_states<TypeAndArgs> {
    template<typename Input>
    static void success(const Input& in, TypeAndArgs& item, State& state) {
        if (item.type == "o") {
            auto& object = state.content.object.emplace_back();
            object.name = item.args.front();
            object.first_face_index = state.content.face.size();
            if (!state.usemtl.empty()) {
                if (object.group.empty())
                    object.group.emplace_back().name = "default";
                object.group.back().usemtl = state.usemtl;
            }
            return;
        }
        if (item.type == "g") {
            auto& object = state.current_object();
            for (auto& group : object.group)
                group.active = false;
            for (const auto& arg : item.args) {
                auto it = std::find_if(object.group.begin(), object.group.end(),
                             [&arg](const Group& g){ return g.name == arg; });
                if (it == object.group.end()) {
                    auto& group = object.group.emplace_back();
                    group.name = arg;
                    group.usemtl = state.usemtl;
                } else {
                    it->active = true;
                }
            }
            return;
        }
        if (item.type == "s") {
            // These types are unsupported and just skipped
            return;
        }
        if (item.type == "usemtl") {
            state.usemtl = item.args.front();
            auto& object = state.current_object();
            if (object.group.empty()) {
                object.group.emplace_back().name = "default";
                object.group.back().usemtl = state.usemtl;
            }
            return;
        }
        if (item.type == "mtllib") {
            state.content.mtllib = item.args;
            return;
        }
        log::debug("Skipping unknown item: {} {}", item.type, fmt::join(item.args, " "));
    }
};


// ----------------------------------------------------------------------------
// Control (error reporting)

template< typename Rule >
struct Control : normal< Rule >
{
    static const char* errmsg;

    template< typename Input, typename... States >
    static void raise( const Input& in, States&&... /*unused*/ ) {
        if (errmsg == nullptr) {
            // default message
#ifndef NDEBUG
            throw parse_error( "parse error matching " + std::string(demangle<Rule>()), in );
#else
            throw parse_error( "parse error", in );
#endif
        }
        throw tao::pegtl::parse_error( errmsg, in );
    }
};

template<> const char* Control<eof>::errmsg = "invalid syntax";
template<> const char* Control<Sep>::errmsg = "expected space";
template<> const char* Control<FloatArg>::errmsg = "expected float";
template<> const char* Control<IndexArg>::errmsg = "expected indices";
template<> const char* Control<Item>::errmsg = "expected type or command";

// default message
template<typename T> const char* Control<T>::errmsg = nullptr;

} // namespace obj::parser


template<class T>
static bool _parse_obj(obj::Content& content, T&& in)
{
    using obj::parser::FileContent;
    using obj::parser::Action;
    using obj::parser::Control;
    try {
        return tao::pegtl::parse< FileContent, Action, Control >( in, obj::parser::State{content});
    } catch (const tao::pegtl::parse_error& e) {
        const auto& p = e.positions().front();
        log::error("{}\n{}\n{:>{}}", e.what(), in.line_at(p), '^', p.column);
        return false;
    }
}


bool ObjParser::parse_file(const fs::path& path)
{
    tao::pegtl::file_input in(path);
    return _parse_obj(content, in);
}


bool ObjParser::parse_string(std::string_view sv)
{
    tao::pegtl::memory_input in(sv, "<buffer>");
    return _parse_obj(content, in);
}

} // namespace xci
