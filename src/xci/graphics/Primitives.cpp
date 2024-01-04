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

#include <range/v3/view/enumerate.hpp>

#include <cassert>
#include <cstring>

namespace xci::graphics {

using ranges::views::enumerate;


PrimitivesBuffers::~PrimitivesBuffers()
{
    m_device_memory.free();
    vkDestroyBuffer(device(), m_index_buffer, nullptr);
    vkDestroyBuffer(device(), m_vertex_buffer, nullptr);
}


void PrimitivesBuffers::create(
        const std::vector<float>& vertex_data,
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
    m_device_memory.allocate();
    m_device_memory.bind_buffer(m_vertex_buffer, vertex_offset);
    m_device_memory.copy_data(vertex_offset, vertex_buffer_ci.size,
                              vertex_data.data());
    m_device_memory.bind_buffer(m_index_buffer, index_offset);
    m_device_memory.copy_data(index_offset, index_buffer_ci.size,
                              index_data.data());
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


UniformBuffers::~UniformBuffers()
{
    m_device_memory.free();
    vkDestroyBuffer(device(), m_buffer, nullptr);
}


void UniformBuffers::create(const std::vector<std::byte>& uniform_data)
{
    // uniform buffers
    const VkBufferCreateInfo uniform_buffer_ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = uniform_data.size(),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_TRY("vkCreateBuffer(uniform)",
           vkCreateBuffer(device(), &uniform_buffer_ci,
                          nullptr, &m_buffer));
    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device(), m_buffer, &mem_req);
    m_uniform_offsets = m_device_memory.reserve(mem_req);

    // allocate memory and copy data
    m_device_memory.allocate();
    m_device_memory.bind_buffer(m_buffer, m_uniform_offsets);
    if (!uniform_data.empty()) {
        m_device_memory.copy_data(m_uniform_offsets,
                                  uniform_data.size(), uniform_data.data());
    }
}


void UniformBuffers::copy_uniforms(size_t offset, size_t size, const void* data)
{
    m_device_memory.copy_data(m_uniform_offsets + offset, size, data);
}


VkDevice UniformBuffers::device() const
{
    return m_renderer.vk_device();
}


// -----------------------------------------------------------------------------


UniformDescriptorSets::~UniformDescriptorSets()
{
    if (m_descriptor_sets != VK_NULL_HANDLE)
        m_descriptor_pool.free(1, &m_descriptor_sets);
}


void UniformDescriptorSets::create(VkDescriptorSetLayout layout)
{
    m_descriptor_pool.allocate(1, &layout, &m_descriptor_sets);
}


void UniformDescriptorSets::update(
        const UniformBuffers& uniform_buffers,
        const std::vector<UniformBinding>& uniform_bindings,
        const std::vector<TextureBinding>& texture_bindings)
{
    std::vector<VkDescriptorBufferInfo> buffer_info;
    std::vector<VkWriteDescriptorSet> write_descriptor_set;
    buffer_info.reserve(uniform_bindings.size());
    write_descriptor_set.reserve(uniform_bindings.size());

    // uniforms
    for (const auto& [binding, uniform] : uniform_bindings | enumerate) {
        if (!uniform)
            continue;
        buffer_info.push_back({
                .buffer = uniform_buffers.vk_uniform_buffer(),
                .offset = uniform.offset,
                .range = uniform.range,
        });
        write_descriptor_set.push_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptor_sets,
                .dstBinding = uint32_t(binding),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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
                .dstSet = m_descriptor_sets,
                .dstBinding = texture_binding.binding,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info.back(),
        });
    }

    vkUpdateDescriptorSets(m_renderer.vk_device(), (uint32_t) write_descriptor_set.size(),
            write_descriptor_set.data(), 0, nullptr);
}


void UniformDescriptorSets::bind(VkCommandBuffer cmd_buf, VkPipelineLayout pipeline_layout)
{
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout, 0, 1,
            &m_descriptor_sets, 0, nullptr);
}


// -----------------------------------------------------------------------------


UniformDataBuilder::~UniformDataBuilder()
{
    m_prim.set_uniform_data(m_binding, m_data.data(), m_data.size() * sizeof(m_data.front()));
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
    destroy_pipeline();
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
        if (std::memcmp(&m_uniform_data[uniform.offset], data, size) != 0) {
            std::memcpy(&m_uniform_data[uniform.offset], data, size);
            m_uniforms_changed = true;
        }
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
    LinearColor buf {color};
    set_uniform_data(binding, &buf, sizeof(buf));
}


void Primitives::set_uniform(uint32_t binding, Color color1, Color color2)
{
    struct { LinearColor c1, c2; } buf { color1, color2 };
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
    Mat4f m4(mat);
    set_uniform_data(binding, m4.data(), m4.c1.byte_size() * 3);
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


void Primitives::set_depth_test(DepthTest depth_test)
{
    m_depth_test = depth_test;
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
        texture.texture->update();
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
    if ((flags & PrimitiveDrawFlags::Projection2D) != PrimitiveDrawFlags::None)
        set_uniform(0, view.projection_matrix());
    if (m_uniforms_changed) {
        m_uniform_buffers = std::make_shared<UniformBuffers>(m_renderer);
        m_uniform_buffers->create(m_uniform_data);
        m_uniforms_changed = false;

        m_descriptor_sets = std::make_shared<UniformDescriptorSets>(m_renderer, m_descriptor_pool.get());
        m_descriptor_sets->create(m_pipeline_layout->vk_descriptor_set_layout());
        m_descriptor_sets->update(*m_uniform_buffers, m_uniforms, m_textures);
    }
    window->add_command_buffer_resource(m_uniform_buffers);

    // descriptor sets
    m_descriptor_sets->bind(cmd_buf, m_pipeline_layout->vk());
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
    pipeline_ci.set_depth_test(m_depth_test);
    pipeline_ci.set_sample_count(m_renderer.sample_count());
    m_pipeline = &m_renderer.get_pipeline(pipeline_ci);

    m_buffers = std::make_shared<PrimitivesBuffers>(m_renderer);
    m_buffers->create(m_vertex_data, m_index_data);

    m_uniform_buffers = std::make_shared<UniformBuffers>(m_renderer);
    m_uniform_buffers->create(m_uniform_data);
    m_uniforms_changed = false;

    m_descriptor_pool = m_renderer.get_descriptor_pool(Window::cmd_buf_count, pipeline_layout_ci.descriptor_pool_sizes());

    m_descriptor_sets = std::make_shared<UniformDescriptorSets>(m_renderer, m_descriptor_pool.get());
    m_descriptor_sets->create(m_pipeline_layout->vk_descriptor_set_layout());
    m_descriptor_sets->update(*m_uniform_buffers, m_uniforms, m_textures);
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
