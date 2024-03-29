// Primitives.h created on 2018-04-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_PRIMITIVES_H
#define XCI_GRAPHICS_PRIMITIVES_H

#include "View.h"
#include "Window.h"
#include "Color.h"
#include "Shader.h"
#include "vulkan/Sampler.h"
#include "vulkan/DeviceMemory.h"
#include "vulkan/Buffer.h"
#include "vulkan/Pipeline.h"
#include "vulkan/DescriptorPool.h"
#include <xci/math/Vec2.h>
#include <xci/core/mixin.h>

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <span>

namespace xci::graphics {

class Shader;
class Texture;
class Renderer;
class Primitives;
class Attachments;


enum class PrimitiveType : uint8_t {
    TriList,        // triangles list
    TriFans,        // also usable as quads
};


struct PrimitiveDrawFlags {
    bool projection_2d : 1 = false;  // set uniform binding 0 to View::projection_matrix
};


struct UniformBinding {
    VkDeviceSize offset = 0;
    VkDeviceSize range = 0;
    bool dynamic = false;
    uint32_t dynamic_offset = 0;
    explicit operator bool() const { return range != 0; }
};


struct TextureBinding {
    uint32_t binding = 0;
    VkImageView image_view = nullptr;
    VkSampler sampler = nullptr;
};


using StorageReadCb = std::function<void(const void* data, size_t size)>;

struct StorageBinding {
    uint32_t binding = 0;
    VkDeviceSize offset = 0;
    VkDeviceSize range = 0;
    StorageReadCb read_cb;
};


class PrimitivesBuffers {
public:
    explicit PrimitivesBuffers(Renderer& renderer)
        : m_renderer(renderer), m_device_memory(renderer) {}
    ~PrimitivesBuffers();

    void create(const std::vector<float>& vertex_data,
                const std::vector<uint16_t>& index_data);

    void bind(VkCommandBuffer cmd_buf);

private:
    VkDevice device() const;

    Renderer& m_renderer;
    Buffer m_vertex_buffer;
    Buffer m_index_buffer;
    DeviceMemory m_device_memory;
};


class UniformBuffers {
public:
    explicit UniformBuffers(Renderer& renderer);
    ~UniformBuffers();

    void create(size_t static_size, size_t dynamic_size, size_t storage_size);
    size_t dynamic_base() const { return m_dynamic_base; }
    size_t dynamic_size() const { return m_dynamic_size; }
    size_t storage_size() const { return m_storage_size; }

    size_t allocate_dynamic_uniform(size_t size);

    // Basic sanity check for the circular buffer.
    // After updating dynamic uniforms, remember mark.
    // When finished rendering the frame, call free_dynamic_uniform_mark for the mark.
    size_t get_dynamic_uniform_mark() const { return m_dynamic_free_offset; }
    void free_dynamic_uniform_mark(size_t mark);

    /// Write uniform data to device memory
    void write_uniforms(size_t offset, size_t size, const void* data);
    void write_dynamic_uniforms(size_t offset, size_t size, const void* data)
        { write_uniforms(m_dynamic_base + offset, size, data); }
    void write_storage(size_t offset, size_t size, const void* data);

    /// Flush pending device memory changes (via copy_*)
    void flush();

    /// Get address of mapped storage memory
    void* mapped_storage(size_t offset);

    VkBuffer vk_uniform_buffer() const { return m_buffer.vk(); }
    VkBuffer vk_storage_buffer() const { return m_storage_buffer.vk(); }

private:
    VkDevice device() const;

    Renderer& m_renderer;
    Buffer m_buffer;
    VkDeviceSize m_dynamic_base {};    // base offset for dynamic uniforms
    size_t m_dynamic_size {};          // size of dynamic uniforms allocation area (circular buffer)
    size_t m_dynamic_free_offset {};   // offset inside dynamic area pointing to next free block
    size_t m_dynamic_used_size {};     // size of used part of dynamic area
    Buffer m_storage_buffer;
    VkDeviceSize m_storage_offset {};
    size_t m_storage_size {};
    DeviceMemory m_device_memory;
    void* m_device_memory_mapped = nullptr;
    std::vector<DeviceMemory::MappedMemoryRange> m_pending_flush;
};


class DescriptorSets {
public:
    explicit DescriptorSets(Renderer& renderer, DescriptorPool& descriptor_pool)
        : m_renderer(renderer), m_descriptor_pool(descriptor_pool) {}
    ~DescriptorSets();

    void create(VkDescriptorSetLayout layout);

    void update(
            const UniformBuffers& uniform_buffers,
            const std::vector<UniformBinding>& uniform_bindings,
            const std::vector<StorageBinding>& storage_bindings,
            const std::vector<TextureBinding>& texture_bindings);

    void bind(VkCommandBuffer cmd_buf, VkPipelineLayout pipeline_layout,
              std::span<const uint32_t> dynamic_offsets);

private:
    Renderer& m_renderer;
    DescriptorPool& m_descriptor_pool;
    VkDescriptorSet m_vk_descriptor_set {};
};


using PrimitivesBuffersPtr = std::shared_ptr<PrimitivesBuffers>;
using UniformBuffersPtr = std::shared_ptr<UniformBuffers>;
using DescriptorSetsPtr = std::shared_ptr<DescriptorSets>;

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

    VertexDataBuilder& normal(const Vec3f& n) { add(n.x); add(n.y); add(n.z); return *this; }
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
        assert(m_expected-- > 0);
#endif
    }
    VertexData& m_vertex_data;
#ifndef NDEBUG
    size_t m_expected;  // expected data (number of floats)
#endif
};


class UniformDataBuilder {
public:
    explicit UniformDataBuilder(Primitives& prim, uint32_t binding, bool dynamic)
            : m_prim(prim), m_binding(binding), m_dynamic(dynamic) {}
    ~UniformDataBuilder();

    UniformDataBuilder& f(float f) { add(f); return *this; }
    UniformDataBuilder& vec2(const Vec2f& v) { add(v.x); add(v.y); return *this; }
    UniformDataBuilder& vec3(const Vec3f& v) { add(v.x); add(v.y); add(v.z); return *this; }
    UniformDataBuilder& vec4(const Vec4f& v) { add(v.x); add(v.y); add(v.z); add(v.w); return *this; }
    UniformDataBuilder& mat4(const Mat4f& m) { m_data.insert(m_data.end(), m.data(), m.data() + m.size()); return *this; }

    UniformDataBuilder& color(Color color) {
        add(color.red_linear_f());
        add(color.green_linear_f());
        add(color.blue_linear_f());
        add(color.alpha_f());
        return *this;
    }

private:
    void add(float d) { m_data.push_back(d); }

    Primitives& m_prim;
    std::vector<float> m_data;
    uint32_t m_binding;
    bool m_dynamic;
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
    ///     add_vertex({0.0f, 0.0f}).color(Color::Black()).uv(1.0f, 2.0f);
    VertexDataBuilder add_vertex(FramebufferCoords xy);

    /// Add 3D vertex
    /// Do not use begin_primitive/end_primitive. Instead, use explicit add_triangle_face()
    /// Example:
    ///     add_vertex({0.0f, 0.0f, 0.0f}).uv({0.0f, 1.0f}).normal({0.0f, 1.0f, 0.0f});
    VertexDataBuilder add_vertex(const Vec3f& pos);
    void add_triangle_face(const Vec3u& indices);

    void set_vertex_data(VertexData vertex_data) { m_vertex_data = std::move(vertex_data); destroy_pipeline(); }
    void set_index_data(IndexData index_data) { m_index_data = std::move(index_data); destroy_pipeline(); }

    void clear();
    bool empty() const { return m_vertex_data.empty(); }

    void set_shader(Shader shader);

    void clear_push_constants();
    void reserve_push_constants(size_t size);
    void set_push_constants_data(const void* data, size_t size);

    void clear_uniforms();
    void set_uniform_data(uint32_t binding, const void* data, size_t size, bool dynamic = false);

    // Uniform type helpers. See uniform block builder below for creating uniform blocks.
    void set_uniform(uint32_t binding, float f) { set_uniform_data(binding, &f, sizeof(f)); }
    void set_uniform(uint32_t binding, Color color);
    void set_uniform(uint32_t binding, const Vec2f& vec) { set_uniform_data(binding, vec.data(), vec.byte_size()); }
    void set_uniform(uint32_t binding, const Vec3f& vec) { set_uniform_data(binding, vec.data(), vec.byte_size()); }
    void set_uniform(uint32_t binding, const Vec4f& vec) { set_uniform_data(binding, vec.data(), vec.byte_size()); }
    void set_uniform(uint32_t binding, const Mat3f& mat);
    void set_uniform(uint32_t binding, const Mat4f& mat) { set_uniform_data(binding, mat.data(), mat.byte_size()); }

    /// Generic uniform block builder
    /// Example:
    ///     set_uniform(1).color(Color::White()).mat4(projection)
    UniformDataBuilder set_uniform(uint32_t binding) { return UniformDataBuilder(*this, binding, false); }

    /// Create or update a dynamic uniform.
    /// This type of uniform can be updated multiple times for a single frame,
    /// i.e. you can draw the same Primitives with different dynamic uniforms.
    /// This won't work with non-dynamic uniforms, because the draw commands are queued,
    /// while the uniform updates are immediate. With dynamic uniforms,
    /// a dynamic offset is recorded within the command buffer.
    /// Note that dynamic uniforms consume much more memory (but usually still much less than vertex buffers).
    UniformDataBuilder set_dynamic_uniform(uint32_t binding) { return UniformDataBuilder(*this, binding, true); }

    void clear_storage();
    void reserve_storage(uint32_t binding, size_t size);
    void set_storage_data(uint32_t binding, const void* data, size_t size);

    /// Set callback to be called when storage can be read back (when finished rendering the frame)
    /// If reserve_storage was called, size must be same.
    /// If it wasn't, a new storage binding will be created.
    void set_storage_read_cb(uint32_t binding, size_t size, StorageReadCb cb);

    void set_texture(uint32_t binding, VkImageView image_view, VkSampler sampler);
    void set_texture(uint32_t binding, Texture& texture, Sampler& sampler);
    void set_texture(uint32_t binding, Texture& texture);  // use default sampler

    void set_blend(BlendFunc func) { m_blend = func; }
    void set_depth_test(DepthTest depth_test) { m_depth_test = depth_test; }

    void update();

    void draw(CommandBuffer& cmd_buf, const Attachments& attachments,
              View& view, PrimitiveDrawFlags flags);
    void draw(View& view, PrimitiveDrawFlags flags = { .projection_2d = true });
    void draw(View& view, VariCoords pos);

private:
    void update_pipeline();
    void destroy_pipeline();

    VkDeviceSize align_uniform(VkDeviceSize offset);
    void copy_all_uniforms();
    void copy_updated_uniforms();

private:
    Renderer& m_renderer;

    VertexFormat m_format;
    [[maybe_unused]] PrimitiveType m_primitive_type;
    int m_closed_vertices = 0;
    int m_open_vertices = -1;
    VertexData m_vertex_data;
    IndexData m_index_data;
    std::vector<std::byte> m_push_constants;
    std::vector<std::byte> m_uniform_data;
    std::vector<std::byte> m_storage_data;
    std::vector<UniformBinding> m_uniforms;  // index = binding
    std::vector<TextureBinding> m_textures;
    std::vector<StorageBinding> m_storage;
    BlendFunc m_blend = BlendFunc::Off;
    DepthTest m_depth_test = DepthTest::Off;
    Shader m_shader;

    PipelineLayout* m_pipeline_layout = nullptr;
    SharedDescriptorPool m_descriptor_pool;
    UniformBuffersPtr m_uniform_buffers;
    PrimitivesBuffersPtr m_buffers;
    DescriptorSetsPtr m_descriptor_sets;
    bool m_uniforms_updated = false;
    bool m_dynamic_uniforms_updated = false;
    bool m_storage_updated = false;
};


} // namespace xci::graphics

#endif // include guard
