// ShaderCompiler.cpp created on 2023-02-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ShaderCompiler.h"

#include <xci/graphics/Shader.h>
#include <xci/core/log.h>

#include <fmt/core.h>
#include <spirv_glsl.hpp>
#include <range/v3/view/enumerate.hpp>

#include <cstdlib>

using namespace xci::graphics;
using namespace xci::core;
using namespace spirv_cross;
using ranges::views::enumerate;


ShaderCompiler::ShaderCompiler()
{
    const char* vulkan_sdk = std::getenv("VULKAN_SDK");
    if (vulkan_sdk)
        m_glslc = fs::path(vulkan_sdk) / "bin/glslc";
}


std::vector<uint32_t>
ShaderCompiler::compile_shader(ShaderStage stage, const fs::path& filename)
{
    fs::path tmp = "/tmp/xci-shader.spv";  // FIXME: mktemp
    std::string cmd = fmt::format("'{}' {} -o {}", m_glslc, filename, tmp);
    if (system(cmd.c_str()) != 0) {
        log::error("Compiling shader file failed: {}", cmd);
        return {};
    }

    return Shader::read_spirv_file(tmp);
}


auto ShaderCompiler::reflect_shader(const std::vector<uint32_t>& spv) -> ShaderResources
{
    const CompilerGLSL glsl(spv);
    auto resources = glsl.get_shader_resources();

    ShaderResources out;

    out.uniform_blocks.reserve(resources.uniform_buffers.size());
    for (const auto& res : resources.uniform_buffers) {
        const unsigned binding = glsl.get_decoration(res.id, spv::DecorationBinding);
        out.uniform_blocks.emplace_back(UniformBlock{
                .type_name = res.name,
                .name = glsl.get_name(res.id),
                .binding = binding,
        });
        auto& block = out.uniform_blocks.back();
        log::info("uniform: binding={} {}: {}", block.binding, block.type_name, block.name);

        const auto& block_type = glsl.get_type(res.base_type_id);
        if (block_type.basetype != SPIRType::Struct) {
            // Only structs (blocks) are supported on top level
            log::error("uniform: binding={} {}: {} is not a struct",
                       block.binding, block.type_name, block.name);
            out.success = false;
            continue;
        }
        block.members.reserve(block_type.member_types.size());
        for (const auto [idx, member_type] : block_type.member_types | enumerate) {
            const auto& type = glsl.get_type(member_type);
            if (type.basetype != SPIRType::Float) {
                // Only floats are supported at member level
                log::error("uniform member: {} is not float or vec", block.name);
                out.success = false;
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
    return out;
}
