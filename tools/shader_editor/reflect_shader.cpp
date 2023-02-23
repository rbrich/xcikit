// reflect_shader.cpp created on 2023-02-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "reflect_shader.h"
#include <spirv_glsl.hpp>
#include <xci/core/log.h>

using namespace xci::core;
using namespace spirv_cross;


bool ReflectShader::reflect(const std::vector<uint32_t>& spv)
{
    const CompilerGLSL glsl(spv);
    auto resources = glsl.get_shader_resources();

    m_uniform_blocks.clear();
    m_uniform_blocks.reserve(resources.uniform_buffers.size());
    bool ok = true;
    for (const auto& res : resources.uniform_buffers) {
        const unsigned binding = glsl.get_decoration(res.id, spv::DecorationBinding);
        m_uniform_blocks.emplace_back(UniformBlock{
                .type_name = res.name,
                .name = glsl.get_name(res.id),
                .binding = binding,
        });
        auto& block = m_uniform_blocks.back();
        log::info("uniform: binding={} {}: {}", block.binding, block.type_name, block.name);

        const auto& block_type = glsl.get_type(res.base_type_id);
        if (block_type.basetype != SPIRType::Struct) {
            // Only structs (blocks) are supported on top level
            log::error("uniform: binding={} {}: {} is not a struct",
                       block.binding, block.type_name, block.name);
            ok = false;
            continue;
        }
        block.members.reserve(block_type.member_types.size());
        unsigned idx = -1;
        for (const auto member_type : block_type.member_types) {
            ++idx;
            const auto& type = glsl.get_type(member_type);
            if (type.basetype != SPIRType::Float) {
                // Only floats are supported at member level
                log::error("uniform member: {} is not float or vec", block.name);
                ok = false;
                continue;
            }
            block.members.emplace_back(BlockMember{
                    .name = glsl.get_member_name(res.base_type_id, idx),
                    .vec_size = type.vecsize,
            });
            const auto& member = block.members.back();
            log::info("  member: {} {}", member.vec_size, member.name);
        }
    }
    return ok;
}
