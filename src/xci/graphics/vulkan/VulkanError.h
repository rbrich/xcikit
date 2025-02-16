// vulkan_error.h created on 2019-12-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <vulkan/vulkan_core.h>  // VkResult
#include <xci/core/log.h>
#include <exception>
#include <string>
#include <string_view>
#include <fmt/format.h>

#ifndef XCI_GRAPHICS_VULKAN_ERROR_H
#define XCI_GRAPHICS_VULKAN_ERROR_H

namespace xci::graphics {


static const char *vk_result_to_cstr(VkResult value) {
    switch (value) {
        case VK_SUCCESS: return "SUCCESS";
        case VK_NOT_READY: return "NOT_READY";
        case VK_TIMEOUT: return "TIMEOUT";
        case VK_EVENT_SET: return "EVENT_SET";
        case VK_EVENT_RESET: return "EVENT_RESET";
        case VK_INCOMPLETE: return "INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN: return "ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION: return "ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_SURFACE_LOST_KHR: return "ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV: return "ERROR_INVALID_SHADER_NV";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_NOT_PERMITTED_EXT: return "ERROR_NOT_PERMITTED_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR: return "THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR: return "THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR: return "OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR: return "OPERATION_NOT_DEFERRED_KHR";
        case VK_PIPELINE_COMPILE_REQUIRED_EXT: return "PIPELINE_COMPILE_REQUIRED_EXT";
        default: return "UNKNOWN";
    }
}

inline std::string vk_format_error(std::string_view msg, enum VkResult vk_res)
{
    return vk_res == VK_SUCCESS
                   ? std::string{msg}
                   : fmt::format("{} ({} {})", msg, int(vk_res), vk_result_to_cstr(vk_res));
}

class VulkanError : public std::exception {
public:
    explicit VulkanError(std::string_view msg, enum VkResult vk_res = VK_SUCCESS)
        : m_msg(vk_format_error(msg, vk_res))
        , m_vk_res(vk_res) {}

    const char* what() const noexcept override { return m_msg.c_str(); }

    VkResult vk_result() const noexcept { return m_vk_res; }

private:
    std::string m_msg;
    VkResult m_vk_res;
};


#ifndef VK_THROW
#define VK_THROW(...) \
    throw VulkanError(__VA_ARGS__)
#endif

#ifndef VK_TRY
#define VK_TRY(msg, expr) \
    do { const VkResult res = (expr); if (res != VK_SUCCESS) VK_THROW(msg, res); } while(0)
#endif


inline void vk_log_error(std::string_view msg, enum VkResult vk_res)
{
    xci::core::log::error("VulkanError: {}", vk_format_error(msg, vk_res));
}


#ifndef VK_TRY_RET
#define VK_TRY_RET(msg, expr, ...) \
    do { if (const VkResult res = (expr); res != VK_SUCCESS) { vk_log_error(msg, res); return __VA_ARGS__; } } while(0)
#endif


/// Utility function to get vector data from Vulkan API
template <typename VecT, typename F, typename... Args>
auto vk_get_vector(VecT& res, F&& f, Args&&... args) {
    uint32_t count;
    f(args..., &count, nullptr);
    res.resize(count);
    const auto r = f(args..., &count, res.data());
    res.resize(count);
    return r;
}


} // namespace xci::graphics

#endif // include guard
