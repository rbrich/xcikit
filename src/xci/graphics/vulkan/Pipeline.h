// Pipeline.h created on 2021-08-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_PIPELINE_H
#define XCI_GRAPHICS_VULKAN_PIPELINE_H

#include "DescriptorPool.h"

#include <xci/core/mixin.h>

#include <vulkan/vulkan.h>

#include <array>
#include <vector>
#include <functional>  // struct hash

namespace xci::graphics {

class Renderer;
class Shader;


enum class VertexFormat {
    V2t2,       // 2 vertex coords, 2 texture coords (all float)
    V2t22,      // 2 vertex coords, 2 + 2 texture coords (all float)
    V2c4t2,     // 2 vertex coords, RGBA color, 2 texture coords (all float)
    V2c4t22,    // 2 vertex coords, RGBA color, 2 + 2 texture coords (all float)
};


enum class BlendFunc {
    Off,
    AlphaBlend,
    InverseVideo,
};


class PipelineLayoutCreateInfo {
public:
    void add_uniform_binding(uint32_t binding);
    void add_texture_binding(uint32_t binding);

    std::vector<VkDescriptorSetLayoutBinding> vk_layout_bindings() const;
    DescriptorPoolSizes descriptor_pool_sizes() const;

    size_t hash() const;

    bool operator==(const PipelineLayoutCreateInfo& rhs) const;

private:
    std::array<uint32_t, 8> m_uniform_bindings;
    uint32_t m_uniform_binding_count = 0;
    uint32_t m_texture_binding = uint32_t(-1);
};


class PipelineLayout: private core::NonCopyable {
public:
    explicit PipelineLayout(Renderer& renderer, const PipelineLayoutCreateInfo& ci);
    ~PipelineLayout();

    const VkPipelineLayout& vk() const { return m_pipeline_layout; }
    const VkDescriptorSetLayout& vk_descriptor_set_layout() const { return m_descriptor_set_layout; }

private:
    Renderer& m_renderer;
    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;
};


class PipelineCreateInfo {
public:
    explicit PipelineCreateInfo(
            Shader& shader, VkPipelineLayout layout, VkRenderPass render_pass);

    void set_vertex_format(VertexFormat format);
    void set_color_blend(BlendFunc blend_func);

    const VkGraphicsPipelineCreateInfo& vk() const { return m_pipeline_ci; }

    size_t hash() const;

    bool operator==(const PipelineCreateInfo& rhs) const;

private:
    // only for hash and operator==
    VertexFormat m_format = static_cast<VertexFormat>(-1);
    BlendFunc m_blend_func = static_cast<BlendFunc>(-1);

    std::array<VkPipelineShaderStageCreateInfo, 2> m_shader_stages;
    VkVertexInputBindingDescription m_binding_desc {};
    std::array<VkVertexInputAttributeDescription, 4> m_attr_descs;
    VkPipelineVertexInputStateCreateInfo m_vertex_input_ci;
    VkPipelineInputAssemblyStateCreateInfo m_input_assembly_ci;
    VkPipelineViewportStateCreateInfo m_viewport_state_ci;
    VkPipelineRasterizationStateCreateInfo m_rasterization_ci;
    VkPipelineMultisampleStateCreateInfo m_multisample_ci;
    VkPipelineColorBlendAttachmentState m_color_blend {};
    VkPipelineColorBlendStateCreateInfo m_color_blend_ci;
    std::array<VkDynamicState, 2> m_dynamic_states;
    VkPipelineDynamicStateCreateInfo m_dynamic_state_ci;
    VkGraphicsPipelineCreateInfo m_pipeline_ci;
};


class Pipeline: private core::NonCopyable {
public:
    explicit Pipeline(Renderer& renderer, const PipelineCreateInfo& pipeline_ci);
    ~Pipeline();

    const VkPipeline& vk() const { return m_pipeline; }

private:
    Renderer& m_renderer;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};


} // namespace xci::graphics


namespace std {
    template <> struct hash<xci::graphics::PipelineLayoutCreateInfo> {
        size_t operator()(const xci::graphics::PipelineLayoutCreateInfo &x) const {
            return x.hash();
        }
    };

    template <> struct hash<xci::graphics::PipelineCreateInfo> {
        size_t operator()(const xci::graphics::PipelineCreateInfo &x) const {
            return x.hash();
        }
    };
}


#endif // include guard
