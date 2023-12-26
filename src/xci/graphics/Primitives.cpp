// Primitives.cpp created on 2018-08-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Primitives.h"
#include "Renderer.h"
#include "Window.h"
#include "View.h"
#include "Texture.h"
#include "Shader.h"
#include "vulkan/VulkanError.h"

#include <range/v3/view/enumerate.hpp>

#include <cassert>
#include <cstring>

namespace xci::graphics {

using ranges::views::enumerate;


PrimitivesBuffers::~PrimitivesBuffers()
{
    m_device_memory.free();
    for (auto buffer : m_uniform_buffers)
        vkDestroyBuffer(device(), buffer, nullptr);
    vkDestroyBuffer(device(), m_index_buffer, nullptr);
    vkDestroyBuffer(device(), m_vertex_buffer, nullptr);
}


void PrimitivesBuffers::create(
        const std::vector<float>& vertex_data,
        const std::vector<uint16_t>& index_data,
        const std::vector<std::byte>& uniform_data)
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

    // uniform buffers
    for (size_t i = 0; i < Window::cmd_buf_count; i++) {
        const VkBufferCreateInfo uniform_buffer_ci = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = uniform_data.size(),
                .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VK_TRY("vkCreateBuffer(uniform)",
                vkCreateBuffer(device(), &uniform_buffer_ci,
                        nullptr, &m_uniform_buffers[i]));
        VkMemoryRequirements mem_req;
        vkGetBufferMemoryRequirements(device(), m_uniform_buffers[i], &mem_req);
        m_uniform_offsets[i] = m_device_memory.reserve(mem_req);
    }

    // allocate memory and copy data
    m_device_memory.allocate();
    m_device_memory.bind_buffer(m_vertex_buffer, vertex_offset);
    m_device_memory.copy_data(vertex_offset, vertex_buffer_ci.size,
            vertex_data.data());
    m_device_memory.bind_buffer(m_index_buffer, index_offset);
    m_device_memory.copy_data(index_offset, index_buffer_ci.size,
            index_data.data());
    for (size_t i = 0; i < Window::cmd_buf_count; i++) {
        m_device_memory.bind_buffer(m_uniform_buffers[i], m_uniform_offsets[i]);
        if (!uniform_data.empty()) {
            m_device_memory.copy_data(
                    m_uniform_offsets[i],
                    uniform_data.size(), uniform_data.data());
        }
    }
}


void PrimitivesBuffers::bind(VkCommandBuffer cmd_buf)
{
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buf, m_index_buffer, 0, VK_INDEX_TYPE_UINT16);
}


void PrimitivesBuffers::copy_uniforms(size_t cmd_buf_idx, size_t offset, size_t size, const void* data)
{
    m_device_memory.copy_data(m_uniform_offsets[cmd_buf_idx] + offset, size, data);
}


VkDevice PrimitivesBuffers::device() const
{
    return m_renderer.vk_device();
}


// -----------------------------------------------------------------------------


PrimitivesDescriptorSets::~PrimitivesDescriptorSets()
{
    if (m_descriptor_sets[0] != VK_NULL_HANDLE)
        m_descriptor_pool.free(Window::cmd_buf_count, m_descriptor_sets);
}


void PrimitivesDescriptorSets::create(VkDescriptorSetLayout layout)
{
    // create descriptor sets
    std::array<VkDescriptorSetLayout, Window::cmd_buf_count> layouts;  // NOLINT
    for (auto& item : layouts)
        item = layout;

    m_descriptor_pool.allocate(Window::cmd_buf_count, layouts.data(), m_descriptor_sets);
}


void PrimitivesDescriptorSets::update(
        const PrimitivesBuffers& buffers,
        const std::vector<UniformBinding>& uniform_bindings,
        const std::vector<TextureBinding>& texture_bindings)
{
    for (size_t i = 0; i < Window::cmd_buf_count; i++) {
        std::vector<VkDescriptorBufferInfo> buffer_info;
        std::vector<VkWriteDescriptorSet> write_descriptor_set;
        buffer_info.reserve(uniform_bindings.size());
        write_descriptor_set.reserve(uniform_bindings.size());

        // uniforms
        for (const auto& [binding, uniform] : uniform_bindings | enumerate) {
            if (!uniform)
                continue;
            buffer_info.push_back({
                    .buffer = buffers.vk_uniform_buffer(i),
                    .offset = uniform.offset,
                    .range = uniform.range,
            });
            write_descriptor_set.push_back(VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_descriptor_sets[i],
                    .dstBinding = uint32_t(binding),
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &buffer_info.back(),
            });
        }

        // textures
        VkDescriptorImageInfo image_info;  // keep alive for vkUpdateDescriptorSets()
        for (const auto& texture_binding : texture_bindings) {
            auto* texture = texture_binding.ptr;
            image_info = {
                    .sampler = texture->vk_sampler(),
                    .imageView = texture->vk_image_view(),
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            write_descriptor_set.push_back(VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_descriptor_sets[i],
                    .dstBinding = texture_binding.binding,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &image_info,
            });
        }

        vkUpdateDescriptorSets(m_renderer.vk_device(), (uint32_t) write_descriptor_set.size(),
                write_descriptor_set.data(), 0, nullptr);
    }
}


void PrimitivesDescriptorSets::bind(
        VkCommandBuffer cmd_buf, size_t cmd_buf_idx,
        VkPipelineLayout pipeline_layout)
{
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout, 0, 1,
            &m_descriptor_sets[cmd_buf_idx], 0, nullptr);
}


// -----------------------------------------------------------------------------


Primitives::Primitives(Renderer& renderer,
        VertexFormat format, PrimitiveType type)
        : m_format(format), m_primitive_type(type), m_renderer(renderer)
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
    destroy_pipeline();
}


void Primitives::set_texture(uint32_t binding, Texture& texture)
{
    const auto it = std::find_if(m_textures.begin(), m_textures.end(),
                 [binding](const TextureBinding& t) { return t.binding == binding; });
    if (it == m_textures.end()) {
        m_textures.push_back({binding, &texture});
    } else {
        it->ptr = &texture;
    }
    destroy_pipeline();
}


void Primitives::clear_uniforms()
{
    m_uniform_data.clear();
    m_uniforms.clear();
    destroy_pipeline();
}


void Primitives::set_uniform_data(uint32_t binding, const void* data, size_t size)
{
    if (binding >= m_uniforms.size())
        m_uniforms.resize(binding + 1);
    auto& uniform = m_uniforms[binding];
    if (uniform.range != 0) {
        // update existing uniform
        assert(uniform.range == size);  // cannot resize existing uniform
        std::memcpy(&m_uniform_data[uniform.offset], data, size);
        return;
    }
    // add new uniform
    auto offset = align_uniform(m_uniform_data.size());
    m_uniform_data.resize(offset + size);
    std::memcpy(&m_uniform_data[offset], data, size);
    m_uniforms[binding] = {offset, size};
    destroy_pipeline();
}


void Primitives::set_uniform(uint32_t binding, Color color)
{
    FloatColor buf {color};
    set_uniform_data(binding, &buf, sizeof(buf));
}


void Primitives::set_uniform(uint32_t binding, Color color1, Color color2)
{
    struct { FloatColor c1, c2; } buf { color1, color2 };
    set_uniform_data(binding, &buf, sizeof(buf));
}


void Primitives::set_uniform(uint32_t binding, const Vec2f& vec)
{
    set_uniform_data(binding, vec.data(), vec.byte_size());
}


void Primitives::set_uniform(uint32_t binding, const Vec3f& vec)
{
    set_uniform_data(binding, vec.data(), vec.byte_size());
}


void Primitives::set_uniform(uint32_t binding, const Vec4f& vec)
{
    set_uniform_data(binding, vec.data(), vec.byte_size());
}


void Primitives::set_uniform(uint32_t binding, const Mat3f& mat)
{
    set_uniform_data(binding, mat.data(), mat.byte_size());
}


void Primitives::set_uniform(uint32_t binding, const Mat4f& mat)
{
    set_uniform_data(binding, mat.data(), mat.byte_size());
}


void Primitives::set_blend(BlendFunc func)
{
    m_blend = func;
    destroy_pipeline();
}


void Primitives::update()
{
    if (empty())
        return;
    if (!m_pipeline) {
        update_pipeline();
    }
    for (const auto& texture : m_textures) {
        texture.ptr->update();
    }
}


void Primitives::draw(View& view, PrimitiveDrawFlags flags)
{
    if (empty())
        return;

    if (!m_pipeline) {
        assert(!"Primitives: call update before draw!");
        return;
    }

    auto* window = view.window();
    auto cmd_buf = window->vk_command_buffer();

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->vk());

    // set viewport
    VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) m_renderer.vk_image_extent().width,
            .height = (float) m_renderer.vk_image_extent().height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    if ((flags & PrimitiveDrawFlags::FlipViewportY) != PrimitiveDrawFlags::None) {
        viewport.y = viewport.height;
        viewport.height = -viewport.height;
    }
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    // set scissor region
    view.apply_crop();

    m_buffers->bind(cmd_buf);
    window->add_command_buffer_resource(m_buffers);

    // uniforms
    auto i = window->command_buffer_index();
    if ((flags & PrimitiveDrawFlags::Projection2D) != PrimitiveDrawFlags::None)
        set_uniform(0, view.projection_matrix());
    m_buffers->copy_uniforms(i, 0, m_uniform_data.size(), m_uniform_data.data());

    // descriptor sets
    m_descriptor_sets->bind(cmd_buf, i, m_pipeline_layout->vk());
    window->add_command_buffer_resource(m_descriptor_sets);

    vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(m_index_data.size()), 1, 0, 0, 0);
}


void Primitives::draw(View& view, VariCoords pos)
{
    auto pop_offset = view.push_offset(view.to_fb(pos));
    draw(view);
}


void Primitives::update_pipeline()
{
    assert(m_shader);
    if (m_uniforms.empty() || !m_uniforms[0]) {
        set_uniform(0, Mat4f{});  // MVP
    }

    PipelineLayoutCreateInfo pipeline_layout_ci;
    for (const auto& [binding, uniform] : m_uniforms | enumerate) {
        if (uniform)
            pipeline_layout_ci.add_uniform_binding(binding);
    }
    for (const auto& texture : m_textures) {
        pipeline_layout_ci.add_texture_binding(texture.binding);
    }
    m_pipeline_layout = &m_renderer.get_pipeline_layout(pipeline_layout_ci);
    PipelineCreateInfo pipeline_ci(m_shader.vk_vertex_module(), m_shader.vk_fragment_module(),
                                   m_pipeline_layout->vk(), m_renderer.vk_render_pass());
    pipeline_ci.set_vertex_format(m_format);
    pipeline_ci.set_color_blend(m_blend);
    m_pipeline = &m_renderer.get_pipeline(pipeline_ci);

    m_buffers = std::make_shared<PrimitivesBuffers>(m_renderer);
    m_buffers->create(m_vertex_data, m_index_data, m_uniform_data);

    m_descriptor_pool = m_renderer.get_descriptor_pool(Window::cmd_buf_count, pipeline_layout_ci.descriptor_pool_sizes());

    m_descriptor_sets = std::make_shared<PrimitivesDescriptorSets>(m_renderer, m_descriptor_pool.get());
    m_descriptor_sets->create(m_pipeline_layout->vk_descriptor_set_layout());
    m_descriptor_sets->update(*m_buffers, m_uniforms, m_textures);
}


void Primitives::destroy_pipeline()
{
    if (m_pipeline == nullptr)
        return;
    m_buffers.reset();
    m_descriptor_sets.reset();
    m_pipeline = nullptr;
}


VkDeviceSize Primitives::align_uniform(VkDeviceSize offset)
{
    const auto min_alignment = m_renderer.min_uniform_offset_alignment();
    auto unaligned = offset % min_alignment;
    if (unaligned > 0)
        offset += min_alignment - unaligned;
    return offset;
}


} // namespace xci::graphics
