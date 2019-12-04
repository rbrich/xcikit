// VulkanPrimitives.cpp created on 2019-10-24, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "VulkanPrimitives.h"
#include "VulkanRenderer.h"
#include "VulkanShader.h"
#include "VulkanWindow.h"

#include <xci/compat/macros.h>

#include <cassert>
#include <cstring>


namespace xci::graphics {


VulkanPrimitives::VulkanPrimitives(VulkanRenderer& renderer,
        VertexFormat format, PrimitiveType type)
        : m_format(format), m_renderer(renderer)
{
    assert(type == PrimitiveType::TriFans);
}


VulkanPrimitives::~VulkanPrimitives()
{
    destroy_pipeline();
}


void VulkanPrimitives::set_shader(Shader& shader)
{
    m_shader = dynamic_cast<VulkanShader*>(&shader);
}


void VulkanPrimitives::reserve(size_t primitives, size_t vertices)
{

}


void VulkanPrimitives::begin_primitive()
{
    assert(m_open_vertices == -1);
    m_open_vertices = 0;
    destroy_pipeline();
}


void VulkanPrimitives::end_primitive()
{
    assert(m_open_vertices >= 3);

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


void VulkanPrimitives::add_vertex(ViewportCoords xy, float u, float v)
{

}


void VulkanPrimitives::add_vertex(ViewportCoords xy, float u1, float v1, float u2, float v2)
{

}


void VulkanPrimitives::add_vertex(ViewportCoords xy, Color c, float u, float v)
{
    assert(m_format == VertexFormat::V2c4t2);
    assert(m_open_vertices >= 0);
    m_open_vertices++;
    m_vertex_data.push_back(xy.x.value);
    m_vertex_data.push_back(xy.y.value);
    m_vertex_data.push_back(c.red_f());
    m_vertex_data.push_back(c.green_f());
    m_vertex_data.push_back(c.blue_f());
    m_vertex_data.push_back(c.alpha_f());
    m_vertex_data.push_back(u);
    m_vertex_data.push_back(v);
}


void
VulkanPrimitives::add_vertex(ViewportCoords xy, Color c, float u1, float v1, float u2, float v2)
{

}


void VulkanPrimitives::clear()
{

}


bool VulkanPrimitives::empty() const
{
    return false;
}


void VulkanPrimitives::set_blend(Primitives::BlendFunc func)
{

}


void VulkanPrimitives::draw(View& view)
{
    if (m_pipeline == VK_NULL_HANDLE)
        create_pipeline();

    auto cmd_buf = dynamic_cast<VulkanWindow*>(view.window())->vk_command_buffer();

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) m_renderer.vk_image_extent().width,
        .height = (float) m_renderer.vk_image_extent().height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buf, m_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(m_index_data.size()), 1, 0, 0, 0);
}


void VulkanPrimitives::create_pipeline()
{
    assert(m_shader != nullptr);

    VkPipelineShaderStageCreateInfo vert_shader_stage_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = m_shader->vk_vertex_module(),
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo frag_shader_stage_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = m_shader->vk_fragment_module(),
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_ci,
        frag_shader_stage_ci
    };

    auto binding_desc = make_binding_desc();
    auto attr_descs = make_attr_descs();

    VkPipelineVertexInputStateCreateInfo vertex_input_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_desc,
        .vertexAttributeDescriptionCount = get_attr_desc_count(),
        .pVertexAttributeDescriptions = attr_descs.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = { INT32_MAX, INT32_MAX },
    };

    VkPipelineViewportStateCreateInfo viewport_state_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,  // dynamic state
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };

    if (vkCreatePipelineLayout(
            m_renderer.vk_device(), &pipeline_layout_ci, nullptr,
            &m_pipeline_layout) != VK_SUCCESS)
        throw std::runtime_error("vkCreatePipelineLayout failed");

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 1,
        .pDynamicStates = dynamic_states,
    };

    VkGraphicsPipelineCreateInfo pipeline_ci = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pViewportState = &viewport_state_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisample_ci,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = m_pipeline_layout,
        .renderPass = m_renderer.vk_render_pass(),
        .subpass = 0,
    };

    if (vkCreateGraphicsPipelines(m_renderer.vk_device(), VK_NULL_HANDLE, 1,
            &pipeline_ci, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("vkCreateGraphicsPipelines failed");

    create_buffers();
}


uint32_t VulkanPrimitives::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(m_renderer.vk_physical_device(), &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i))
        && (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


void VulkanPrimitives::create_buffers()
{
    VkBufferCreateInfo vertex_buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(m_vertex_data[0]) * m_vertex_data.size(),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(m_renderer.vk_device(), &vertex_buffer_ci,
            nullptr, &m_vertex_buffer) != VK_SUCCESS)
        throw std::runtime_error("vkCreateBuffer(vertex) failed");

    VkBufferCreateInfo index_buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(m_index_data[0]) * m_index_data.size(),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(m_renderer.vk_device(), &index_buffer_ci,
            nullptr, &m_index_buffer) != VK_SUCCESS)
        throw std::runtime_error("vkCreateBuffer(index) failed");

    VkMemoryRequirements vertex_mem_req;
    vkGetBufferMemoryRequirements(m_renderer.vk_device(), m_vertex_buffer, &vertex_mem_req);
    VkMemoryRequirements index_mem_req;
    vkGetBufferMemoryRequirements(m_renderer.vk_device(), m_index_buffer, &index_mem_req);

    VkDeviceSize size = vertex_mem_req.size;
    auto padding = index_mem_req.alignment - (size % index_mem_req.alignment);
    padding = (padding == index_mem_req.alignment) ? 0 : padding;
    size += padding + index_mem_req.size;

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = size,
        .memoryTypeIndex = find_memory_type(
                vertex_mem_req.memoryTypeBits & index_mem_req.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    if (vkAllocateMemory(m_renderer.vk_device(), &alloc_info,
            nullptr, &m_buffer_memory) != VK_SUCCESS)
        throw std::runtime_error("vkAllocateMemory failed (vertex/index buffer)");

    void* mapped;
    vkBindBufferMemory(m_renderer.vk_device(), m_vertex_buffer,
            m_buffer_memory, 0);
    vkMapMemory(m_renderer.vk_device(), m_buffer_memory,
            0, vertex_buffer_ci.size, 0, &mapped);
    std::memcpy(mapped, m_vertex_data.data(), (size_t) vertex_buffer_ci.size);
    vkUnmapMemory(m_renderer.vk_device(), m_buffer_memory);

    VkDeviceSize offset = vertex_mem_req.size + padding;
    vkBindBufferMemory(m_renderer.vk_device(), m_index_buffer,
            m_buffer_memory, offset);
    vkMapMemory(m_renderer.vk_device(), m_buffer_memory,
            offset, index_buffer_ci.size, 0, &mapped);
    std::memcpy(mapped, m_index_data.data(), (size_t) index_buffer_ci.size);
    vkUnmapMemory(m_renderer.vk_device(), m_buffer_memory);
}


void VulkanPrimitives::destroy_pipeline()
{
    if (m_pipeline == VK_NULL_HANDLE)
        return;
    vkFreeMemory(m_renderer.vk_device(), m_buffer_memory, nullptr);
    vkDestroyBuffer(m_renderer.vk_device(), m_index_buffer, nullptr);
    vkDestroyBuffer(m_renderer.vk_device(), m_vertex_buffer, nullptr);
    vkDestroyPipelineLayout(m_renderer.vk_device(), m_pipeline_layout, nullptr);
    vkDestroyPipeline(m_renderer.vk_device(), m_pipeline, nullptr);
    m_pipeline = VK_NULL_HANDLE;
}


auto VulkanPrimitives::make_binding_desc() -> VkVertexInputBindingDescription
{
    switch (m_format) {
        case VertexFormat::V2t2: return { .stride = sizeof(float) * 4 };
        case VertexFormat::V2t22: return { .stride = sizeof(float) * 6 };
        case VertexFormat::V2c4t2: return { .stride = sizeof(float) * 8 };
        case VertexFormat::V2c4t22: return { .stride = sizeof(float) * 10 };
    }
    UNREACHABLE;
}


uint32_t VulkanPrimitives::get_attr_desc_count()
{
    switch (m_format) {
        case VertexFormat::V2t2: return 2;
        case VertexFormat::V2t22: return 3;
        case VertexFormat::V2c4t2: return 3;
        case VertexFormat::V2c4t22: return 4;
    }
    UNREACHABLE;
}


auto VulkanPrimitives::make_attr_descs()
-> std::array<VkVertexInputAttributeDescription, max_attr_descs>
{
    std::array<VkVertexInputAttributeDescription, max_attr_descs> out;  // NOLINT
    out[0] = VkVertexInputAttributeDescription {
        .location = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = sizeof(float) * 0,
    };
    switch (m_format) {
        case VertexFormat::V2t22:
            out[2] = VkVertexInputAttributeDescription {
                .location = 2,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = sizeof(float) * 4,
            };
            FALLTHROUGH;
        case VertexFormat::V2t2:
            out[1] = VkVertexInputAttributeDescription {
                .location = 1,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = sizeof(float) * 2,
            };
            break;
        case VertexFormat::V2c4t22:
            out[3] = VkVertexInputAttributeDescription {
                .location = 3,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = sizeof(float) * 8,
            };
            FALLTHROUGH;
        case VertexFormat::V2c4t2:
            out[1] = VkVertexInputAttributeDescription {
                .location = 1,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = sizeof(float) * 2,
            };
            out[2] = VkVertexInputAttributeDescription {
                .location = 2,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = sizeof(float) * 6,
            };
            break;
    }
    return out;
}


} // namespace xci::graphics
