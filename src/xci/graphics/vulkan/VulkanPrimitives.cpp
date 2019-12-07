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
#include "VulkanError.h"

#include <xci/compat/macros.h>

#include <cassert>


namespace xci::graphics {


VulkanPrimitives::VulkanPrimitives(VulkanRenderer& renderer,
        VertexFormat format, PrimitiveType type)
        : m_format(format), m_renderer(renderer),
          m_device_memory(renderer)
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


void VulkanPrimitives::set_uniform(const char* name, float f)
{

}


void VulkanPrimitives::set_uniform(const char* name, float f1, float f2, float f3, float f4)
{

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

    auto* window = dynamic_cast<VulkanWindow*>(view.window());
    auto cmd_buf = window->vk_command_buffer();

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

    // projection matrix
    {
        auto mvp = view.projection_matrix(false);
        assert(mvp.size() * sizeof(mvp[0]) == m_mvp_size);
        auto i = window->vk_command_buffer_index();
        m_device_memory.copy_data(m_uniform_offsets[i],
                m_mvp_size, mvp.data());
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipeline_layout,0, 1,
                &m_descriptor_sets[i], 0, nullptr);
    }

    vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(m_index_data.size()), 1, 0, 0, 0);
}


VkDevice VulkanPrimitives::device() const
{
    return m_renderer.vk_device();
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

    create_descriptor_set_layout();

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptor_set_layout,
        .pushConstantRangeCount = 0,
    };

    VK_TRY("vkCreatePipelineLayout",
            vkCreatePipelineLayout(
                    device(), &pipeline_layout_ci, nullptr,
                    &m_pipeline_layout));

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

    VK_TRY("vkCreateGraphicsPipelines",
            vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1,
                    &pipeline_ci, nullptr, &m_pipeline));

    create_buffers();
    create_descriptor_sets();
}


void VulkanPrimitives::create_buffers()
{
    // vertex buffer
    VkBufferCreateInfo vertex_buffer_ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(m_vertex_data[0]) * m_vertex_data.size(),
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
    VkBufferCreateInfo index_buffer_ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(m_index_data[0]) * m_index_data.size(),
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
    for (size_t i = 0; i < VulkanWindow::cmd_buf_count; i++) {
        VkBufferCreateInfo uniform_buffer_ci = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = m_mvp_size,
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
            m_vertex_data.data());
    m_device_memory.bind_buffer(m_index_buffer, index_offset);
    m_device_memory.copy_data(index_offset, index_buffer_ci.size,
            m_index_data.data());
    for (size_t i = 0; i < VulkanWindow::cmd_buf_count; i++) {
        m_device_memory.bind_buffer(m_uniform_buffers[i], m_uniform_offsets[i]);
    }
}


void VulkanPrimitives::create_descriptor_set_layout()
{
    // descriptor set layout
    VkDescriptorSetLayoutBinding mvp_layout_binding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };
    VkDescriptorSetLayoutCreateInfo layout_ci = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &mvp_layout_binding,
    };
    VK_TRY("vkCreateDescriptorSetLayout",
            vkCreateDescriptorSetLayout(
                    device(), &layout_ci,
                    nullptr, &m_descriptor_set_layout));
}


void VulkanPrimitives::create_descriptor_sets()
{
    // descriptor pool
    VkDescriptorPoolSize pool_size = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = VulkanWindow::cmd_buf_count,
    };
    VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = VulkanWindow::cmd_buf_count,
            .poolSizeCount = 1,
            .pPoolSizes = &pool_size,
    };
    VK_TRY("vkCreateDescriptorPool",
            vkCreateDescriptorPool(device(), &pool_info, nullptr,
                    &m_descriptor_pool));

    // create descriptor sets
    std::array<VkDescriptorSetLayout, VulkanWindow::cmd_buf_count> layouts;  // NOLINT
    for (auto& item : layouts)
        item = m_descriptor_set_layout;

    VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptor_pool,
            .descriptorSetCount = VulkanWindow::cmd_buf_count,
            .pSetLayouts = layouts.data(),
    };

    VK_TRY("vkAllocateDescriptorSets",
            vkAllocateDescriptorSets(device(), &alloc_info,
                    m_descriptor_sets));

    for (size_t i = 0; i < VulkanWindow::cmd_buf_count; i++) {
        VkDescriptorBufferInfo buffer_info = {
                .buffer = m_uniform_buffers[i],
                .offset = 0,
                .range = VK_WHOLE_SIZE,
        };

        VkWriteDescriptorSet write_descriptor_set = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &buffer_info,
        };

        vkUpdateDescriptorSets(device(), 1,
                &write_descriptor_set, 0, nullptr);
    }
}


void VulkanPrimitives::destroy_pipeline()
{
    if (m_pipeline == VK_NULL_HANDLE)
        return;
    m_device_memory.free();
    for (auto buffer : m_uniform_buffers)
        vkDestroyBuffer(device(), buffer, nullptr);
    vkDestroyBuffer(device(), m_index_buffer, nullptr);
    vkDestroyBuffer(device(), m_vertex_buffer, nullptr);
    vkDestroyDescriptorPool(device(), m_descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device(), m_descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(device(), m_pipeline_layout, nullptr);
    vkDestroyPipeline(device(), m_pipeline, nullptr);
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
