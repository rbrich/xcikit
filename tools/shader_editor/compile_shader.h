// compile_shader.h created on 2023-02-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SHED_COMPILE_SHADER_H
#define XCI_SHED_COMPILE_SHADER_H

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;


enum class ShaderStage {
    Vertex,
    Fragment
};


class ShaderCompiler {
public:
    ShaderCompiler();

    std::vector<uint32_t> compile_shader(ShaderStage stage, const fs::path& filename);

private:
    fs::path m_glslc = "glslc";
};


#endif  // include guard
