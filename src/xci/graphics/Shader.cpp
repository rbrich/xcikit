// Shader.cpp created on 2018-09-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Shader.h"
#include "Renderer.h"
#include "vulkan/VulkanError.h"
#include <xci/core/log.h>
#include <xci/core/file.h>

#include <cassert>
#include <cstring>

namespace xci::graphics {

using namespace xci::core;
using namespace xci::core::log;


static std::vector<std::uint32_t> read_spirv_file(const std::string& pathname)
{
    std::ifstream f(pathname, std::ios::ate | std::ios::binary);
    if (!f)
        return {};

    auto file_size = size_t(f.tellg());
    f.seekg(0, std::ios::beg);

    assert(file_size % sizeof(std::uint32_t) == 0);
    std::vector<std::uint32_t> content(file_size / sizeof(std::uint32_t));
    f.read(reinterpret_cast<char*>(content.data()), file_size);
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
    VkShaderModuleCreateInfo module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = size,
            .pCode = code,
    };

    VkShaderModule module;
    VK_TRY("vkCreateShaderModule",
            vkCreateShaderModule(m_device, &module_create_info, nullptr, &module));
    return module;
}


bool Shader::load_from_file(const std::string& vertex, const std::string& fragment)
{
    clear();

    auto vertex_code = read_spirv_file(vertex);
    auto fragment_code = read_spirv_file(fragment);

    if (vertex_code.empty() || fragment_code.empty())
        return false;

    m_vertex_module = create_module(
            vertex_code.data(),
            vertex_code.size() * sizeof(uint32_t));
    m_fragment_module = create_module(
            fragment_code.data(),
            fragment_code.size() * sizeof(uint32_t));

    log_info("Loaded vertex shader: {}", vertex);
    log_info("Loaded fragment shader: {}", fragment);
    return true;
}


bool Shader::load_from_memory(
        const char* vertex_data, int vertex_size,
        const char* fragment_data, int fragment_size)
{
    // copy data to properly aligned memory (we cannot be sure with char* args)
    assert(vertex_size % sizeof(uint32_t) == 0);
    assert(fragment_size % sizeof(uint32_t) == 0);
    auto* vertex_code = new uint32_t[vertex_size / sizeof(uint32_t)];
    auto* fragment_code = new uint32_t[fragment_size / sizeof(uint32_t)];
    std::memcpy(vertex_code, vertex_data, vertex_size);
    std::memcpy(fragment_code, fragment_data, fragment_size);

    clear();
    m_vertex_module = create_module( vertex_code, vertex_size);
    m_fragment_module = create_module(fragment_code, fragment_size);
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
