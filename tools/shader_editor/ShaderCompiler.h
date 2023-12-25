// ShaderCompiler.h created on 2023-02-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SHED_SHADER_COMPILER_H
#define XCI_SHED_SHADER_COMPILER_H

#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;


enum class ShaderStage {
    Vertex,
    Fragment
};


class ShaderCompiler {
public:
    ShaderCompiler();

    /// \returns Empty vector on error
    std::vector<uint32_t> compile_shader(ShaderStage stage, const fs::path& filename) const;

    struct BlockMember {
        std::string name;
        unsigned vec_size;
    };

    struct UniformBlock {
        std::string type_name;
        std::string name;
        unsigned binding;
        std::vector<BlockMember> members;
    };

    struct ShaderResources {
        std::vector<UniformBlock> uniform_blocks;
        bool success = true;  // was there an error?
        explicit operator bool() const noexcept { return success; }
    };

    /// \returns Empty vector if there are no uniforms in the shader or on error
    ShaderResources reflect_shader(const std::vector<uint32_t>& spv) const;

private:
    fs::path m_glslc = "glslc";
};


#endif  // include guard
