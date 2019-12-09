// Renderer.h created on 2018-04-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_RENDERER_H
#define XCI_GRAPHICS_RENDERER_H

#include "Texture.h"
#include "Shader.h"
#include "Primitives.h"
#include <xci/core/geometry.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <optional>
#include <memory>
#include <array>
#include <cstdint>

namespace xci::graphics {


class Renderer {
public:
    explicit Renderer(core::Vfs& vfs);
    ~Renderer();

    core::Vfs& vfs() { return m_vfs; }

    /// Get one of the predefined shaders
    /// \param shader_id Use `Custom` to create new shader
    /// \return shared_ptr to the shader or nullptr on error
    Shader& get_shader(ShaderId shader_id);

    /// Load one of the predefined shaders
    /// This creates new Shader object. Use `get_shader` to get one
    /// from the cache instead.
    bool load_shader(ShaderId shader_id, Shader& shader);

    void clear_shader_cache();

    void init(GLFWwindow* m_window);
    void reset_framebuffer(VkExtent2D new_size = {0, 0});

    // Vulkan handles
    VkInstance vk_instance() const { return m_instance; }
    VkDevice vk_device() const { return m_device; }
    VkPhysicalDevice vk_physical_device() const { return m_physical_device; }
    VkSwapchainKHR vk_swapchain() const { return m_swapchain; }
    VkQueue vk_queue() const { return m_queue; }
    VkCommandPool vk_command_pool() const { return m_command_pool; }
    VkCommandPool vk_transient_command_pool() const { return m_transient_command_pool; }
    const VkExtent2D& vk_image_extent() const { return m_extent; }
    VkRenderPass vk_render_pass() const { return m_render_pass; }
    VkFramebuffer vk_framebuffer(uint32_t index) const { return m_framebuffers[index]; }

private:
    void create_device();
    void create_swapchain();
    void create_renderpass();
    void create_framebuffers();
    void destroy_swapchain();
    void destroy_framebuffers();

    std::optional<uint32_t> query_queue_families(VkPhysicalDevice device);
    bool query_swapchain(VkPhysicalDevice device);

private:
    core::Vfs& m_vfs;
    static constexpr auto c_num_shaders = (size_t)ShaderId::_count;
    std::array<std::unique_ptr<Shader>, c_num_shaders> m_shader = {};

    VkInstance m_instance {};
    VkSurfaceKHR m_surface {};
    VkPhysicalDevice m_physical_device {};
    VkDevice m_device {};
    VkQueue m_queue {};
    VkSwapchainKHR m_swapchain {};
    VkRenderPass m_render_pass {};
    VkCommandPool m_command_pool {};
    VkCommandPool m_transient_command_pool {};

    static constexpr uint32_t max_image_count = 8;
    VkImage m_images[max_image_count] {};
    VkImageView m_image_views[max_image_count] {};
    VkFramebuffer m_framebuffers[max_image_count] {};

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
