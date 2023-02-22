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


std::vector<std::uint32_t> Shader::read_spirv_file(const fs::path& pathname)
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


Shader::Shader(Renderer& renderer) : m_device(renderer.vk_device()) {}


bool Shader::load_from_vfs(const Vfs& vfs, const std::string& vertex, const std::string& fragment)
{
    auto vert_file = vfs.read_file(vertex);
    auto frag_file = vfs.read_file(fragment);
    if (vert_file.is_real_file() && frag_file.is_real_file()) {
        return load_from_file(vert_file.path(), frag_file.path());
    } else {
        auto vert_data = vert_file.content();
        auto frag_data = frag_file.content();
        return load_from_memory(
            reinterpret_cast<const char*>(vert_data->data()), static_cast<int>(vert_data->size()),
            reinterpret_cast<const char*>(frag_data->data()), static_cast<int>(frag_data->size()));
    }
}


VkShaderModule Shader::create_module(const uint32_t* code, size_t size)
{
    const VkShaderModuleCreateInfo module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = size,
            .pCode = code,
    };

    VkShaderModule module;
    VK_TRY("vkCreateShaderModule",
            vkCreateShaderModule(m_device, &module_create_info, nullptr, &module));
    return module;
}


bool Shader::load_from_file(const fs::path& vertex, const fs::path& fragment)
{
    auto vertex_code = read_spirv_file(vertex);
    auto fragment_code = read_spirv_file(fragment);

    if (vertex_code.empty() || fragment_code.empty())
        return false;

    load_from_memory(vertex_code, fragment_code);

    log::info("Loaded vertex shader: {}", vertex);
    log::info("Loaded fragment shader: {}", fragment);
    return true;
}


bool Shader::load_from_memory(
        const char* vertex_data, int vertex_size,
        const char* fragment_data, int fragment_size)
{
    assert(intptr_t(vertex_data) % 4 == 0);  // must have 4-byte alignment
    assert(intptr_t(fragment_data) % 4 == 0);  // must have 4-byte alignment
    assert(vertex_size % sizeof(uint32_t) == 0);  // size must be divisible by 4
    assert(fragment_size % sizeof(uint32_t) == 0);  // size must be divisible by 4

    clear();
    m_vertex_module = create_module(
            reinterpret_cast<const uint32_t*>(vertex_data), vertex_size);
    m_fragment_module = create_module(
            reinterpret_cast<const uint32_t*>(fragment_data), fragment_size);
    return true;
}


bool Shader::load_from_memory(std::span<const uint32_t> vertex_code,
                              std::span<const uint32_t> fragment_code)
{
    clear();
    m_vertex_module = create_module(vertex_code.data(), vertex_code.size_bytes());
    m_fragment_module = create_module(fragment_code.data(), fragment_code.size_bytes());
    return true;
}


bool Shader::is_ready() const
{
    return m_vertex_module != VK_NULL_HANDLE && m_fragment_module != VK_NULL_HANDLE;
}


void Shader::clear()
{
    vkDestroyShaderModule(m_device, m_vertex_module, nullptr);
    vkDestroyShaderModule(m_device, m_fragment_module, nullptr);
}


} // namespace xci::graphics
