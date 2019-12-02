// VulkanRenderer.cpp created on 2019-10-23, part of XCI toolkit
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

#include "VulkanRenderer.h"
#include "VulkanTexture.h"
#include "VulkanShader.h"
#include "VulkanPrimitives.h"
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/compat/macros.h>

#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/view/enumerate.hpp>

#include <memory>
#include <cstring>
#include <bitset>

namespace xci::graphics {

using namespace xci::core;
using namespace xci::core::log;
using std::make_unique;


static void glfw_error_callback(int error, const char* description)
{
    log_error("GLFW error {}: {}", error, description);
}


#ifdef XCI_DEBUG_VULKAN
static Logger::Level vulkan_severity_to_log_level(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return Logger::Level::Debug;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return Logger::Level::Info;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return Logger::Level::Warning;
        default:
            assert(!"unexpected log level");
            FALLTHROUGH;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return Logger::Level::Error;
    }
}

static const char* vulkan_msg_type_to_cstr(
        VkDebugUtilsMessageTypeFlagsEXT msg_type)
{
    switch (msg_type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            return "general";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            return "validation";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            return "performance";
        default:
            return "unknown";
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT msg_type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* user_data)
{
    Logger::default_instance().log(
            vulkan_severity_to_log_level(severity),
            format("VK ({}): {}",
                    vulkan_msg_type_to_cstr(msg_type), data->pMessage));
    return VK_FALSE;
}
#endif


VulkanRenderer::VulkanRenderer(core::Vfs& vfs)
    : Renderer(vfs)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        log_error("Couldn't initialize GLFW...");
    }

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("Vulkan not supported.");
    }

    VkApplicationInfo application_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "an xci-graphics based app",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "xci-graphics",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_1,
    };

    VkInstanceCreateInfo instance_create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &application_info,
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef XCI_DEBUG_VULKAN
    // enable validation layers
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> layer_props(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layer_props.data());
    log_info("Vulkan: {} validation layers available:", layer_count);
    std::vector<const char*> enabled_layers;
    enabled_layers.reserve(layer_count);
    for (const auto& props : layer_props) {
        bool enable = false;
        if ((
                starts_with(props.layerName, "VK_LAYER_LUNARG_") ||
                starts_with(props.layerName, "VK_LAYER_GOOGLE_") ||
                starts_with(props.layerName, "VK_LAYER_KHRONOS_")
            ) && !ranges::any_of(enabled_layers,[&](const char* name) {
                return strcmp(name, props.layerName) == 0;
            }) && strcmp(props.layerName, "VK_LAYER_LUNARG_api_dump") != 0
        )
            enable = true;
        log_info("[{}] {} - {} (spec {}, impl {})",
                enable ? 'x' : ' ',
                props.layerName, props.description,
                props.specVersion, props.implementationVersion);
        if (enable)
            enabled_layers.push_back(props.layerName);
    }
    instance_create_info.enabledLayerCount = enabled_layers.size();
    instance_create_info.ppEnabledLayerNames = enabled_layers.data();

    // setup debug messenger
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = vulkan_debug_callback;

    // this should enable debug messenger for create/destroy of the instance itself
    instance_create_info.pNext = &debugCreateInfo;
#endif

    uint32_t ext_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> ext_props(ext_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, ext_props.data());
    log_info("Vulkan: {} extensions available:", ext_count);
    for (const auto& props : ext_props) {
        bool enable = ranges::any_of(extensions,[&](const char* name) {
            return strcmp(name, props.extensionName) == 0;
        });
        log_info("[{}] {} (spec {})",
                 enable ? 'x' : ' ',
                 props.extensionName, props.specVersion);
    }

    instance_create_info.enabledExtensionCount = extensions.size();
    instance_create_info.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("vulkan: failed to create VkInstance");

#ifdef XCI_DEBUG_VULKAN
    // create debug messenger
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT == nullptr)
        throw std::runtime_error("vkCreateDebugUtilsMessengerEXT not available");
    if (vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
#endif
}


VulkanRenderer::~VulkanRenderer()
{
    destroy_framebuffers();
    vkDestroyRenderPass(m_device, m_render_pass, nullptr);
    destroy_swapchain();
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyCommandPool(m_device, m_command_pool, nullptr);
    vkDestroyDevice(m_device, nullptr);
#ifdef XCI_DEBUG_VULKAN
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT != nullptr)
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
#endif
    vkDestroyInstance(m_instance, nullptr);
    glfwTerminate();
}


TexturePtr VulkanRenderer::create_texture()
{
    return std::make_shared<VulkanTexture>();
}


ShaderPtr VulkanRenderer::create_shader()
{
    return std::make_shared<VulkanShader>(m_device);
}


PrimitivesPtr VulkanRenderer::create_primitives(VertexFormat format,
                                                PrimitiveType type)
{
    return std::make_shared<VulkanPrimitives>(*this, format, type);
}


void VulkanRenderer::init(GLFWwindow* window)
{
    if (glfwCreateWindowSurface(m_instance,
            window, nullptr,
            &m_surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface!");

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    m_extent = { uint32_t(width), uint32_t(height) };

    create_device();
    create_swapchain();
    create_renderpass();
    create_framebuffers();
}


void VulkanRenderer::reset_framebuffer(VkExtent2D new_size)
{
    vkDeviceWaitIdle(m_device);

    m_extent = new_size;
    if (!query_swapchain(m_physical_device))
        throw std::runtime_error("vulkan: physical device no longer usable");

    destroy_framebuffers();
    destroy_swapchain();
    create_swapchain();
    create_framebuffers();

    TRACE("framebuffer resized to {}x{}", m_extent.width, m_extent.height);
}


void VulkanRenderer::create_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
    if (device_count == 0)
        throw std::runtime_error("vulkan: couldn't find any physical device");
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

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
            0, &m_queue);

    // create VkCommandPool
    VkCommandPoolCreateInfo command_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_queue_family,
    };
    if (vkCreateCommandPool(m_device, &command_pool_ci,
            nullptr, &m_command_pool) != VK_SUCCESS)
        throw std::runtime_error("failed to create command pool!");
}


void VulkanRenderer::create_swapchain()
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
    TRACE("Vulkan: swapchain image count: {}", m_image_count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain,
            &m_image_count, nullptr);

    if (m_image_count > max_image_count)
        throw std::runtime_error("vulkan: too many swapchain images");

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_image_count, m_images);
    assert(m_image_count <= max_image_count);

    VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_surface_format.format,
            .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };
    for (size_t i = 0; i < m_image_count; i++) {
        image_view_create_info.image = m_images[i];
        if (vkCreateImageView(m_device, &image_view_create_info,
                nullptr, &m_image_views[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create image views!");
    }
}


void VulkanRenderer::create_renderpass()
{
    VkAttachmentDescription color_attachment = {
            .format = m_surface_format.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
            .attachment = 0,  // layout(location = 0)
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_ref,
    };

    VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                    | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo render_pass_ci = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &color_attachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
    };

    if (vkCreateRenderPass(m_device, &render_pass_ci,
            nullptr, &m_render_pass) != VK_SUCCESS)
        throw std::runtime_error("failed to create render pass!");
}


void VulkanRenderer::create_framebuffers()
{
    for (size_t i = 0; i < m_image_count; i++) {
        VkImageView attachments[] = {
            m_image_views[i]
        };

        VkFramebufferCreateInfo framebuffer_ci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_render_pass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = m_extent.width,
            .height = m_extent.height,
            .layers = 1,
        };

        if (vkCreateFramebuffer(m_device, &framebuffer_ci,
                nullptr, &m_framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create framebuffer!");
    }
}


std::optional<uint32_t>
VulkanRenderer::query_queue_families(VkPhysicalDevice device)
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


bool VulkanRenderer::query_swapchain(VkPhysicalDevice device)
{
    VkSurfaceCapabilitiesKHR capabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &capabilities) != VK_SUCCESS)
        throw std::runtime_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");

    if (capabilities.currentExtent.width != UINT32_MAX)
        m_extent = capabilities.currentExtent;

    m_extent.width = std::clamp(m_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    m_extent.height = std::clamp(m_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    // evaluate min image count
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


void VulkanRenderer::destroy_swapchain()
{
    for (auto image_view : m_image_views) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}


void VulkanRenderer::destroy_framebuffers()
{
    for (auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
}


} // namespace xci::graphics
