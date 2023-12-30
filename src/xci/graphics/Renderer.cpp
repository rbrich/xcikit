// Renderer.cpp created on 2018-11-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Renderer.h"
#include "vulkan/VulkanError.h"

#include <xci/config.h>
#include <xci/core/log.h>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <SDL_hints.h>

#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/take.hpp>

#include <fmt/core.h>

#include <memory>
#include <bitset>
#include <array>
#include <bit>
#include <cstring>
#include <cassert>

namespace xci::graphics {

using namespace xci::core;
using ranges::cpp20::views::take;
using ranges::views::enumerate;
using ranges::cpp20::any_of;


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
            [[fallthrough]];
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
            fmt::format("VK ({}): {}",
                    vulkan_msg_type_to_cstr(msg_type), data->pMessage));
    return VK_FALSE;
}
#endif


Renderer::Renderer(Vfs& vfs)
        : m_vfs(vfs)
{
#if SDL_VERSION_ATLEAST(2,24,0)
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        VK_THROW(fmt::format("Couldn't initialize SDL: {}", SDL_GetError()));
}


bool Renderer::create_instance(SDL_Window* window)
{
    const VkApplicationInfo application_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "xcikit app",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "xci-graphics",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_1,
    };

    VkInstanceCreateInfo instance_create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef VK_KHR_portability_enumeration
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
            .pApplicationInfo = &application_info,
    };

    std::vector<const char *> extensions;
    {
        unsigned int sdlExtensionCount = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
        extensions.resize(sdlExtensionCount);
        SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensions.data());
        extensions.resize(sdlExtensionCount);
    }

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
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
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

#ifdef VK_KHR_portability_enumeration
    // Required for MoltenVK
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    uint32_t ext_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> ext_props(ext_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, ext_props.data());
    log::info("Vulkan: {} extensions available:", ext_count);
    for (const auto& props : ext_props) {
        const bool enable = any_of(extensions, [&](const char* name) {
            return strcmp(name, props.extensionName) == 0;
        });
        log::info("[{}] {} (spec {})",
                 enable ? 'x' : ' ',
                 props.extensionName, props.specVersion);
    }

    instance_create_info.enabledExtensionCount = (uint32_t) extensions.size();
    instance_create_info.ppEnabledExtensionNames = extensions.data();

    VK_TRY_RET("vkCreateInstance",
            vkCreateInstance(&instance_create_info, nullptr, &m_instance));

#ifdef XCI_DEBUG_VULKAN
    // create debug messenger
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT == nullptr) {
        log::error("VulkanError: {}", "vkCreateDebugUtilsMessengerEXT not available");
        return false;
    }
    VK_TRY_RET("vkCreateDebugUtilsMessengerEXT",
            vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo,
                    nullptr, &m_debug_messenger));
#endif

    return true;
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
    SDL_Quit();
}


Shader Renderer::get_shader(std::string_view vert_name, std::string_view frag_name)
{
    const auto vert_path = fmt::format("shaders/{}.vert.spv", vert_name);
    auto* vert_module = load_shader_module(vert_path);
    if (!vert_module)
        VK_THROW(std::string("Failed to load shader: ") + vert_path);

    const auto frag_path = fmt::format("shaders/{}.frag.spv", frag_name);
    auto* frag_module = load_shader_module(frag_path);
    if (!frag_module)
        VK_THROW(std::string("Failed to load shader: ") + frag_path);

    return Shader(*vert_module, *frag_module);
}


ShaderModule* Renderer::load_shader_module(const std::string& vfs_path)
{
    auto [it, inserted] = m_shader_module.try_emplace(vfs_path, *this);
    auto& shader_module = it->second;
    if (!inserted || shader_module.load_from_vfs(m_vfs, vfs_path))
        return &shader_module;
    return {};
}


Sampler& Renderer::get_sampler(SamplerAddressMode address_mode, float anisotropy)
{
    SamplerCreateInfo ci(address_mode, std::min(anisotropy, m_max_sampler_anisotropy));
    auto [it, added] = m_sampler.try_emplace(ci);
    if (added)
        it->second.create(vk_device(), ci);
    return it->second;
}


void Renderer::clear_sampler_cache()
{
    for (auto&& [ci, sampler] : m_sampler)
        sampler.destroy(vk_device());
    m_sampler.clear();
}


PipelineLayout& Renderer::get_pipeline_layout(const PipelineLayoutCreateInfo& ci)
{
    auto [it, added] = m_pipeline_layout.try_emplace(ci, *this, ci);
    return it->second;
}


Pipeline& Renderer::get_pipeline(const PipelineCreateInfo& ci)
{
    auto [it, added] = m_pipeline.try_emplace(ci, *this, ci);
    return it->second;
}


void Renderer::clear_pipeline_cache()
{
    m_pipeline_layout.clear();
    m_pipeline.clear();
}


SharedDescriptorPool
Renderer::get_descriptor_pool(uint32_t reserved_sets, DescriptorPoolSizes pool_sizes)
{
    auto [it, added] = m_descriptor_pool.try_emplace(pool_sizes);
    auto& vec_of_pools = it->second;
    for (auto& pool : vec_of_pools) {
        if (pool.book_capacity(reserved_sets))
            return SharedDescriptorPool(pool, reserved_sets);
    }
    // None of existing pools had enough capacity
    auto& pool = vec_of_pools.emplace_back(*this);
    pool.create(1000, pool_sizes);
    if (pool.book_capacity(reserved_sets))
        return SharedDescriptorPool(pool, reserved_sets);
    // reserved_sets is > 1000
    VK_THROW("Can't reserve " + std::to_string(reserved_sets) + " descriptor sets.");
}


bool Renderer::create_surface(SDL_Window* window)
{
    if (!create_instance(window))
        return false;

    if (SDL_Vulkan_CreateSurface(window, m_instance, &m_surface) == SDL_FALSE) {
        log::error("{} failed", "SDL_Vulkan_CreateSurface");
        return false;
    }

    try {
        create_device();
    } catch (const VulkanError& e) {
        log::error("VulkanError: {}", e.what());
        return false;
    }

    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    try {
        m_swapchain.query_surface_capabilities(m_physical_device, { uint32_t(width), uint32_t(height) });
    } catch (const VulkanError& e) {
        log::error("VulkanError: {}", e.what());
        return false;
    }

    if (!m_swapchain.query(m_physical_device)) {
        log::error("vulkan: physical device no longer usable");
        return false;
    }

    m_swapchain.create();
    create_renderpass();
    m_swapchain.create_framebuffers();
    return true;
}


void Renderer::destroy_surface()
{
    if (m_surface == VK_NULL_HANDLE)
        return;

    clear_shader_cache();
    clear_sampler_cache();
    clear_pipeline_cache();
    clear_descriptor_pool_cache();
    m_swapchain.destroy_framebuffers();
    destroy_renderpass();
    m_swapchain.destroy();
    destroy_device();

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
}


void Renderer::create_device()
{
    constexpr uint32_t max_physical_devices = 20;
    std::array<VkPhysicalDevice, max_physical_devices> devices;  // NOLINT
    uint32_t device_count = max_physical_devices;
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());
    if (device_count == 0)
        VK_THROW("vulkan: couldn't find any physical device");

    // device extensions
    const char* const required_device_extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    const char* const additional_device_extensions[] = {
            "VK_KHR_portability_subset",  // required if present on the device
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,  // enable option to flip Y for OpenGL compatibility
    };
    std::vector<const char*> chosen_device_extensions;

    // queue family index - queried here, used later
    uint32_t graphics_queue_family = 0;

    log::info("Vulkan: {} devices available:", device_count);
    for (const auto& device : devices | take(device_count)) {
        VkPhysicalDeviceProperties device_props;
        vkGetPhysicalDeviceProperties(device, &device_props);
        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceFeatures(device, &device_features);

        // Choose the first adequate device,
        // or the one selected by set_device_id
        bool choose = (m_physical_device == VK_NULL_HANDLE) && \
            (m_device_id == ~0u || m_device_id == device_props.deviceID);

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
            std::bitset<std::size(required_device_extensions)> has_exts;
            std::bitset<std::size(additional_device_extensions)> add_exts;
            for (const auto& ext : ext_props) {
                for (size_t i = 0; i < std::size(required_device_extensions); i++) {
                    if (std::strcmp(ext.extensionName, required_device_extensions[i]) == 0) {
                        has_exts.set(i);
                        break;
                    }
                }

                for (size_t i = 0; i < std::size(additional_device_extensions); i++) {
                    if (std::strcmp(ext.extensionName, additional_device_extensions[i]) == 0) {
                        add_exts.set(i);
                        break;
                    }
                }
            }
            choose = has_exts.all();
            if (choose) {
                chosen_device_extensions.reserve(
                        std::size(required_device_extensions) + std::size(additional_device_extensions));
                std::copy(std::begin(required_device_extensions), std::end(required_device_extensions),
                          std::back_inserter(chosen_device_extensions));
                for (size_t i = 0; i < std::size(additional_device_extensions); i++) {
                    if (add_exts[i])
                        chosen_device_extensions.push_back(additional_device_extensions[i]);
                }
            }
        }

        // check swapchain
        if (choose) {
            choose = m_swapchain.query(device);
        }

        // save chosen device handle
        if (choose) {
            m_physical_device = device;
            load_device_properties(device_props);
        }

        if (m_device_id == device_props.deviceID && !choose) {
            log::error("Chosen device ID not usable: {}", m_device_id);
            VK_THROW("Chosen device ID not usable");
        }

        log::info("({}) {}: {} (api {})",
                choose ? '*' : ' ',
                device_props.deviceID,
                device_props.deviceName, device_props.apiVersion);
    }

    if (!m_physical_device) {
        VK_THROW("Did not found any usable device");
    }

    // create VkDevice
    {
        const float queue_priorities[] = {1.0f};
        const VkDeviceQueueCreateInfo queue_create_info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphics_queue_family,
                .queueCount = 1,
                .pQueuePriorities = queue_priorities,
        };

        const VkPhysicalDeviceFeatures device_features = {};

        const VkDeviceCreateInfo device_create_info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queue_create_info,
                .enabledExtensionCount = (uint32_t) chosen_device_extensions.size(),
                .ppEnabledExtensionNames = chosen_device_extensions.data(),
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
        const VkCommandPoolCreateInfo command_pool_ci = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = graphics_queue_family,
        };
        VK_TRY("vkCreateCommandPool",
                vkCreateCommandPool(m_device, &command_pool_ci,
                        nullptr, &m_command_pool));
    }
    {
        const VkCommandPoolCreateInfo command_pool_ci = {
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
    if (m_device == VK_NULL_HANDLE)
        return;
    vkDestroyCommandPool(m_device, m_command_pool, nullptr);
    vkDestroyCommandPool(m_device, m_transient_command_pool, nullptr);
    vkDestroyDevice(m_device, nullptr);
}


void Renderer::create_renderpass()
{
    const VkAttachmentDescription attachment[] = {
        // color attachment
        {
            .format = m_swapchain.vk_surface_format().format,
            .samples = m_swapchain.sample_count(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = m_swapchain.is_multisample() ? VK_ATTACHMENT_STORE_OP_DONT_CARE :
                                                      VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = m_swapchain.is_multisample() ?
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        // depth attachment
        {
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = m_swapchain.sample_count(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
        // resolve attachment for MSAA
        {
            .format = m_swapchain.vk_surface_format().format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
       }
    };

    const VkAttachmentReference color_attachment_ref = {
            .attachment = 0,  // layout(location = 0)
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_attachment_ref = {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference resolve_attachment_ref = {
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_ref,
            .pResolveAttachments = m_swapchain.is_multisample() ? &resolve_attachment_ref : nullptr,
            .pDepthStencilAttachment = depth_buffering() ? &depth_attachment_ref : nullptr,
    };

    const VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            (depth_buffering() ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT : 0u),
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            (depth_buffering() ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT : 0u),
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             (depth_buffering() ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0u),
    };

    const VkRenderPassCreateInfo render_pass_ci = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1 + uint32_t(depth_buffering()) + uint32_t(m_swapchain.is_multisample()),
            .pAttachments = attachment,
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
    if (m_device != VK_NULL_HANDLE)
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);
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


void Renderer::load_device_properties(const VkPhysicalDeviceProperties& props)
{
    m_max_image_dimension_2d = props.limits.maxImageDimension2D;
    m_min_uniform_offset_alignment = props.limits.minUniformBufferOffsetAlignment;
    m_max_sampler_anisotropy = props.limits.maxSamplerAnisotropy;

    // Max sample count for combined color & depth buffer
    VkSampleCountFlags flags = props.limits.framebufferColorSampleCounts &
                               props.limits.framebufferDepthSampleCounts;
    m_max_sample_count = (VkSampleCountFlagBits) std::bit_floor(flags);
    set_sample_count(std::min(sample_count(), max_sample_count()));
}


} // namespace xci::graphics
