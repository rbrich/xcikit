// VulkanDevice.cpp created on 2019-10-30, part of XCI toolkit
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

#include "VulkanDevice.h"
#include <xci/core/log.h>

#include <range/v3/view/enumerate.hpp>

#include <bitset>
#include <cstring>

namespace xci::graphics {

using namespace xci::core::log;


VulkanDevice::VulkanDevice(VulkanRenderer& renderer) : m_renderer(renderer) {}


VulkanDevice::~VulkanDevice()
{
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroySurfaceKHR(m_renderer.vk_instance(), m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);
}


void VulkanDevice::init(GLFWwindow* window)
{
    if (glfwCreateWindowSurface(m_renderer.vk_instance(),
                                window, nullptr,
                                &m_surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface!");

    create_device();
    create_swapchain();
}


void VulkanDevice::create_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_renderer.vk_instance(), &device_count, nullptr);
    if (device_count == 0)
        throw std::runtime_error("vulkan: couldn't find any physical device");
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_renderer.vk_instance(), &device_count, devices.data());

    // required device extensions
    const char* const device_extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // queue family index - queried here, used later
    uint32_t graphics_queue_family = 0;

    log_info("Vulkan: {} devices available:", device_count);
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties device_props;
        vkGetPhysicalDeviceProperties(device, &device_props);
        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceFeatures(device, &device_features);

        // choose only the first adequate device
        bool choose = m_physical_device == VK_NULL_HANDLE;

        // check supported queue families
        if (choose) {
            auto family = query_queue_families(device);
            choose = bool(family);
            if (family)
                graphics_queue_family = family.value();
        }

        // check support of required extensions
        if (choose) {
            uint32_t ext_count;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr);
            std::vector<VkExtensionProperties> ext_props(ext_count);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, ext_props.data());
            std::bitset<std::size(device_extensions)> has_exts;
            for (const auto& ext : ext_props) {
                for (size_t i = 0; i < std::size(device_extensions); i++) {
                    if (std::strcmp(ext.extensionName, device_extensions[i]) == 0) {
                        has_exts.set(i);
                        break;
                    }
                }
            }
            choose = has_exts.all();
        }

        // check swapchain
        if (choose) {
            choose = query_swapchain(device);
        }

        // save chosen device handle
        if (choose) {
            m_physical_device = device;
        }

        log_info("({}) {}: {} (api {})",
                 choose ? '*' : ' ',
                 device_props.deviceID,
                 device_props.deviceName, device_props.apiVersion);
    }

    // create VkDevice
    {
        const float queue_priorities[] = {1.0f};
        VkDeviceQueueCreateInfo queue_create_info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphics_queue_family,
                .queueCount = 1,
                .pQueuePriorities = queue_priorities,
        };

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo device_create_info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queue_create_info,
//#ifdef XCI_DEBUG_VULKAN
//                .enabledLayerCount = (uint32_t) enabled_layers.size(),
//                .ppEnabledLayerNames = enabled_layers.data(),
//#endif
                .enabledExtensionCount = std::size(device_extensions),
                .ppEnabledExtensionNames = device_extensions,
                .pEnabledFeatures = &device_features,
        };

        if (vkCreateDevice(m_physical_device, &device_create_info,
                           nullptr, &m_device) != VK_SUCCESS)
            throw std::runtime_error("vkCreateDevice failed");
    }

    vkGetDeviceQueue(m_device, graphics_queue_family,
                     0, &m_graphics_queue);
}


void VulkanDevice::create_swapchain()
{
    VkSwapchainCreateInfoKHR swapchain_create_info {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_surface,
            .minImageCount = m_image_count,
            .imageFormat = m_surface_format.format,
            .imageColorSpace = m_surface_format.colorSpace,
            .imageExtent = m_extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = m_present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
    };

    if (vkCreateSwapchainKHR(m_device, &swapchain_create_info,
            nullptr, &m_swapchain) != VK_SUCCESS)
        throw std::runtime_error("failed to create swap chain!");

    uint32_t image_count;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
    m_images.resize(image_count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_images.data());
}


std::optional<uint32_t>
VulkanDevice::query_queue_families(VkPhysicalDevice device)
{
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

    for (auto&& [i, family] : families | ranges::views::enumerate) {
        // require that the queue supports both graphics and presentation

        if (!(family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            continue;

        VkBool32 surface_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &surface_support);
        if (!surface_support)
            continue;

        return i;
    }
    return {};
}


bool VulkanDevice::query_swapchain(VkPhysicalDevice device)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &capabilities);

    m_extent.width = std::clamp(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    m_extent.height = std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    m_image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && m_image_count > capabilities.maxImageCount)
        m_image_count = capabilities.maxImageCount;

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, formats.data());

    bool fmt_found = false;
    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            m_surface_format = fmt;
            fmt_found = true;
            break;
        }
    }
    if (!fmt_found && format_count > 0)
        m_surface_format = formats[0];

    uint32_t mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &mode_count, nullptr);
    std::vector<VkPresentModeKHR> modes(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &mode_count, modes.data());

    for (const auto mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            m_present_mode = mode;
    }

    return format_count > 0 && mode_count > 0;
}


} // namespace xci::graphics
