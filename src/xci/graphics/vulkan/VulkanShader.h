// VulkanShader.h created on 2019-11-01, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_VULKAN_SHADER_H
#define XCI_GRAPHICS_VULKAN_SHADER_H

#include <xci/graphics/Shader.h>

#include <vulkan/vulkan.h>

namespace xci::graphics {


class VulkanShader : public Shader {
public:
    explicit VulkanShader(VkDevice device) : m_device(device) {}
    ~VulkanShader() override { clear(); }

    bool is_ready() const override;

    // If successful, setup a watch on the file to auto-reload on any change.
    bool load_from_file(const std::string& vertex, const std::string& fragment) override;

    bool load_from_memory(
            const char* vertex_data, int vertex_size,
            const char* fragment_data, int fragment_size) override;

    void set_texture(const char* name, TexturePtr& texture) override;

    // Vulkan handles:
    VkShaderModule vk_vertex_module() const { return m_vertex_module; }
    VkShaderModule vk_fragment_module() const { return m_fragment_module; }

private:
    VkShaderModule create_module(const uint32_t* code, size_t size);
    void clear();

private:
    VkDevice m_device;
    VkShaderModule m_vertex_module {};
    VkShaderModule m_fragment_module {};
};


} // namespace xci::graphics

#endif // include guard
