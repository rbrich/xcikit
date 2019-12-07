// VulkanShader.cpp created on 2019-11-01, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "VulkanShader.h"
#include "VulkanError.h"
#include <xci/core/log.h>

#include <cassert>
#include <cstring>

namespace xci::graphics {

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


VkShaderModule VulkanShader::create_module(const uint32_t* code, size_t size)
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


bool VulkanShader::load_from_file(const std::string& vertex, const std::string& fragment)
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


bool VulkanShader::load_from_memory(
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


bool VulkanShader::is_ready() const
{
    return m_vertex_module != VK_NULL_HANDLE && m_fragment_module != VK_NULL_HANDLE;
}


void VulkanShader::clear()
{
    vkDestroyShaderModule(m_device, m_vertex_module, nullptr);
    vkDestroyShaderModule(m_device, m_fragment_module, nullptr);
}


void VulkanShader::set_texture(const char* name, TexturePtr& texture)
{

}


} // namespace xci::graphics
