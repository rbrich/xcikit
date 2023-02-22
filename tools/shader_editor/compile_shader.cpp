// compile_shader.cpp created on 2023-02-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "compile_shader.h"

#include <xci/graphics/Shader.h>
#include <xci/core/log.h>

#include <fmt/core.h>

#include <cstdlib>

using namespace xci::graphics;
using namespace xci::core;


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
