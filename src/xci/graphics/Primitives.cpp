// Primitives.cpp created on 2018-08-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Primitives.h"
#include "Renderer.h"
#include "Window.h"
#include "View.h"
#include "Texture.h"
#include "Shader.h"
#include "vulkan/VulkanError.h"
#include "vulkan/Attachments.h"
#include <xci/core/memory.h>

#include <range/v3/view/enumerate.hpp>

#include <cassert>
#include <cstring>

namespace xci::graphics {

using ranges::views::enumerate;
using core::align_to;


PrimitivesBuffers::~PrimitivesBuffers()
{
    m_device_memory.free();
    vkDestroyBuffer(device(), m_index_buffer, nullptr);
    vkDestroyBuffer(device(), m_vertex_buffer, nullptr);
}


void PrimitivesBuffers::create(const std::vector<float>& vertex_data,
                               const std::vector<uint16_t>& index_data)
{
    // vertex buffer
    const VkBufferCreateInfo vertex_buffer_ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(vertex_data[0]) * vertex_data.size(),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_TRY("vkCreateBuffer(vertex)",
            vkCreateBuffer(device(), &vertex_buffer_ci,
                    nullptr, &m_vertex_buffer));
    VkMemoryRequirements vertex_mem_req;
    vkGetBufferMemoryRequirements(device(), m_vertex_buffer, &vertex_mem_req);
    auto vertex_offset = m_device_memory.reserve(vertex_mem_req);

    // index buffer
    const VkBufferCreateInfo index_buffer_ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(index_data[0]) * index_data.size(),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_TRY("vkCreateBuffer(index)",
            vkCreateBuffer(device(), &index_buffer_ci,
                    nullptr, &m_index_buffer));
    VkMemoryRequirements index_mem_req;
    vkGetBufferMemoryRequirements(device(), m_index_buffer, &index_mem_req);
    auto index_offset = m_device_memory.reserve(index_mem_req);

    // allocate memory and copy data
    m_device_memory.allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    m_device_memory.bind_buffer(m_vertex_buffer, vertex_offset);
    void* mapped = m_device_memory.map(vertex_offset, vertex_buffer_ci.size);
    std::memcpy(mapped, vertex_data.data(), vertex_buffer_ci.size);
    m_device_memory.flush(vertex_offset);
    m_device_memory.unmap();

    m_device_memory.bind_buffer(m_index_buffer, index_offset);
    mapped = m_device_memory.map(index_offset, index_buffer_ci.size);
    std::memcpy(mapped, index_data.data(), index_buffer_ci.size);
    m_device_memory.flush(index_offset);
    m_device_memory.unmap();
}


void PrimitivesBuffers::bind(VkCommandBuffer cmd_buf)
{
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buf, m_index_buffer, 0, VK_INDEX_TYPE_UINT16);
}


VkDevice PrimitivesBuffers::device() const
{
    return m_renderer.vk_device();
}


// -----------------------------------------------------------------------------


UniformBuffers::UniformBuffers(Renderer& renderer)
    : m_renderer(renderer), m_device_memory(renderer) {}


UniformBuffers::~UniformBuffers()
{
    if (m_device_memory_mapped)
        m_device_memory.unmap();
    m_device_memory.free();
    vkDestroyBuffer(device(), m_buffer, nullptr);
    vkDestroyBuffer(device(), m_storage_buffer, nullptr);
}


void UniformBuffers::create(size_t static_size, size_t dynamic_size, size_t storage_size)
{
    // uniform buffers
    m_dynamic_base = align_to(static_size, size_t(m_renderer.min_uniform_offset_alignment()));
    m_dynamic_size = dynamic_size;
    const VkBufferCreateInfo uniform_buffer_ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = m_dynamic_base + m_dynamic_size,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_TRY("vkCreateBuffer(uniform)",
           vkCreateBuffer(device(), &uniform_buffer_ci,
                          nullptr, &m_buffer));
    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device(), m_buffer, &mem_req);
    const auto base = m_device_memory.reserve(mem_req);
    assert(base == 0);  // This is currently expected (the memory is not pooled)

    // storage buffer
    if (storage_size != 0) {
        m_storage_size = align_to(storage_size, size_t(m_renderer.non_coherent_atom_size()));
        const VkBufferCreateInfo storage_buffer_ci = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = m_storage_size,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VK_TRY("vkCreateBuffer(storage)",
               vkCreateBuffer(device(), &storage_buffer_ci, nullptr, &m_storage_buffer));
        VkMemoryRequirements storage_mem_req;
        vkGetBufferMemoryRequirements(device(), m_storage_buffer, &storage_mem_req);
        m_storage_offset = m_device_memory.reserve(storage_mem_req);
    }

    // allocate memory and copy data
    m_device_memory.allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    m_device_memory.bind_buffer(m_buffer, base);
    m_device_memory_mapped = m_device_memory.map(base);

    if (storage_size != 0) {
        m_device_memory.bind_buffer(m_storage_buffer, m_storage_offset);
    }
}


size_t UniformBuffers::allocate_dynamic_uniform(size_t size)
{
    const auto aligned_size = align_to(size, size_t(m_renderer.min_uniform_offset_alignment()));
    if (m_dynamic_free_offset + aligned_size > m_dynamic_size) {
        m_dynamic_used_size += m_dynamic_size - m_dynamic_free_offset;  // mark the rest of area as used
        m_dynamic_free_offset = 0;
    }
    if (m_dynamic_used_size + aligned_size > m_dynamic_size) {
        throw VulkanError(fmt::format("Dynamic uniform area overflow (used {} of {})",
                                      m_dynamic_used_size, m_dynamic_size));
    }
    m_dynamic_used_size += aligned_size;
    m_dynamic_free_offset += aligned_size;
    return m_dynamic_free_offset - aligned_size;
}


void UniformBuffers::free_dynamic_uniform_mark(size_t mark)
{
    // The new used area is between mark (original free offset) and the current free offset
    if (mark <= m_dynamic_free_offset) {
        m_dynamic_used_size = m_dynamic_free_offset - mark;
    } else {
        // buffer was cycled
        m_dynamic_used_size = m_dynamic_size - mark  // the rest until end
                              + m_dynamic_free_offset;    // start to the free offset
    }
}


void UniformBuffers::write_uniforms(size_t offset, size_t size, const void* data)
{
    std::memcpy((char*)(m_device_memory_mapped) + offset, data, size);
    m_pending_flush.push_back({offset, size});
}


void UniformBuffers::write_storage(size_t offset, size_t size, const void* data)
{
    const auto base = m_storage_offset + offset;
    std::memcpy((char*)(m_device_memory_mapped) + base, data, size);
    m_pending_flush.push_back({base, size});
}


void UniformBuffers::flush()
{
    if (!m_pending_flush.empty()) {
        m_device_memory.flush(m_pending_flush);
        m_pending_flush.clear();
    }
}


void* UniformBuffers::mapped_storage(size_t offset)
{
    return (char*)(m_device_memory_mapped) + m_storage_offset + offset;
}


VkDevice UniformBuffers::device() const
{
    return m_renderer.vk_device();
}


// -----------------------------------------------------------------------------


DescriptorSets::~DescriptorSets()
{
    if (m_vk_descriptor_set != VK_NULL_HANDLE)
        m_descriptor_pool.free(1, &m_vk_descriptor_set);
}


void DescriptorSets::create(VkDescriptorSetLayout layout)
{
    m_descriptor_pool.allocate(1, &layout, &m_vk_descriptor_set);
}


void DescriptorSets::update(
        const UniformBuffers& uniform_buffers,
        const std::vector<UniformBinding>& uniform_bindings,
        const std::vector<StorageBinding>& storage_bindings,
        const std::vector<TextureBinding>& texture_bindings)
{
    std::vector<VkDescriptorBufferInfo> buffer_info;
    std::vector<VkWriteDescriptorSet> write_descriptor_set;
    buffer_info.reserve(uniform_bindings.size() + storage_bindings.size());
    write_descriptor_set.reserve(uniform_bindings.size());

    // uniforms
    for (const auto& [binding, uniform] : uniform_bindings | enumerate) {
        if (!uniform)
            continue;
        buffer_info.push_back({
                .buffer = uniform_buffers.vk_uniform_buffer(),
                .offset = uniform.dynamic? uniform_buffers.dynamic_base() : uniform.offset,
                .range = uniform.range,
        });
        write_descriptor_set.push_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_vk_descriptor_set,
                .dstBinding = uint32_t(binding),
                .descriptorCount = 1,
                .descriptorType = uniform.dynamic
                                      ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                      : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &buffer_info.back(),
        });
    }

    // storage
    for (const auto& storage_binding : storage_bindings) {
        buffer_info.push_back({
                .buffer = uniform_buffers.vk_storage_buffer(),
                .offset = storage_binding.offset,
                .range = storage_binding.range,
        });
        write_descriptor_set.push_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_vk_descriptor_set,
                .dstBinding = uint32_t(storage_binding.binding),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &buffer_info.back(),
        });
    }

    // textures
    std::vector<VkDescriptorImageInfo> image_info;
    image_info.reserve(texture_bindings.size());
    for (const auto& texture_binding : texture_bindings) {
        image_info.push_back({
                .sampler = texture_binding.sampler->vk(),
                .imageView = texture_binding.texture->vk_image_view(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
        write_descriptor_set.push_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_vk_descriptor_set,
                .dstBinding = texture_binding.binding,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info.back(),
        });
    }

    vkUpdateDescriptorSets(m_renderer.vk_device(), (uint32_t) write_descriptor_set.size(),
            write_descriptor_set.data(), 0, nullptr);
}


void DescriptorSets::bind(VkCommandBuffer cmd_buf, VkPipelineLayout pipeline_layout,
                                 std::span<const uint32_t> dynamic_offsets)
{
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout, 0, 1,
            &m_vk_descriptor_set,
            dynamic_offsets.size(), dynamic_offsets.data());
}


// -----------------------------------------------------------------------------


UniformDataBuilder::~UniformDataBuilder()
{
    m_prim.set_uniform_data(m_binding,
                            m_data.data(), m_data.size() * sizeof(m_data.front()),
                            m_dynamic);
}


// -----------------------------------------------------------------------------


Primitives::Primitives(Renderer& renderer,
        VertexFormat format, PrimitiveType type)
        : m_renderer(renderer), m_format(format), m_primitive_type(type)
{}


Primitives::~Primitives()
{
    destroy_pipeline();
}


void Primitives::reserve(size_t vertices)
{
    m_vertex_data.reserve(vertices * get_vertex_format_stride(m_format));
    // heuristic for quads (won't match for other primitives)
    // each 4 vertices (a quad) require 6 indices (two triangles)
    m_index_data.reserve(vertices / 4 * 6);
}


void Primitives::begin_primitive()
{
    assert(m_open_vertices == -1);
    m_open_vertices = 0;
    destroy_pipeline();
}


void Primitives::end_primitive()
{
    assert(m_open_vertices >= 3);
    assert(m_primitive_type == PrimitiveType::TriFans);

    // fan triangles: 0 1 2, 0 2 3, 0 3 4, ...
    const auto base = m_closed_vertices;
    auto offset = 1;
    while (offset + 1 < m_open_vertices) {
        m_index_data.push_back(base);
        m_index_data.push_back(base + offset);
        m_index_data.push_back(base + ++offset);
    }

    m_closed_vertices += m_open_vertices;
    m_open_vertices = -1;
}


VertexDataBuilder Primitives::add_vertex(FramebufferCoords xy)
{
    assert(m_open_vertices != -1);
    m_open_vertices++;
    m_vertex_data.push_back(xy.x.value);
    m_vertex_data.push_back(xy.y.value);
#ifndef NDEBUG
    return VertexDataBuilder(m_vertex_data, get_vertex_format_stride(m_format) - 2);
#else
    return VertexDataBuilder(m_vertex_data);
#endif
}


VertexDataBuilder Primitives::add_vertex(const Vec3f& pos)
{
    m_vertex_data.push_back(pos.x);
    m_vertex_data.push_back(pos.y);
    m_vertex_data.push_back(pos.z);
#ifndef NDEBUG
    return VertexDataBuilder(m_vertex_data, get_vertex_format_stride(m_format) - 3);
#else
    return VertexDataBuilder(m_vertex_data);
#endif
}


void Primitives::add_triangle_face(const Vec3u& indices)
{
    m_index_data.push_back(indices[0]);
    m_index_data.push_back(indices[1]);
    m_index_data.push_back(indices[2]);
}


void Primitives::clear()
{
    destroy_pipeline();
    m_vertex_data.clear();
    m_index_data.clear();
    m_closed_vertices = 0;
    m_open_vertices = -1;
}


void Primitives::set_shader(Shader shader)
{
    m_shader = shader;
}


void Primitives::set_texture(uint32_t binding, Texture& texture, Sampler& sampler)
{
    const auto it = std::find_if(m_textures.begin(), m_textures.end(),
                 [binding](const TextureBinding& t) { return t.binding == binding; });
    if (it == m_textures.end()) {
        m_textures.push_back({binding, &texture, &sampler});
    } else {
        it->texture = &texture;
        it->sampler = &sampler;
    }
    destroy_pipeline();
}


void Primitives::set_texture(uint32_t binding, Texture& texture)
{
    set_texture(binding, texture, m_renderer.get_sampler());
}


void Primitives::clear_push_constants()
{
    m_push_constants.clear();
    destroy_pipeline();
}


void Primitives::reserve_push_constants(size_t size)
{
    m_push_constants.resize(size);
    destroy_pipeline();
}


void Primitives::set_push_constants_data(const void* data, size_t size)
{
    if (m_push_constants.size() != size) {
        // new push constants, updating pipeline layout
        destroy_pipeline();
        m_push_constants.resize(size);
    }
    std::memcpy(m_push_constants.data(), data, size);
}


void Primitives::clear_uniforms()
{
    m_uniform_data.clear();
    m_uniforms.clear();
    destroy_pipeline();
}


void Primitives::set_uniform_data(uint32_t binding, const void* data, size_t size, bool dynamic)
{
    if (binding >= m_uniforms.size())
        m_uniforms.resize(binding + 1);
    auto& uniform = m_uniforms[binding];
    if (uniform.range != 0) {
        // update existing uniform
        assert(uniform.range == size);  // cannot resize existing uniform
        assert(uniform.dynamic == dynamic);  // cannot change uniform type
        if (std::memcmp(&m_uniform_data[uniform.offset], data, size) != 0) {
            std::memcpy(&m_uniform_data[uniform.offset], data, size);
            if (uniform.dynamic) {
                m_dynamic_uniforms_updated = true;
            } else {
                m_uniforms_updated = true;
            }
        }
        return;
    }
    // add new uniform
    auto offset = align_uniform(m_uniform_data.size());
    m_uniform_data.resize(offset + size);
    std::memcpy(&m_uniform_data[offset], data, size);
    m_uniforms[binding] = {offset, size, dynamic};
    destroy_pipeline();
}


void Primitives::set_uniform(uint32_t binding, Color color)
{
    LinearColor buf {color};
    set_uniform_data(binding, &buf, sizeof(buf));
}


void Primitives::set_uniform(uint32_t binding, const Mat3f& mat)
{
    Mat4f m4(mat);
    set_uniform_data(binding, m4.data(), m4.c1.byte_size() * 3);
}


void Primitives::clear_storage()
{
    m_storage.clear();
    m_storage_data.clear();
    destroy_pipeline();
}


void Primitives::reserve_storage(uint32_t binding, size_t size)
{
    const auto offset = m_storage.empty() ? 0 : m_storage.back().offset + m_storage.back().range;
    m_storage.emplace_back(StorageBinding{.binding = binding, .offset = offset, .range = size});
    m_storage_data.resize(offset + size);
    destroy_pipeline();
}


void Primitives::set_storage_data(uint32_t binding, const void* data, size_t size)
{
    const auto it = std::find_if(m_storage.begin(), m_storage.end(),
                                 [binding](const StorageBinding& b) { return b.binding == binding; });
    if (it == m_storage.end()) {
        // add new
        reserve_storage(binding, size);
        std::memcpy(&m_storage_data[m_storage.back().offset], data, size);
    } else {
        // update existing
        assert(it->range == size);
        if (std::memcmp(&m_storage_data[it->offset], data, size) != 0) {
            std::memcpy(&m_storage_data[it->offset], data, size);
            m_storage_updated = true;
        }
    }
}


void Primitives::set_storage_read_cb(uint32_t binding, size_t size, StorageReadCb cb)
{
    const auto it = std::find_if(m_storage.begin(), m_storage.end(),
                                 [binding](const StorageBinding& b) { return b.binding == binding; });
    if (it == m_storage.end()) {
        reserve_storage(binding, size);
        m_storage.back().read_cb = std::move(cb);
        destroy_pipeline();
    } else {
        assert(it->range == size);
        it->read_cb = std::move(cb);
    }
}


void Primitives::update()
{
    if (empty())
        return;
    if (!m_pipeline_layout) {
        update_pipeline();
    }
    for (const auto& texture : m_textures) {
        texture.texture->update();
    }
    copy_updated_uniforms();
}


void Primitives::draw(CommandBuffer& cmd_buf, Attachments& attachments,
                      View& view, PrimitiveDrawFlags flags)
{
    if (empty())
        return;

    if (!m_pipeline_layout) {
        assert(!"Primitives: call update before draw!");
        return;
    }

    const auto vk_cmd = cmd_buf.vk();

    // bind pipeline
    assert(m_shader);
    PipelineCreateInfo pipeline_ci(m_shader.vk_vertex_module(), m_shader.vk_fragment_module(),
                                   m_pipeline_layout->vk(), attachments.render_pass());
    pipeline_ci.set_vertex_format(m_format);
    pipeline_ci.set_color_blend(m_blend);
    pipeline_ci.set_depth_test(m_depth_test);
    pipeline_ci.set_sample_count(attachments.msaa_samples());
    Pipeline& pipeline = m_renderer.get_pipeline(pipeline_ci);
    vkCmdBindPipeline(vk_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.vk());

    // set scissor region
    view.apply_crop(vk_cmd);

    m_buffers->bind(vk_cmd);
    cmd_buf.add_resource(m_buffers);

    // push constants
    if (!m_push_constants.empty())
        vkCmdPushConstants(vk_cmd, m_pipeline_layout->vk(),
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, uint32_t(m_push_constants.size()), m_push_constants.data());

    // uniforms
    if ((flags & PrimitiveDrawFlags::Projection2D) != PrimitiveDrawFlags::None)
        set_uniform(0, view.projection_matrix());
    if (m_dynamic_uniforms_updated) {
        // Free dynamic uniforms allocated for this frame at the end of render pass
        const auto mark = m_uniform_buffers->get_dynamic_uniform_mark();
        cmd_buf.add_cleanup(
                [buf = m_uniform_buffers, mark]{ buf->free_dynamic_uniform_mark(mark); });
        m_dynamic_uniforms_updated = false;
    }
    if (m_uniforms_updated) {
        const auto dynamic_size = m_uniform_buffers->dynamic_size();
        const auto storage_size = m_uniform_buffers->storage_size();
        m_uniform_buffers = std::make_shared<UniformBuffers>(m_renderer);
        m_uniform_buffers->create(m_uniform_data.size(), dynamic_size, storage_size);
        copy_all_uniforms();

        m_descriptor_sets = std::make_shared<DescriptorSets>(m_renderer, m_descriptor_pool.get());
        m_descriptor_sets->create(m_pipeline_layout->vk_descriptor_set_layout());
        m_descriptor_sets->update(*m_uniform_buffers, m_uniforms, m_storage, m_textures);
    }
    cmd_buf.add_resource(m_uniform_buffers);

    // storage buffers - read back
    for (const auto& storage : m_storage) {
        if (storage.read_cb) {
            cmd_buf.add_cleanup([this, &storage] {
                if (storage.read_cb)
                    storage.read_cb(m_uniform_buffers->mapped_storage(storage.offset), storage.range);
            });
        }
    }

    // bind descriptor sets
    std::vector<uint32_t> dynamic_offsets;
    for (auto& uniform : m_uniforms) {
        if (uniform && uniform.dynamic)
            dynamic_offsets.push_back(uniform.dynamic_offset);
    }
    m_descriptor_sets->bind(vk_cmd, m_pipeline_layout->vk(), dynamic_offsets);
    cmd_buf.add_resource(m_descriptor_sets);

    // draw
    vkCmdDrawIndexed(vk_cmd, static_cast<uint32_t>(m_index_data.size()), 1, 0, 0, 0);
}


void Primitives::draw(View& view, PrimitiveDrawFlags flags)
{
    auto* window = view.window();
    draw(window->command_buffer(), m_renderer.swapchain().attachments(),
         view, flags);
}


void Primitives::draw(View& view, VariCoords pos)
{
    auto pop_offset = view.push_offset(view.to_fb(pos));
    draw(view);
}


void Primitives::update_pipeline()
{
    if (m_uniforms.empty() || !m_uniforms[0]) {
        set_uniform(0, Mat4f{});  // MVP
    }

    PipelineLayoutCreateInfo pipeline_layout_ci;
    if (!m_push_constants.empty()) {
        pipeline_layout_ci.add_push_constant_range(0, uint32_t(m_push_constants.size()));
    }
    uint32_t dynamic_size = 0;
    for (const auto& [binding, uniform] : m_uniforms | enumerate) {
        if (uniform)
            pipeline_layout_ci.add_uniform_binding(binding, uniform.dynamic);
        if (uniform.dynamic) {
            dynamic_size += 10000 * align_uniform(uniform.range);
        }
    }
    for (const auto& texture : m_textures) {
        pipeline_layout_ci.add_texture_binding(texture.binding);
    }
    size_t storage_size = 0;
    for (const auto& storage : m_storage) {
        pipeline_layout_ci.add_storage_binding(storage.binding);
        storage_size = storage.offset + storage.range;
    }
    m_pipeline_layout = &m_renderer.get_pipeline_layout(pipeline_layout_ci);

    m_buffers = std::make_shared<PrimitivesBuffers>(m_renderer);
    m_buffers->create(m_vertex_data, m_index_data);

    m_uniform_buffers = std::make_shared<UniformBuffers>(m_renderer);
    m_uniform_buffers->create(m_uniform_data.size(), dynamic_size, storage_size);
    copy_all_uniforms();

    m_descriptor_pool = m_renderer.get_descriptor_pool(Window::cmd_buf_count, pipeline_layout_ci.descriptor_pool_sizes());

    m_descriptor_sets = std::make_shared<DescriptorSets>(m_renderer, m_descriptor_pool.get());
    m_descriptor_sets->create(m_pipeline_layout->vk_descriptor_set_layout());
    m_descriptor_sets->update(*m_uniform_buffers, m_uniforms, m_storage, m_textures);
}


void Primitives::destroy_pipeline()
{
    if (!m_pipeline_layout)
        return;
    m_buffers.reset();
    m_uniform_buffers.reset();
    m_descriptor_sets.reset();
    m_pipeline_layout = nullptr;
}


VkDeviceSize Primitives::align_uniform(VkDeviceSize offset)
{
    const auto min_alignment = m_renderer.min_uniform_offset_alignment();
    return align_to(offset, min_alignment);
}


void Primitives::copy_all_uniforms()
{
    m_uniform_buffers->write_uniforms(0, m_uniform_data.size(), m_uniform_data.data());
    m_uniforms_updated = false;
    for (auto& uniform : m_uniforms) {
        if (uniform && uniform.dynamic)
            m_uniform_buffers->write_dynamic_uniforms(uniform.dynamic_offset,
                                                      uniform.range, m_uniform_data.data() + uniform.offset);
    }
    m_dynamic_uniforms_updated = false;
    if (!m_storage_data.empty())
        m_uniform_buffers->write_storage(0, m_storage_data.size(), m_storage_data.data());
    m_storage_updated = false;
    m_uniform_buffers->flush();
}


void Primitives::copy_updated_uniforms()
{
    if (!m_uniform_buffers)
        return;
    if (m_uniforms_updated) {
        m_uniform_buffers->write_uniforms(0, m_uniform_data.size(), m_uniform_data.data());
        m_uniforms_updated = false;
    }
    if (m_dynamic_uniforms_updated) {
        for (auto& uniform : m_uniforms) {
            if (uniform && uniform.dynamic) {
                const auto size = uniform.range;
                uniform.dynamic_offset = m_uniform_buffers->allocate_dynamic_uniform(size);
                m_uniform_buffers->write_dynamic_uniforms(uniform.dynamic_offset, size, &m_uniform_data[uniform.offset]);
            }
        }
    }
    if (m_storage_updated) {
        m_uniform_buffers->write_storage(0, m_storage_data.size(), m_storage_data.data());
        m_storage_updated = false;
    }
    m_uniform_buffers->flush();
}


} // namespace xci::graphics
