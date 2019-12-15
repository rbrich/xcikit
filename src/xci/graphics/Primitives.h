// Primitives.h created on 2018-04-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_PRIMITIVES_H
#define XCI_GRAPHICS_PRIMITIVES_H

#include "View.h"
#include "Window.h"
#include "Color.h"
#include "Shader.h"
#include "Texture.h"
#include "vulkan/DeviceMemory.h"
#include <xci/core/geometry.h>
#include <xci/core/NonCopyable.h>

#include <vulkan/vulkan.h>

#include <array>

namespace xci::graphics {

class Shader;
class Renderer;


enum class VertexFormat {
    V2t2,       // 2 vertex coords, 2 texture coords (all float)
    V2t22,      // 2 vertex coords, 2 + 2 texture coords (all float)
    V2c4t2,     // 2 vertex coords, RGBA color, 2 texture coords (all float)
    V2c4t22,    // 2 vertex coords, RGBA color, 2 + 2 texture coords (all float)
};

enum class PrimitiveType {
    TriFans,        // also usable as quads
};


enum class BlendFunc {
    Off,
    AlphaBlend,
    InverseVideo,
};


class Primitives: private core::NonCopyable {
public:
    explicit Primitives(Renderer& renderer, VertexFormat format, PrimitiveType type);
    ~Primitives();

    /// Reserve memory for primitives that will be added later
    /// \param vertices     expected total number of vertices in all primitives
    void reserve(size_t vertices);

    void begin_primitive();
    void end_primitive();
    void add_vertex(ViewportCoords xy, float u, float v);
    void add_vertex(ViewportCoords xy, float u1, float v1, float u2, float v2);
    void add_vertex(ViewportCoords xy, Color color, float u, float v);
    void add_vertex(ViewportCoords xy, Color color, float u1, float v1, float u2, float v2);
    void clear();
    bool empty() const { return m_vertex_data.empty(); }

    void set_shader(Shader& shader);

    void clear_uniforms();
    void add_uniform_data(uint32_t binding, const void* data, size_t size);
    void add_uniform(uint32_t binding, float f) { add_uniform_data(binding, &f, sizeof(f)); }
    void add_uniform(uint32_t binding, float f1, float f2);
    void add_uniform(uint32_t binding, const Color& color);
    void add_uniform(uint32_t binding, const Color& color1, const Color& color2);

    void set_texture(uint32_t binding, Texture& texture);

    void set_blend(BlendFunc func);

    void update();
    void draw(View& view);
    void draw(View& view, const ViewportCoords& pos);

private:
    VkDevice device() const;
    void create_pipeline();
    void create_buffers();
    void create_descriptor_set_layout();
    void create_descriptor_sets();
    void destroy_pipeline();

    VkDeviceSize align_uniform(VkDeviceSize offset);
    auto make_binding_desc() -> VkVertexInputBindingDescription;
    uint32_t get_vertex_float_count();
    uint32_t get_attr_desc_count();
    static constexpr size_t max_attr_descs = 4;
    auto make_attr_descs() -> std::array<VkVertexInputAttributeDescription, max_attr_descs>;
    auto make_color_blend() -> VkPipelineColorBlendAttachmentState;

private:
    VertexFormat m_format;
    int m_closed_vertices = 0;
    int m_open_vertices = -1;
    std::vector<float> m_vertex_data;
    std::vector<uint16_t> m_index_data;
    static constexpr VkDeviceSize m_mvp_size = sizeof(float) * 16;
    std::vector<std::byte> m_uniform_data;
    struct Uniform {
        uint32_t binding;
        VkDeviceSize offset;
        VkDeviceSize range;
    };
    std::vector<Uniform> m_uniforms;
    VkDeviceSize m_min_uniform_offset_alignment = 0;
    struct TextureBinding {
        uint32_t binding = 0;
        Texture* ptr = nullptr;
    };
    TextureBinding m_texture;
    BlendFunc m_blend = BlendFunc::Off;

    Renderer& m_renderer;
    Shader* m_shader = nullptr;
    VkDescriptorSetLayout m_descriptor_set_layout {};
    VkDescriptorPool m_descriptor_pool {};
    VkDescriptorSet m_descriptor_sets[Window::cmd_buf_count] {};
    VkPipelineLayout m_pipeline_layout {};
    VkPipeline m_pipeline {};
    VkBuffer m_vertex_buffer {};
    VkBuffer m_index_buffer {};
    VkBuffer m_uniform_buffers[Window::cmd_buf_count] {};
    VkDeviceSize m_uniform_offsets[Window::cmd_buf_count] {};
    DeviceMemory m_device_memory;
};


} // namespace xci::graphics

#endif // include guard
