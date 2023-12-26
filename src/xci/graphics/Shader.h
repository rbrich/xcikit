// Shader.h created on 2018-04-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHADER_H
#define XCI_GRAPHICS_SHADER_H

#include <xci/vfs/Vfs.h>

#include <vulkan/vulkan.h>

#include <string>
#include <span>
#include <memory>
#include <filesystem>

namespace xci::graphics {

using xci::vfs::Vfs;
namespace fs = std::filesystem;

class Renderer;


std::vector<std::uint32_t> read_spirv_file(const fs::path& pathname);


class ShaderModule {
public:
    explicit ShaderModule(Renderer& renderer);
    ~ShaderModule() { destroy(); }

    /// Create shader directly from memory
    bool create(std::span<const uint32_t> code);

    /// Create shader directly from memory.
    /// Note that the memory MUST be aligned to 4 bytes.
    bool create(const char* data, size_t size);

    /// Load shader from file
    bool load_from_file(const fs::path& path);

    /// Load shader from VFS
    bool load_from_vfs(const Vfs& vfs, const std::string& path);

    explicit operator bool() const { return m_module != VK_NULL_HANDLE; }

    /// Get Vulkan handle
    VkShaderModule vk() const { return m_module; }

private:
    void destroy() { vkDestroyShaderModule(m_device, m_module, nullptr); }

    VkDevice m_device;
    VkShaderModule m_module {};
};


class Shader {
public:
    Shader() = default;
    Shader(const ShaderModule& vertex, const ShaderModule& fragment)
            : m_vertex_module(&vertex)
            , m_fragment_module(&fragment) {}

    /// Is this shader already loaded?
    explicit operator bool() const { return m_vertex_module && m_fragment_module; }

    // Vulkan handles:
    VkShaderModule vk_vertex_module() const { return m_vertex_module->vk(); }
    VkShaderModule vk_fragment_module() const { return m_fragment_module->vk(); }

private:
    const ShaderModule* m_vertex_module = nullptr;
    const ShaderModule* m_fragment_module = nullptr;
};


} // namespace xci::graphics

#endif // include guard
