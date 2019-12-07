// vulkan_error.h created on 2019-12-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <exception>
#include <string>

#ifndef XCI_GRAPHICS_VULKAN_ERROR_H
#define XCI_GRAPHICS_VULKAN_ERROR_H

namespace xci::graphics {


class VulkanError : public std::exception {
public:
    explicit VulkanError(std::string msg) : m_msg(std::move(msg)) {}

    const char* what() const noexcept override { return m_msg.c_str(); }

private:
    std::string m_msg;
};


#ifndef VK_THROW
#define VK_THROW(msg) \
    throw VulkanError(msg)
#endif

#ifndef VK_TRY
#define VK_TRY(msg, expr) \
    do { if ((expr) != VK_SUCCESS) throw VulkanError(msg); } while(0)
#endif


} // namespace xci::graphics

#endif // include guard
