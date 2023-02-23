// reflect_shader.h created on 2023-02-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SHED_REFLECT_SHADER_H
#define XCI_SHED_REFLECT_SHADER_H

#include <vector>
#include <string>


class ReflectShader {
public:
    bool reflect(const std::vector<uint32_t>& spv);

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

    const std::vector<UniformBlock>& get_uniform_blocks() const { return m_uniform_blocks; }

private:
    std::vector<UniformBlock> m_uniform_blocks;
};


#endif  // include guard
