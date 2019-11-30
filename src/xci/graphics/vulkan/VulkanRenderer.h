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

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

namespace xci::graphics {


class VulkanRenderer: public Renderer {
public:
    explicit VulkanRenderer(core::Vfs& vfs);
    ~VulkanRenderer() override;

    TexturePtr create_texture() override;

    ShaderPtr create_shader() override;

    PrimitivesPtr create_primitives(VertexFormat format,
                                    PrimitiveType type) override;

    void init(GLFWwindow* m_window);

    // Vulkan handles
    VkInstance vk_instance() const { return m_instance; }
    VkDevice vk_device() const { return m_device; }
    const VkExtent2D& vk_image_extent() const { return m_extent; }
    const VkSurfaceFormatKHR& vk_surface_format() const { return m_surface_format; }

private:
    void create_device();
    void create_swapchain();

    std::optional<uint32_t> query_queue_families(VkPhysicalDevice device);
    bool query_swapchain(VkPhysicalDevice device);

private:
    VkInstance m_instance {};
#ifdef XCI_DEBUG_VULKAN
    VkDebugUtilsMessengerEXT m_debug_messenger {};
#endif
    VkSurfaceKHR m_surface {};
    VkPhysicalDevice m_physical_device {};
    VkDevice m_device {};
    VkQueue m_graphics_queue {};
    VkSwapchainKHR m_swapchain {};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;

    // swapchain create info
    VkSurfaceFormatKHR m_surface_format {};
    VkPresentModeKHR m_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D m_extent {};
    uint32_t m_image_count = 0;
};


} // namespace xci::graphics

#endif // include guard
