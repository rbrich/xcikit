// Shader.cpp created on 2018-09-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Shader.h"
#include "Renderer.h"
#include "vulkan/VulkanError.h"
#include <xci/core/log.h>
#include <xci/core/file.h>

#include <cassert>

namespace xci::graphics {

using namespace xci::core;


/// Auxiliary function to read spirv file into int32 vector.
std::vector<std::uint32_t> read_spirv_file(const fs::path& pathname)
{
    std::ifstream f(pathname, std::ios::ate | std::ios::binary);
    if (!f)
        return {};

    auto file_size = size_t(f.tellg());
    f.seekg(0, std::ios::beg);

    assert(file_size % sizeof(std::uint32_t) == 0);
    std::vector<std::uint32_t> content(file_size / sizeof(std::uint32_t));
    f.read(reinterpret_cast<char*>(content.data()), std::streamsize(file_size));
    if (!f)
        content.clear();

    return content;
}


ShaderModule::ShaderModule(Renderer& renderer) : m_device(renderer.vk_device()) {}


bool ShaderModule::create(std::span<const uint32_t> code)
{
    if (m_module)
        destroy();
    const VkShaderModuleCreateInfo module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size_bytes(),
            .pCode = code.data(),
    };
    VK_TRY_RET("vkCreateShaderModule",
           vkCreateShaderModule(m_device, &module_create_info, nullptr, &m_module),
           false);
    return true;
}


bool ShaderModule::create(const char* data, size_t size)
{
    assert(intptr_t(data) % 4 == 0);  // must have 4-byte alignment
    assert(size % 4 == 0);  // size must be divisible by 4

    return create({reinterpret_cast<const uint32_t*>(data), size / sizeof(uint32_t)});
}


bool ShaderModule::load_from_file(const fs::path& path)
{
    log::info("Loading shader: {}", path);
    auto code = read_spirv_file(path);
    if (code.empty())
        return false;
    return create(code);
}


bool ShaderModule::load_from_vfs(const Vfs& vfs, const std::string& path)
{
    log::info("Loading shader: {}", path);
    auto data = vfs.read_file(path).content();
    if (!data)
        return false;
    return create(reinterpret_cast<const char*>(data->data()), data->size());
}


} // namespace xci::graphics
