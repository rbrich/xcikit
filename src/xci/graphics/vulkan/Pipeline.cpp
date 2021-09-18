// Pipeline.cpp created on 2021-08-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Pipeline.h"
#include "VulkanError.h"
#include <xci/graphics/Renderer.h>
#include <xci/compat/macros.h>

#include <cassert>
#include <bit>

namespace xci::graphics {


void PipelineLayoutCreateInfo::add_uniform_binding(uint32_t binding)
{
    assert(m_uniform_binding_count < m_uniform_bindings.size());
    m_uniform_bindings[m_uniform_binding_count] = binding;
    ++m_uniform_binding_count;
}


void PipelineLayoutCreateInfo::add_texture_binding(uint32_t binding)
{
    assert(m_texture_binding == uint32_t(-1));
    m_texture_binding = binding;
}


std::vector<VkDescriptorSetLayoutBinding> PipelineLayoutCreateInfo::vk_layout_bindings() const
{
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;

    // mvp
    layout_bindings.push_back({
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    });

    // uniforms
    for (unsigned i = 0; i != m_uniform_binding_count; ++i) {
        layout_bindings.push_back({
                .binding = m_uniform_bindings[i],
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags =
                        VK_SHADER_STAGE_VERTEX_BIT |
                        VK_SHADER_STAGE_FRAGMENT_BIT,
        });
    }

    // texture
    if (m_texture_binding != uint32_t(-1)) {
        layout_bindings.push_back({
                .binding = m_texture_binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });
    }

    return layout_bindings;
}


DescriptorPoolSizes PipelineLayoutCreateInfo::descriptor_pool_sizes() const
{
    DescriptorPoolSizes sizes;

    // mvp
    sizes.add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

    // uniforms
    sizes.add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_uniform_binding_count);

    // texture
    if (m_texture_binding != uint32_t(-1))
        sizes.add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

    return sizes;
}


size_t PipelineLayoutCreateInfo::hash() const
{
    size_t h = 0;
    for (unsigned i = 0; i != m_uniform_binding_count; ++i) {
        h = std::rotl(h, 1) ^ m_uniform_bindings[i];
    }
    if (m_texture_binding != uint32_t(-1))
        h = std::rotl(h, 1) ^ m_texture_binding;
    return h;
}


bool PipelineLayoutCreateInfo::operator==(const PipelineLayoutCreateInfo& rhs) const
{
    return std::tie(m_uniform_binding_count, m_texture_binding) ==
           std::tie(rhs.m_uniform_binding_count, rhs.m_texture_binding) &&
           std::memcmp(m_uniform_bindings.data(), rhs.m_uniform_bindings.data(),
                   m_uniform_binding_count * sizeof(decltype(m_uniform_bindings)::value_type)) == 0;
}


PipelineLayout::PipelineLayout(Renderer& renderer, const PipelineLayoutCreateInfo& ci)
        : m_renderer(renderer)
{
    auto layout_bindings = ci.vk_layout_bindings();

    VkDescriptorSetLayoutCreateInfo layout_ci = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = (uint32_t) layout_bindings.size(),
            .pBindings = layout_bindings.data(),
    };
    VK_TRY("vkCreateDescriptorSetLayout",
            vkCreateDescriptorSetLayout(
                    m_renderer.vk_device(), &layout_ci,
                    nullptr, &m_descriptor_set_layout));

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptor_set_layout,
            .pushConstantRangeCount = 0,
    };

    VK_TRY("vkCreatePipelineLayout",
            vkCreatePipelineLayout(
                    m_renderer.vk_device(), &pipeline_layout_ci, nullptr,
                    &m_pipeline_layout));
}


PipelineLayout::~PipelineLayout()
{
    vkDestroyDescriptorSetLayout(m_renderer.vk_device(), m_descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(m_renderer.vk_device(), m_pipeline_layout, nullptr);
}


PipelineCreateInfo::PipelineCreateInfo(
        Shader& shader, VkPipelineLayout layout, VkRenderPass render_pass)
{
    m_shader_stages[0] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader.vk_vertex_module(),
            .pName = "main",
    };

    m_shader_stages[1] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shader.vk_fragment_module(),
            .pName = "main",
    };

    m_vertex_input_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &m_binding_desc,
            .vertexAttributeDescriptionCount = 0,  // see set_vertex_format
            .pVertexAttributeDescriptions = m_attr_descs.data(),
    };

    m_input_assembly_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    m_viewport_state_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = nullptr,  // dynamic state
            .scissorCount = 1,
            .pScissors = nullptr,  // dynamic state
    };

    m_rasterization_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f,
    };

    m_multisample_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
    };

    m_color_blend_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &m_color_blend,
    };

    m_dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
    };

    m_dynamic_state_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = (uint32_t) m_dynamic_states.size(),
            .pDynamicStates = m_dynamic_states.data(),
    };


    m_pipeline_ci = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = (uint32_t) m_shader_stages.size(),
            .pStages = m_shader_stages.data(),
            .pVertexInputState = &m_vertex_input_ci,
            .pInputAssemblyState = &m_input_assembly_ci,
            .pViewportState = &m_viewport_state_ci,
            .pRasterizationState = &m_rasterization_ci,
            .pMultisampleState = &m_multisample_ci,
            .pColorBlendState = &m_color_blend_ci,
            .pDynamicState = &m_dynamic_state_ci,
            .layout = layout,
            .renderPass = render_pass,
            .subpass = 0,
    };
}


void PipelineCreateInfo::set_vertex_format(VertexFormat format)
{
    m_format = format;
    constexpr uint32_t sf = sizeof(float);
    m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
    uint32_t stride;
    uint32_t attr_desc_count = 0;
    switch (format) {
        case VertexFormat::V2t2:
            stride = 4;
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sf};
            attr_desc_count = 2;
            break;
        case VertexFormat::V2t22:
            stride = 6;
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 4 * sf};
            attr_desc_count = 3;
            break;
        case VertexFormat::V2c4t2:
            stride = 8;
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 6 * sf};
            attr_desc_count = 3;
            break;
        case VertexFormat::V2c4t22:
            stride = 10;
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 6 * sf};
            m_attr_descs[3] = {3, 0, VK_FORMAT_R32G32_SFLOAT, 8 * sf};
            attr_desc_count = 4;
            break;
        default:
            UNREACHABLE;
    }
    assert(attr_desc_count != 0);
    assert(attr_desc_count <= m_attr_descs.size());
    m_binding_desc = {
            .binding = 0,
            .stride = stride * sf,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    m_vertex_input_ci.vertexAttributeDescriptionCount = attr_desc_count;
}


void PipelineCreateInfo::set_color_blend(BlendFunc blend_func)
{
    m_blend_func = blend_func;
    constexpr VkColorComponentFlags color_mask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

    switch (blend_func) {
        case BlendFunc::Off:
            m_color_blend = {
                    .blendEnable = VK_FALSE,
                    .colorWriteMask = color_mask,
            };
            break;
        case BlendFunc::AlphaBlend:
            m_color_blend = {
                    .blendEnable = VK_TRUE,
                    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .colorBlendOp = VK_BLEND_OP_ADD,
                    .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .alphaBlendOp = VK_BLEND_OP_ADD,
                    .colorWriteMask = color_mask,
            };
            break;
        case BlendFunc::InverseVideo:
            m_color_blend = {
                    .blendEnable = VK_TRUE,
                    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
                    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                    .colorBlendOp = VK_BLEND_OP_ADD,
                    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                    .alphaBlendOp = VK_BLEND_OP_ADD,
                    .colorWriteMask = color_mask,
            };
            break;
    }
}


size_t PipelineCreateInfo::hash() const
{
    size_t h = 0;
    h = std::rotl(h, 1) ^ size_t(m_shader_stages[0].module);
    h = std::rotl(h, 1) ^ size_t(m_shader_stages[1].module);
    h = std::rotl(h, 1) ^ size_t(m_pipeline_ci.layout);
    h = std::rotl(h, 1) ^ size_t(m_pipeline_ci.renderPass);
    h = std::rotl(h, 1) ^ (size_t(m_format) << 16 | size_t(m_blend_func));
    return 0;
}


bool PipelineCreateInfo::operator==(const PipelineCreateInfo& rhs) const
{
    return std::tie(m_shader_stages[0].module, m_shader_stages[1].module,
                    m_pipeline_ci.layout, m_pipeline_ci.renderPass,
                    m_format, m_blend_func) ==
           std::tie(rhs.m_shader_stages[0].module, rhs.m_shader_stages[1].module,
                   rhs.m_pipeline_ci.layout, rhs.m_pipeline_ci.renderPass,
                   rhs.m_format, rhs.m_blend_func);
}


Pipeline::Pipeline(Renderer& renderer, const PipelineCreateInfo& pipeline_ci)
        : m_renderer(renderer)
{
    VK_TRY("vkCreateGraphicsPipelines",
            vkCreateGraphicsPipelines(m_renderer.vk_device(), VK_NULL_HANDLE, 1,
                    &pipeline_ci.vk(), nullptr, &m_pipeline));
}


Pipeline::~Pipeline()
{
    vkDestroyPipeline(m_renderer.vk_device(), m_pipeline, nullptr);
}


} // namespace xci::graphics
