// Renderer.cpp created on 2018-11-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Renderer.h"
#include "vulkan/VulkanError.h"

#include <xci/config.h>
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/compat/macros.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <fmt/core.h>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/take.hpp>
#include <memory>
#include <cstring>
#include <bitset>
#include <array>

namespace xci::graphics {

using namespace xci::core;
using ranges::cpp20::views::take;
using ranges::views::enumerate;
using ranges::cpp20::any_of;
using fmt::format;
using std::make_unique;


static void glfw_error_callback(int error, const char* description)
{
    log::error("GLFW error {}: {}", error, description);
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
    (void) user_data;
    Logger::default_instance().log(
            vulkan_severity_to_log_level(severity),
            format("VK ({}): {}",
                    vulkan_msg_type_to_cstr(msg_type), data->pMessage));
    return VK_FALSE;
}
#endif


Renderer::Renderer(core::Vfs& vfs)
        : m_vfs(vfs)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        VK_THROW("Couldn't initialize GLFW");

    if (!glfwVulkanSupported())
        VK_THROW("Vulkan not supported.");

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
    log::info("Vulkan: {} validation layers available:", layer_count);
    std::vector<const char*> enabled_layers;
    enabled_layers.reserve(layer_count);
    for (const auto& props : layer_props) {
        bool enable = false;
        std::string layer_name(props.layerName);
        if ((
                layer_name.starts_with("VK_LAYER_LUNARG_") ||
                layer_name.starts_with("VK_LAYER_GOOGLE_") ||
                layer_name.starts_with("VK_LAYER_KHRONOS_")
            ) && !any_of(enabled_layers, [&](const char* name) {
                return layer_name == name;
            }) && layer_name != "VK_LAYER_LUNARG_api_dump"
        )
            enable = true;
        log::info("[{}] {} - {} (spec {}, impl {})",
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
    log::info("Vulkan: {} extensions available:", ext_count);
    for (const auto& props : ext_props) {
        bool enable = any_of(extensions, [&](const char* name) {
            return strcmp(name, props.extensionName) == 0;
        });
        log::info("[{}] {} (spec {})",
                 enable ? 'x' : ' ',
                 props.extensionName, props.specVersion);
    }

    instance_create_info.enabledExtensionCount = extensions.size();
    instance_create_info.ppEnabledExtensionNames = extensions.data();

    VK_TRY("vkCreateInstance",
            vkCreateInstance(&instance_create_info, nullptr, &m_instance));

#ifdef XCI_DEBUG_VULKAN
    // create debug messenger
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT == nullptr)
        VK_THROW("vkCreateDebugUtilsMessengerEXT not available");
    VK_TRY("vkCreateDebugUtilsMessengerEXT",
            vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo,
                    nullptr, &m_debug_messenger));
#endif
}


Renderer::~Renderer()
{
    destroy_surface();

#ifdef XCI_DEBUG_VULKAN
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT != nullptr)
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
#endif
    vkDestroyInstance(m_instance, nullptr);
    glfwTerminate();
}


Shader& Renderer::get_shader(ShaderId shader_id)
{
    auto& shader = m_shader[(size_t) shader_id];
    if (!shader) {
        shader = std::make_unique<Shader>(*this);
        if (!load_shader(shader_id, *shader))
            VK_THROW("Shader not loaded!");
    }
    return *shader;
}


bool Renderer::load_shader(ShaderId shader_id, Shader& shader)
{
    switch (shader_id) {
        case ShaderId::Sprite:
            return shader.load_from_vfs(vfs(),
                    "shaders/sprite.vert.spv",
                    "shaders/sprite.frag.spv");
        case ShaderId::SpriteC:
            return shader.load_from_vfs(vfs(),
                    "shaders/sprite_c.vert.spv",
                    "shaders/sprite_c.frag.spv");
        case ShaderId::Line:
            return shader.load_from_vfs(vfs(),
                    "shaders/line.vert.spv",
                    "shaders/line.frag.spv");
        case ShaderId::Rectangle:
            return shader.load_from_vfs(vfs(),
                    "shaders/rectangle.vert.spv",
                    "shaders/rectangle.frag.spv");
        case ShaderId::Ellipse:
            return shader.load_from_vfs(vfs(),
                    "shaders/ellipse.vert.spv",
                    "shaders/ellipse.frag.spv");
        case ShaderId::Fps:
            return shader.load_from_vfs(vfs(),
                    "shaders/fps.vert.spv",
                    "shaders/fps.frag.spv");
        case ShaderId::Cursor:
            return shader.load_from_vfs(vfs(),
                    "shaders/cursor.vert.spv",
                    "shaders/cursor.frag.spv");
        case ShaderId::NumItems_:
            return false;
    }
    UNREACHABLE;
}


void Renderer::clear_shader_cache()
{
    for (auto& shader : m_shader)
        shader.reset();
}


void Renderer::create_surface(GLFWwindow* window)
{
    VK_TRY("glfwCreateWindowSurface",
            glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface));

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    m_extent = { uint32_t(width), uint32_t(height) };

    create_device();
    create_swapchain();
    create_renderpass();
    create_framebuffers();
}


void Renderer::destroy_surface()
{
    if (m_surface == VK_NULL_HANDLE)
        return;

    clear_shader_cache();
    destroy_framebuffers();
    destroy_renderpass();
    destroy_swapchain();
    destroy_device();

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
}


void Renderer::reset_framebuffer(VkExtent2D new_size)
{
    vkDeviceWaitIdle(m_device);

    m_extent = new_size;
    if (!query_swapchain(m_physical_device))
        VK_THROW("vulkan: physical device no longer usable");

    destroy_framebuffers();
    destroy_swapchain();
    create_swapchain();
    create_framebuffers();

    TRACE("framebuffer resized to {}x{}", m_extent.width, m_extent.height);
}


void Renderer::create_device()
{
    constexpr uint32_t max_physical_devices = 20;
    std::array<VkPhysicalDevice, max_physical_devices> devices;  // NOLINT
    uint32_t device_count = max_physical_devices;
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());
    if (device_count == 0)
        VK_THROW("vulkan: couldn't find any physical device");

    // required device extensions
    const char* const device_extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // queue family index - queried here, used later
    uint32_t graphics_queue_family = 0;

    log::info("Vulkan: {} devices available:", device_count);
    for (const auto& device : devices | take(device_count)) {
        VkPhysicalDeviceProperties device_props;
        vkGetPhysicalDeviceProperties(device, &device_props);
        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceFeatures(device, &device_features);

        // choose only the first adequate device
        bool choose = m_physical_device == VK_NULL_HANDLE;

        // check supported queue families
        if (choose) {
            auto family = query_queue_families(device);
            if (family)
                graphics_queue_family = family.value();
            else
                choose = false;
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

        log::info("({}) {}: {} (api {})",
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

        VK_TRY("vkCreateDevice",
                vkCreateDevice(m_physical_device, &device_create_info,
                        nullptr, &m_device));
    }

    vkGetDeviceQueue(m_device, graphics_queue_family,
            0, &m_queue);

    // create VkCommandPool
    {
        VkCommandPoolCreateInfo command_pool_ci = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = graphics_queue_family,
        };
        VK_TRY("vkCreateCommandPool",
                vkCreateCommandPool(m_device, &command_pool_ci,
                        nullptr, &m_command_pool));
    }
    {
        VkCommandPoolCreateInfo command_pool_ci = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queueFamilyIndex = graphics_queue_family,
        };
        VK_TRY("vkCreateCommandPool(TRANSIENT)",
                vkCreateCommandPool(m_device, &command_pool_ci,
                        nullptr, &m_transient_command_pool));
    }
}


void Renderer::destroy_device()
{
    vkDestroyCommandPool(m_device, m_command_pool, nullptr);
    vkDestroyCommandPool(m_device, m_transient_command_pool, nullptr);
    vkDestroyDevice(m_device, nullptr);
}


void Renderer::create_swapchain()
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

    VK_TRY("vkCreateSwapchainKHR",
            vkCreateSwapchainKHR(m_device, &swapchain_create_info,
                    nullptr, &m_swapchain));

    TRACE("Vulkan: swapchain image count: {}", m_image_count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain,
            &m_image_count, nullptr);

    if (m_image_count > max_image_count)
        VK_THROW("vulkan: too many swapchain images");

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_image_count, m_images);
    assert(m_image_count <= max_image_count);

    VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_surface_format.format,
            .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };
    for (size_t i = 0; i < m_image_count; i++) {
        image_view_ci.image = m_images[i];
        VK_TRY("vkCreateImageView",
                vkCreateImageView(m_device, &image_view_ci,
                        nullptr, &m_image_views[i]));
    }
}


void Renderer::destroy_swapchain()
{
    for (auto image_view : m_image_views | take(m_image_count)) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}


void Renderer::create_renderpass()
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
            .dstAccessMask =
                    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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

    VK_TRY("vkCreateRenderPass",
            vkCreateRenderPass(m_device, &render_pass_ci,
                    nullptr, &m_render_pass));
}


void Renderer::destroy_renderpass()
{
    vkDestroyRenderPass(m_device, m_render_pass, nullptr);
}


void Renderer::create_framebuffers()
{
    for (size_t i = 0; i < m_image_count; i++) {
        VkImageView attachments[] = { m_image_views[i] };

        VkFramebufferCreateInfo framebuffer_ci = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_render_pass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = m_extent.width,
                .height = m_extent.height,
                .layers = 1,
        };

        VK_TRY("vkCreateFramebuffer",
                vkCreateFramebuffer(m_device, &framebuffer_ci,
                        nullptr, &m_framebuffers[i]));
    }
}


void Renderer::destroy_framebuffers()
{
    for (auto framebuffer : m_framebuffers | take(m_image_count)) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
}


std::optional<uint32_t>
Renderer::query_queue_families(VkPhysicalDevice device)
{
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

    for (auto&& [i, family] : families | enumerate) {
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


bool Renderer::query_swapchain(VkPhysicalDevice device)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VK_TRY("vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                    device, m_surface, &capabilities));

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

    auto found_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto mode : modes) {
        if (mode == m_present_mode)
            found_mode = mode;
    }
    if (m_present_mode != found_mode) {
        log::warning("vulkan: requested present mode not supported: {}", m_present_mode);
        m_present_mode = found_mode;
    }

    return format_count > 0 && mode_count > 0;
}


void Renderer::set_present_mode(PresentMode mode)
{
    switch (mode) {
        case PresentMode::Immediate:
            m_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        case PresentMode::Mailbox:
            m_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        case PresentMode::Fifo:
            m_present_mode = VK_PRESENT_MODE_FIFO_KHR;
            break;
        case PresentMode::FifoRelaxed:
            m_present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            break;
    }

    // not yet initialized
    if (!m_surface)
        return;

    vkDeviceWaitIdle(m_device);

    if (!query_swapchain(m_physical_device))
        VK_THROW("vulkan: physical device no longer usable");

    destroy_framebuffers();
    destroy_swapchain();
    create_swapchain();
    create_framebuffers();
}


} // namespace xci::graphics
