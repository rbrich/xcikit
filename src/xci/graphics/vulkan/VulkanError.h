// vulkan_error.h created on 2019-12-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <vulkan/vulkan_core.h>  // VkResult
#include <exception>
#include <string>
#include <string_view>
#include <fmt/core.h>

#ifndef XCI_GRAPHICS_VULKAN_ERROR_H
#define XCI_GRAPHICS_VULKAN_ERROR_H

namespace xci::graphics {


class VulkanError : public std::exception {
public:
    explicit VulkanError(std::string_view msg, VkResult vk_res = VK_SUCCESS)
        : m_msg(vk_res == VK_SUCCESS ? msg : fmt::format("{} ({})", msg, vk_res)), m_vk_res(vk_res) {}

    const char* what() const noexcept override { return m_msg.c_str(); }

    VkResult vk_result() const noexcept { return m_vk_res; }

private:
    std::string m_msg;
    VkResult m_vk_res;
};


#ifndef VK_THROW
#define VK_THROW(msg) \
    throw VulkanError(msg)
#endif

#ifndef VK_TRY
#define VK_TRY(msg, expr) \
    do { VkResult res = (expr); if (res != VK_SUCCESS) throw VulkanError(msg, res); } while(0)
#endif


} // namespace xci::graphics

#endif // include guard
