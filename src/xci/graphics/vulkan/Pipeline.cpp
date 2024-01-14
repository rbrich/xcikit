// Pipeline.cpp created on 2021-08-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Pipeline.h"
#include "VulkanError.h"
#include <xci/graphics/Renderer.h>
#include <xci/compat/macros.h>

#include <cassert>
#include <bit>

namespace xci::graphics {


unsigned get_vertex_format_stride(VertexFormat format)
{
    switch (format) {
        case VertexFormat::V2: return 2;
        case VertexFormat::V2t2: return 4;
        case VertexFormat::V2t3: return 5;
        case VertexFormat::V2t22: return 6;
        case VertexFormat::V2t222: return 8;
        case VertexFormat::V2c4: return 6;
        case VertexFormat::V2c4t2: return 8;
        case VertexFormat::V2c4t22: return 10;
        case VertexFormat::V2c44t2: return 12;
        case VertexFormat::V2c44t3: return 13;
        case VertexFormat::V2c44t22: return 14;
        case VertexFormat::V2c44t222: return 16;
        case VertexFormat::V3n3: return 6;
        case VertexFormat::V3n3t2: return 8;
    }
    XCI_UNREACHABLE;
}


void PipelineLayoutCreateInfo::add_uniform_binding(uint32_t binding, bool dynamic)
{
    m_layout_bindings.push_back({binding,
        (dynamic ? LayoutBinding::TypeDynamicUniform : 0)
                 | LayoutBinding::StageVertex | LayoutBinding::StageFragment});
}


void PipelineLayoutCreateInfo::add_texture_binding(uint32_t binding)
{
    m_layout_bindings.push_back({binding,
        LayoutBinding::TypeImageSampler | LayoutBinding::StageFragment});
}


void PipelineLayoutCreateInfo::add_storage_binding(uint32_t binding)
{
    m_layout_bindings.push_back({binding,
        LayoutBinding::TypeStorageBuffer | LayoutBinding::StageFragment});
}


void PipelineLayoutCreateInfo::add_push_constant_range(uint32_t offset, uint32_t size)
{
    m_push_constant_ranges.emplace_back(offset, size);
}


std::vector<VkDescriptorSetLayoutBinding> PipelineLayoutCreateInfo::vk_layout_bindings() const
{
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    for (const auto& item : m_layout_bindings) {
        layout_bindings.push_back({
                .binding = item.binding,
                .descriptorType = item.vk_descriptor_type(),
                .descriptorCount = 1,
                .stageFlags =
                        ((item.flags & LayoutBinding::StageVertex)? VK_SHADER_STAGE_VERTEX_BIT : 0u) |
                        ((item.flags & LayoutBinding::StageFragment)? VK_SHADER_STAGE_FRAGMENT_BIT : 0u),
        });
    }
    return layout_bindings;
}


std::vector<VkPushConstantRange> PipelineLayoutCreateInfo::vk_push_constant_ranges() const
{
    std::vector<VkPushConstantRange> ranges;
    for (const auto& item : m_push_constant_ranges) {
        ranges.push_back({
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = item.offset,
                .size = item.size,
        });
    }
    return ranges;
}


DescriptorPoolSizes PipelineLayoutCreateInfo::descriptor_pool_sizes() const
{
    DescriptorPoolSizes sizes;

    // uniforms
    const auto uniform_count = std::count_if(m_layout_bindings.begin(), m_layout_bindings.end(),
             [](const auto& v) { return (v.flags & LayoutBinding::TypeMask) == LayoutBinding::TypeUniform; });
    if (uniform_count)
        sizes.add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniform_count);

    // dynamic uniforms
    const auto dyn_uniform_count = std::count_if(m_layout_bindings.begin(), m_layout_bindings.end(),
             [](const auto& v) { return (v.flags & LayoutBinding::TypeMask) == LayoutBinding::TypeDynamicUniform; });
    if (dyn_uniform_count)
        sizes.add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, dyn_uniform_count);

    // texture
    const auto texture_count = std::count_if(m_layout_bindings.begin(), m_layout_bindings.end(),
             [](const auto& v)
             { return (v.flags & LayoutBinding::TypeMask) == LayoutBinding::TypeImageSampler; });
    if (texture_count)
        sizes.add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texture_count);

    // storage buffer
    const auto storage_count = std::count_if(m_layout_bindings.begin(), m_layout_bindings.end(),
             [](const auto& v) { return (v.flags & LayoutBinding::TypeMask) == LayoutBinding::TypeStorageBuffer; });
    if (storage_count)
        sizes.add(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storage_count);

    return sizes;
}


size_t PipelineLayoutCreateInfo::hash() const
{
    size_t h = 0;
    for (const auto& item : m_layout_bindings) {
        h = std::rotl(h, 7) ^ ((item.binding << 4) | item.flags);
    }
    for (const auto& item : m_push_constant_ranges) {
        h = std::rotl(h, 7) ^ ((item.offset << 4) | item.size);
    }
    return h;
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

    auto push_constant_ranges = ci.vk_push_constant_ranges();

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptor_set_layout,
            .pushConstantRangeCount = (uint32_t) push_constant_ranges.size(),
            .pPushConstantRanges = push_constant_ranges.data(),
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
        VkShaderModule vertex_shader, VkShaderModule fragment_shader,
        VkPipelineLayout layout, VkRenderPass render_pass)
{
    m_shader_stages[0] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader,
            .pName = "main",
    };

    m_shader_stages[1] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader,
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

    m_depth_stencil_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_NEVER,
            .depthBoundsTestEnable = VK_FALSE,
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
            .pDepthStencilState= &m_depth_stencil_ci,
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
    uint32_t attr_desc_count = 0;
    switch (format) {
        case VertexFormat::V2:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            attr_desc_count = 1;
            break;
        case VertexFormat::V2t2:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sf};
            attr_desc_count = 2;
            break;
        case VertexFormat::V2t3:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 2 * sf};
            attr_desc_count = 2;
            break;
        case VertexFormat::V2t22:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 4 * sf};
            attr_desc_count = 3;
            break;
        case VertexFormat::V2t222:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 4 * sf};
            m_attr_descs[3] = {3, 0, VK_FORMAT_R32G32_SFLOAT, 6 * sf};
            attr_desc_count = 4;
            break;
        case VertexFormat::V2c4:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            attr_desc_count = 2;
            break;
        case VertexFormat::V2c4t2:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 6 * sf};
            attr_desc_count = 3;
            break;
        case VertexFormat::V2c4t22:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 6 * sf};
            m_attr_descs[3] = {3, 0, VK_FORMAT_R32G32_SFLOAT, 8 * sf};
            attr_desc_count = 4;
            break;
        case VertexFormat::V2c44t2:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 6 * sf};
            m_attr_descs[3] = {3, 0, VK_FORMAT_R32G32_SFLOAT, 10 * sf};
            attr_desc_count = 4;
            break;
        case VertexFormat::V2c44t3:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 6 * sf};
            m_attr_descs[3] = {3, 0, VK_FORMAT_R32G32B32_SFLOAT, 10 * sf};
            attr_desc_count = 4;
            break;
        case VertexFormat::V2c44t22:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 6 * sf};
            m_attr_descs[3] = {3, 0, VK_FORMAT_R32G32_SFLOAT, 10 * sf};
            m_attr_descs[4] = {4, 0, VK_FORMAT_R32G32_SFLOAT, 12 * sf};
            attr_desc_count = 5;
            break;
        case VertexFormat::V2c44t222:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 6 * sf};
            m_attr_descs[3] = {3, 0, VK_FORMAT_R32G32_SFLOAT, 10 * sf};
            m_attr_descs[4] = {4, 0, VK_FORMAT_R32G32_SFLOAT, 12 * sf};
            m_attr_descs[5] = {5, 0, VK_FORMAT_R32G32_SFLOAT, 14 * sf};
            attr_desc_count = 6;
            break;
        case VertexFormat::V3n3:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sf};
            attr_desc_count = 2;
            break;
        case VertexFormat::V3n3t2:
            m_attr_descs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
            m_attr_descs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sf};
            m_attr_descs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 6 * sf};
            attr_desc_count = 3;
            break;
    }
    assert(attr_desc_count > 0 && attr_desc_count <= m_attr_descs.size());
    m_binding_desc = {
            .binding = 0,
            .stride = get_vertex_format_stride(format) * sf,
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


void PipelineCreateInfo::set_depth_test(DepthTest depth_test)
{
    switch (depth_test) {
        case DepthTest::Off:
            m_depth_stencil_ci.depthTestEnable = VK_FALSE;
            m_depth_stencil_ci.depthWriteEnable = VK_FALSE;
            break;
        case DepthTest::Less:
        case DepthTest::LessOrEqual:
            m_depth_stencil_ci.depthTestEnable = VK_TRUE;
            m_depth_stencil_ci.depthWriteEnable = VK_TRUE;
            m_depth_stencil_ci.depthCompareOp = (depth_test == DepthTest::Less)?
                    VK_COMPARE_OP_LESS : VK_COMPARE_OP_LESS_OR_EQUAL;
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
    return h;
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
