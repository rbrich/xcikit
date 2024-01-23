// Attachments.cpp created on 2024-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Attachments.h"
#include "VulkanError.h"

namespace xci::graphics {


void Attachments::create_renderpass(VkDevice device)
{
    std::vector<VkAttachmentDescription> attachment_desc;

    // color attachments
    for (const auto& color : m_color_attachments) {
        attachment_desc.push_back({
            .format = color.format,
            .samples = sample_count_flag(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = has_msaa() ? VK_ATTACHMENT_STORE_OP_DONT_CARE :
                                    VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = has_msaa() ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                                        color.final_layout,
        });
    }

    // depth/stencil attachment
    if (has_depth_stencil()) {
        attachment_desc.push_back({
            .format = depth_stencil_format(),
            .samples = sample_count_flag(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        });
    }

    // resolve attachments for MSAA
    if (has_msaa()) {
        for (const auto& color : m_color_attachments) {
            attachment_desc.push_back({
                .format = color.format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = color.final_layout,
            });
        }
    }

    // attachment references
    uint32_t index = 0;
    std::vector<VkAttachmentReference> attachment_ref;
    for ([[maybe_unused]] const auto& color : m_color_attachments) {
        attachment_ref.push_back({
            .attachment = index++,  // layout(location = X)
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    }
    uint32_t depth_stencil_ref = 0;
    if (has_depth_stencil()) {
        depth_stencil_ref = index;
        attachment_ref.push_back({
            .attachment = index++,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        });
    }
    uint32_t resolve_ref = 0;
    if (has_msaa()) {
        resolve_ref = index;
        for ([[maybe_unused]] const auto& color : m_color_attachments) {
            attachment_ref.push_back({
                .attachment = index++,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            });
        }
    }

    const VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = (uint32_t) m_color_attachments.size(),
            .pColorAttachments = attachment_ref.data(),
            .pResolveAttachments = has_msaa() ? &attachment_ref[resolve_ref] : nullptr,
            .pDepthStencilAttachment = has_depth_stencil() ? &attachment_ref[depth_stencil_ref] : nullptr,
    };

    const VkSubpassDependency dependencies[] = {
        { // color attachment
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        {  // depth attachment
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        { // transfer color attachment
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
    };

    const VkRenderPassCreateInfo render_pass_ci = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = uint32_t(attachment_desc.size()),
            .pAttachments = attachment_desc.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 3,
            .pDependencies = dependencies,
    };

    VK_TRY("vkCreateRenderPass",
            vkCreateRenderPass(device, &render_pass_ci, nullptr, &m_render_pass));
}


void Attachments::destroy_renderpass(VkDevice device)
{
    vkDestroyRenderPass(device, m_render_pass, nullptr);
    m_render_pass = nullptr;
}


VkFormat Attachments::depth_stencil_format() const
{
    if (m_depth_bits == 32 && m_stencil_bits == 0) return VK_FORMAT_D32_SFLOAT;
    if (m_depth_bits == 32 && m_stencil_bits == 8) return VK_FORMAT_D32_SFLOAT_S8_UINT;
    if (m_depth_bits == 0 && m_stencil_bits == 8) return VK_FORMAT_S8_UINT;
    if (m_depth_bits == 16 && m_stencil_bits == 0) return VK_FORMAT_D16_UNORM;
    if (m_depth_bits == 16 && m_stencil_bits == 8) return VK_FORMAT_D16_UNORM_S8_UINT;
    if (m_depth_bits == 24 && m_stencil_bits == 8) return VK_FORMAT_D24_UNORM_S8_UINT;
    assert("Unsupported depth/stencil format!");
    return VK_FORMAT_D32_SFLOAT_S8_UINT;
}


auto Attachments::vk_clear_values() const -> std::vector<VkClearValue>
{
    std::vector<VkClearValue> clear_values;
    for (const auto& color_attachment : m_color_attachments) {
        clear_values.push_back(VkClearValue{.color = color_attachment.clear_value});
    }
    if (has_depth_stencil()) {
        clear_values.push_back(VkClearValue{.depthStencil = {1.0f, 0}});
    }
    return clear_values;
}


} // namespace xci::graphics
