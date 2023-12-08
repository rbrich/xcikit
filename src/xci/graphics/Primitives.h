// Primitives.h created on 2018-04-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_PRIMITIVES_H
#define XCI_GRAPHICS_PRIMITIVES_H

#include "View.h"
#include "Window.h"
#include "Color.h"
#include "Shader.h"
#include "Texture.h"
#include "vulkan/DeviceMemory.h"
#include "vulkan/Pipeline.h"
#include "vulkan/DescriptorPool.h"
#include <xci/math/Vec2.h>
#include <xci/core/mixin.h>

#include <vulkan/vulkan.h>

#include <array>
#include <memory>

namespace xci::graphics {

class Shader;
class Renderer;


enum class PrimitiveType {
    TriFans,        // also usable as quads
};


struct UniformBinding {
    uint32_t binding;
    VkDeviceSize offset;
    VkDeviceSize range;
};


struct TextureBinding {
    uint32_t binding = 0;
    Texture* ptr = nullptr;
};


class PrimitivesBuffers: public Resource {
public:
    explicit PrimitivesBuffers(Renderer& renderer)
        : m_renderer(renderer), m_device_memory(renderer) {}
    ~PrimitivesBuffers();

    void create(
            const std::vector<float>& vertex_data,
            const std::vector<uint16_t>& index_data,
            VkDeviceSize uniform_base,
            const std::vector<std::byte>& uniform_data);

    void bind(VkCommandBuffer cmd_buf);

    void copy_mvp(size_t cmd_buf_idx, const Mat4f& mvp);

    VkBuffer vk_uniform_buffer(size_t cmd_buf_idx) const { return m_uniform_buffers[cmd_buf_idx]; }

private:
    VkDevice device() const;

    Renderer& m_renderer;
    VkBuffer m_vertex_buffer {};
    VkBuffer m_index_buffer {};
    VkBuffer m_uniform_buffers[Window::cmd_buf_count] {};
    VkDeviceSize m_uniform_offsets[Window::cmd_buf_count] {};
    DeviceMemory m_device_memory;
};


class PrimitivesDescriptorSets: public Resource {
public:
    explicit PrimitivesDescriptorSets(Renderer& renderer, DescriptorPool& descriptor_pool)
        : m_renderer(renderer), m_descriptor_pool(descriptor_pool) {}
    ~PrimitivesDescriptorSets();

    void create(VkDescriptorSetLayout layout);

    void update(
            const PrimitivesBuffers& buffers,
            VkDeviceSize uniform_base,
            const std::vector<UniformBinding>& uniform_bindings,
            const TextureBinding& texture_binding);

    void bind(VkCommandBuffer cmd_buf, size_t cmd_buf_idx,
              VkPipelineLayout pipeline_layout);

private:
    Renderer& m_renderer;
    DescriptorPool& m_descriptor_pool;
    VkDescriptorSet m_descriptor_sets[Window::cmd_buf_count] {};
};


using PrimitivesBuffersPtr = std::shared_ptr<PrimitivesBuffers>;
using PrimitivesDescriptorSetsPtr = std::shared_ptr<PrimitivesDescriptorSets>;


class VertexData {
public:
#ifndef NDEBUG
    explicit VertexData(std::vector<float>& data, size_t expected) : m_vertex_data(data), m_expected(expected) {}
    ~VertexData() { assert(m_expected == 0); }
#else
    explicit VertexData(std::vector<float>& data) : m_vertex_data(data) {}
#endif

    VertexData& uv(float u, float v) { add(u); add(v); return *this; }
    VertexData& uv(Vec2f uv) { add(uv.x); add(uv.y); return *this; }
    VertexData& uvw(float u, float v, float w) { add(u); add(v); add(w); return *this; }
    VertexData& color(Color color) {
        add(color.red_f());
        add(color.green_f());
        add(color.blue_f());
        add(color.alpha_f());
        return *this;
    }

private:
    void add(float d) {
        m_vertex_data.push_back(d);
#ifndef NDEBUG
        assert(--m_expected >= 0);
#endif
    }
    std::vector<float>& m_vertex_data;
#ifndef NDEBUG
    size_t m_expected;  // expected data (number of floats)
#endif
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

    /// Add vertex coords + data
    /// Example:
    ///     add_vertex({0.0f, 0.0f})(Color::Black())(1.0f, 2.0f);
    VertexData add_vertex(FramebufferCoords xy);

    void clear();
    bool empty() const { return m_vertex_data.empty(); }

    void set_shader(Shader& shader);

    void clear_uniforms();
    void add_uniform_data(uint32_t binding, const void* data, size_t size);
    void add_uniform(uint32_t binding, float f) { add_uniform_data(binding, &f, sizeof(f)); }
    void add_uniform(uint32_t binding, float f1, float f2);
    void add_uniform(uint32_t binding, Color color);
    void add_uniform(uint32_t binding, Color color1, Color color2);

    void set_texture(uint32_t binding, Texture& texture);

    void set_blend(BlendFunc func);

    void update();
    void draw(View& view);
    void draw(View& view, VariCoords pos);

private:
    void update_pipeline();
    void destroy_pipeline();

    VkDeviceSize align_uniform(VkDeviceSize offset);

private:
    VertexFormat m_format;
    int m_closed_vertices = 0;
    int m_open_vertices = -1;
    std::vector<float> m_vertex_data;
    std::vector<uint16_t> m_index_data;
    std::vector<std::byte> m_uniform_data;
    std::vector<UniformBinding> m_uniforms;
    TextureBinding m_texture;
    BlendFunc m_blend = BlendFunc::Off;

    Renderer& m_renderer;
    Shader* m_shader = nullptr;

    PipelineLayout* m_pipeline_layout = nullptr;
    SharedDescriptorPool m_descriptor_pool;
    PrimitivesBuffersPtr m_buffers;
    PrimitivesDescriptorSetsPtr m_descriptor_sets;

    Pipeline* m_pipeline = nullptr;
};


} // namespace xci::graphics

#endif // include guard
