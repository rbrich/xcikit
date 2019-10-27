// VulkanDevice.h created on 2019-10-30, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_VULKAN_DEVICE_H
#define XCI_GRAPHICS_VULKAN_DEVICE_H

#include "VulkanRenderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>

namespace xci::graphics {


class VulkanDevice {
public:
    explicit VulkanDevice(VulkanRenderer& renderer);
    ~VulkanDevice();

    void init(GLFWwindow* m_window);

private:
    void create_device();
    void create_swapchain();

    std::optional<uint32_t> query_queue_families(VkPhysicalDevice device);
    bool query_swapchain(VkPhysicalDevice device);

private:
    VulkanRenderer& m_renderer;
    VkSurfaceKHR m_surface {};
    VkPhysicalDevice m_physical_device {};
    VkDevice m_device {};
    VkQueue m_graphics_queue {};
    VkSwapchainKHR m_swapchain {};
    std::vector<VkImage> m_images;

    // swapchain create info
    VkSurfaceFormatKHR m_surface_format {};
    VkPresentModeKHR m_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D m_extent {};
    uint32_t m_image_count = 0;
};


} // namespace xci::graphics

#endif // include guard
