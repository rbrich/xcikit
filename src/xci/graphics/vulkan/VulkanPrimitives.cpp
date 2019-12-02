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

#include <cassert>


namespace xci::graphics {


VulkanPrimitives::VulkanPrimitives(
        VulkanRenderer& renderer, VertexFormat format, PrimitiveType type)
        : m_renderer(renderer)
{
    assert(type == PrimitiveType::TriStrips);
}


VulkanPrimitives::~VulkanPrimitives()
{
    vkDestroyPipeline(m_renderer.vk_device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_renderer.vk_device(), m_pipeline_layout, nullptr);
}


void VulkanPrimitives::set_shader(Shader& shader)
{
    m_shader = dynamic_cast<VulkanShader*>(&shader);
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

    VkPipelineVertexInputStateCreateInfo vertex_input_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = { INT32_MAX, INT32_MAX };

    VkPipelineViewportStateCreateInfo viewport_state_ci = {};
    viewport_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_ci.viewportCount = 1;
    viewport_state_ci.pViewports = nullptr;  // dynamic state
    viewport_state_ci.scissorCount = 1;
    viewport_state_ci.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_ci = {};
    rasterization_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_ci.depthClampEnable = VK_FALSE;
    rasterization_ci.rasterizerDiscardEnable = VK_FALSE;
    rasterization_ci.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_ci.lineWidth = 1.0f;
    rasterization_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;  // FIXME : OpenGL default is CCW
    rasterization_ci.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample_ci = {};
    multisample_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_ci.sampleShadingEnable = VK_FALSE;
    multisample_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_ci = {};
    color_blend_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_ci.logicOpEnable = VK_FALSE;
    color_blend_ci.logicOp = VK_LOGIC_OP_COPY;
    color_blend_ci.attachmentCount = 1;
    color_blend_ci.pAttachments = &color_blend_attachment;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 0;
    pipeline_layout_ci.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(
            m_renderer.vk_device(), &pipeline_layout_ci, nullptr,
            &m_pipeline_layout) != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout!");

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
        throw std::runtime_error("failed to create graphics pipeline!");
}


void VulkanPrimitives::reserve(size_t primitives, size_t vertices)
{

}


void VulkanPrimitives::begin_primitive()
{

}


void VulkanPrimitives::end_primitive()
{

}


void VulkanPrimitives::add_vertex(ViewportCoords xy, float u, float v)
{

}


void VulkanPrimitives::add_vertex(ViewportCoords xy, float u1, float v1, float u2, float v2)
{

}


void VulkanPrimitives::add_vertex(ViewportCoords xy, Color c, float u, float v)
{

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

    vkCmdDraw(cmd_buf, 3, 1, 0, 0);
}


} // namespace xci::graphics
