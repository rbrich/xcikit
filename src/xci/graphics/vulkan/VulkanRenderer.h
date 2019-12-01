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
    VkSwapchainKHR vk_swapchain() const { return m_swapchain; }
    VkQueue vk_queue() const { return m_queue; }
    VkCommandPool vk_command_pool() const { return m_command_pool; }
    const VkExtent2D& vk_image_extent() const { return m_extent; }
    VkRenderPass vk_render_pass() const { return m_render_pass; }
    VkFramebuffer vk_framebuffer(uint32_t index) const { return m_framebuffers[index]; }

private:
    void create_device();
    void create_swapchain();
    void create_renderpass();
    void create_framebuffers();

    std::optional<uint32_t> query_queue_families(VkPhysicalDevice device);
    bool query_swapchain(VkPhysicalDevice device);

private:
    VkInstance m_instance {};
    VkSurfaceKHR m_surface {};
    VkPhysicalDevice m_physical_device {};
    VkDevice m_device {};
    VkQueue m_queue {};
    VkSwapchainKHR m_swapchain {};
    VkRenderPass m_render_pass {};
    VkCommandPool m_command_pool {};

    static constexpr uint32_t max_image_count = 8;
    VkImage m_images[max_image_count];
    VkImageView m_image_views[max_image_count];
    VkFramebuffer m_framebuffers[max_image_count];

    // swapchain create info
    VkSurfaceFormatKHR m_surface_format {};
    VkPresentModeKHR m_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D m_extent {};
    uint32_t m_image_count = 0;  // swapchain image count, N <= max_image_count

#ifdef XCI_DEBUG_VULKAN
    VkDebugUtilsMessengerEXT m_debug_messenger {};
#endif
};


} // namespace xci::graphics

#endif // include guard
