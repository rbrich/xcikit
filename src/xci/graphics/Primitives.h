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


enum class PrimitiveType : uint8_t {
    TriList,        // triangles list
    TriFans,        // also usable as quads
};


enum class PrimitiveDrawFlags : uint8_t {
    None          = 0x00,
    Projection2D  = 0x01,  // set uniform binding 0 to View::projection_matrix
    FlipViewportY = 0x02,  // OpenGL coords compatibility
};
inline PrimitiveDrawFlags operator|(PrimitiveDrawFlags a, PrimitiveDrawFlags b) { return PrimitiveDrawFlags(uint8_t(a) | uint8_t(b)); }
inline PrimitiveDrawFlags operator&(PrimitiveDrawFlags a, PrimitiveDrawFlags b) { return PrimitiveDrawFlags(uint8_t(a) & uint8_t(b)); }


struct UniformBinding {
    VkDeviceSize offset = 0;
    VkDeviceSize range = 0;
    explicit operator bool() const { return range != 0; }
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
            const std::vector<std::byte>& uniform_data);

    void bind(VkCommandBuffer cmd_buf);

    void copy_uniforms(size_t cmd_buf_idx, size_t offset, size_t size, const void* data);

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
            const std::vector<UniformBinding>& uniform_bindings,
            const std::vector<TextureBinding>& texture_bindings);

    void bind(VkCommandBuffer cmd_buf, size_t cmd_buf_idx,
              VkPipelineLayout pipeline_layout);

private:
    Renderer& m_renderer;
    DescriptorPool& m_descriptor_pool;
    VkDescriptorSet m_descriptor_sets[Window::cmd_buf_count] {};
};


using PrimitivesBuffersPtr = std::shared_ptr<PrimitivesBuffers>;
using PrimitivesDescriptorSetsPtr = std::shared_ptr<PrimitivesDescriptorSets>;

using VertexData = std::vector<float>;
using IndexData = std::vector<uint16_t>;

class VertexDataBuilder {
public:
#ifndef NDEBUG
    explicit VertexDataBuilder(VertexData& data, size_t expected) : m_vertex_data(data), m_expected(expected) {}
    ~VertexDataBuilder() { assert(m_expected == 0); }
#else
    explicit VertexDataBuilder(VertexData& data) : m_vertex_data(data) {}
#endif

    VertexDataBuilder& uv(float u, float v) { add(u); add(v); return *this; }
    VertexDataBuilder& uv(Vec2f uv) { add(uv.x); add(uv.y); return *this; }
    VertexDataBuilder& uvw(float u, float v, float w) { add(u); add(v); add(w); return *this; }
    VertexDataBuilder& color(Color color) {
        add(color.red_linear_f());
        add(color.green_linear_f());
        add(color.blue_linear_f());
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
    VertexData& m_vertex_data;
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
    VertexDataBuilder add_vertex(FramebufferCoords xy);

    void set_vertex_data(VertexData vertex_data) { m_vertex_data = std::move(vertex_data); destroy_pipeline(); }
    void set_index_data(IndexData index_data) { m_index_data = std::move(index_data); destroy_pipeline(); }

    void clear();
    bool empty() const { return m_vertex_data.empty(); }

    void set_shader(Shader shader);

    void clear_uniforms();
    void set_uniform_data(uint32_t binding, const void* data, size_t size);
    void set_uniform(uint32_t binding, float f) { set_uniform_data(binding, &f, sizeof(f)); }
    void set_uniform(uint32_t binding, Color color);
    void set_uniform(uint32_t binding, Color color1, Color color2);
    void set_uniform(uint32_t binding, const Vec2f& vec);
    void set_uniform(uint32_t binding, const Vec3f& vec);
    void set_uniform(uint32_t binding, const Vec4f& vec);
    void set_uniform(uint32_t binding, const Mat3f& mat);
    void set_uniform(uint32_t binding, const Mat4f& mat);

    void set_texture(uint32_t binding, Texture& texture);

    void set_blend(BlendFunc func);
    void set_depth_test(DepthTest depth_test);

    void update();

    void draw(View& view, PrimitiveDrawFlags flags = PrimitiveDrawFlags::Projection2D);
    void draw(View& view, VariCoords pos);

private:
    void update_pipeline();
    void destroy_pipeline();

    VkDeviceSize align_uniform(VkDeviceSize offset);

private:
    VertexFormat m_format;
    [[maybe_unused]] PrimitiveType m_primitive_type;
    int m_closed_vertices = 0;
    int m_open_vertices = -1;
    VertexData m_vertex_data;
    IndexData m_index_data;
    std::vector<std::byte> m_uniform_data;
    std::vector<UniformBinding> m_uniforms;  // index = binding
    std::vector<TextureBinding> m_textures;
    BlendFunc m_blend = BlendFunc::Off;
    DepthTest m_depth_test = DepthTest::Off;

    Renderer& m_renderer;
    Shader m_shader;

    PipelineLayout* m_pipeline_layout = nullptr;
    SharedDescriptorPool m_descriptor_pool;
    PrimitivesBuffersPtr m_buffers;
    PrimitivesDescriptorSetsPtr m_descriptor_sets;

    Pipeline* m_pipeline = nullptr;
};


} // namespace xci::graphics

#endif // include guard
