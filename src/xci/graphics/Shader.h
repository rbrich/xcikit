// Shader.h created on 2018-04-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHADER_H
#define XCI_GRAPHICS_SHADER_H

#include <xci/graphics/Texture.h>
#include <xci/core/Vfs.h>

#include <vulkan/vulkan.h>

#include <string>
#include <span>
#include <memory>
#include <filesystem>

namespace xci::graphics {

namespace fs = std::filesystem;

class Renderer;


// predefined shaders
enum class ShaderId {
    Sprite = 0,
    SpriteR,  // R channel as alpha, color from uniform
    SpriteC,  // R channel as alpha, color from vertex
    Line,
    LineC,
    Rectangle,
    RectangleC,
    RoundedRectangle,
    RoundedRectangleC,
    Ellipse,
    EllipseC,
    Triangle,
    TriangleC,
    Polygon,
    PolygonC,
    Fps,
    Cursor,

    NumItems_
};


class Shader {
public:
    explicit Shader(Renderer& renderer);
    ~Shader() { clear(); }

    /// Is this shader already loaded?
    bool is_ready() const;

    // Load and compile GLSL program:

    /// Load program from VFS
    /// This in turn calls either `load_from_file` or `load_from_memory`
    /// depending on kind of VfsLoader used (real file or archive)
    bool load_from_vfs(const core::Vfs& vfs, const std::string& vertex, const std::string& fragment);

    /// Load program from a file (possibly adding a file watch for auto-reload)
    bool load_from_file(const fs::path& vertex, const fs::path& fragment);

    /// Load program directly from memory
    /// Note that the memory MUST be aligned to at least 4 bytes.
    bool load_from_memory(
            const char* vertex_data, int vertex_size,
            const char* fragment_data, int fragment_size);

    /// Load program directly from memory
    /// This overloads takes int32 code data.
    bool load_from_memory(std::span<const uint32_t> vertex_code,
                          std::span<const uint32_t> fragment_code);

    // Vulkan handles:
    VkShaderModule vk_vertex_module() const { return m_vertex_module; }
    VkShaderModule vk_fragment_module() const { return m_fragment_module; }

    /// Auxiliary function to read spirv file into int32 vector.
    static std::vector<std::uint32_t> read_spirv_file(const fs::path& pathname);

private:
    VkShaderModule create_module(const uint32_t* code, size_t size);
    void clear();

private:
    VkDevice m_device;
    VkShaderModule m_vertex_module {};
    VkShaderModule m_fragment_module {};
};


} // namespace xci::graphics

#endif // include guard
