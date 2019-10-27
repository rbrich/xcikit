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
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/compat/macros.h>
#include <range/v3/algorithm/any_of.hpp>
#include <memory>
#include <cstring>

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


VulkanRenderer::VulkanRenderer(core::Vfs& vfs) : Renderer(vfs)
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
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT != nullptr)
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
    vkDestroyInstance(m_instance, nullptr);
    glfwTerminate();
}


TexturePtr VulkanRenderer::create_texture()
{
    return std::make_shared<VulkanTexture>();
}


ShaderPtr VulkanRenderer::create_shader()
{
    return xci::graphics::ShaderPtr();
}


PrimitivesPtr VulkanRenderer::create_primitives(VertexFormat format,
                                                PrimitiveType type)
{
    return xci::graphics::PrimitivesPtr();
}


} // namespace xci::graphics
