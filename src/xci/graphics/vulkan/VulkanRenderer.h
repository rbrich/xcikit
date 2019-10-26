// VulkanRenderer.h created on 2019-10-23, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_VULKAN_RENDERER_H
#define XCI_GRAPHICS_VULKAN_RENDERER_H

#include <xci/graphics/Renderer.h>
#include <xci/config.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace xci::graphics {


class VulkanRenderer: public Renderer {
public:
    explicit VulkanRenderer(core::Vfs& vfs);
    ~VulkanRenderer() override;

    TexturePtr create_texture() override;

    ShaderPtr create_shader() override;

    PrimitivesPtr create_primitives(VertexFormat format,
                                    PrimitiveType type) override;

private:
    bool check_queue_families(const VkPhysicalDevice& device);

private:
    VkInstance m_instance {};
    VkPhysicalDevice m_physical_device {};
    VkDevice m_device {};
    VkQueue m_graphics_queue {};
    size_t m_idx_graphics_queue_family = 0;
#ifdef XCI_DEBUG_VULKAN
    VkDebugUtilsMessengerEXT m_debug_messenger {};
#endif
};


} // namespace xci::graphics

#endif // include guard
