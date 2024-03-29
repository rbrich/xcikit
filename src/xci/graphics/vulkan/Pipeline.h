// Pipeline.h created on 2021-08-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_PIPELINE_H
#define XCI_GRAPHICS_VULKAN_PIPELINE_H

#include "DescriptorPool.h"

#include <xci/core/mixin.h>
#include <xci/compat/macros.h>

#include <vulkan/vulkan.h>

#include <array>
#include <vector>
#include <functional>  // struct hash

namespace xci::graphics {

class Renderer;
class Shader;
class Attachments;


enum class VertexFormat : uint8_t {
    // 2D
    V2,         // 2 vertex coords
    V2t2,       // 2 vertex coords, 2 texture coords (all float)
    V2t3,       // 2 vertex coords, 3 texture coords (or barycentric coords)
    V2t22,      // 2 vertex coords, 2 + 2 texture coords (all float)
    V2t222,     // 2 vertex coords, 2 + 2 + 2 texture coords (all float)
    V2c4,       // 2 vertex coords, RGBA color
    V2c4t2,     // 2 vertex coords, RGBA color, 2 texture coords (all float)
    V2c4t22,    // 2 vertex coords, RGBA color, 2 + 2 texture coords (all float)
    V2c44t2,    // 2 vertex coords, 2x RGBA color, 2 texture coords
    V2c44t3,    // 2 vertex coords, 2x RGBA color, 3 texture coords (or barycentric coords)
    V2c44t22,   // 2 vertex coords, 2x RGBA color, 2 + 2 texture coords (all float)
    V2c44t222,  // 2 vertex coords, 2x RGBA color, 2 + 2 + 2 texture coords (all float)

    // 3D
    V3n3,       // 3 vertex coords, 3 normals
    V3n3t2,     // 3 vertex coords, 3 normals, 2 tex coords
};

/// Get stride or size of vertex format data.
/// Counted in floats, i.e. multiply by 4 to get stride in bytes.
unsigned get_vertex_format_stride(VertexFormat format);


enum class BlendFunc : uint8_t {
    Off,
    AlphaBlend,
    InverseVideo,
};


enum class DepthTest : uint8_t {
    Off,
    Less,
    LessOrEqual,
};


class PipelineLayoutCreateInfo {
public:
    void add_uniform_binding(uint32_t binding, bool dynamic = false);
    void add_texture_binding(uint32_t binding);
    void add_storage_binding(uint32_t binding);
    void add_push_constant_range(uint32_t offset, uint32_t size);

    std::vector<VkDescriptorSetLayoutBinding> vk_layout_bindings() const;
    std::vector<VkPushConstantRange> vk_push_constant_ranges() const;
    DescriptorPoolSizes descriptor_pool_sizes() const;

    size_t hash() const;

    bool operator==(const PipelineLayoutCreateInfo& rhs) const = default;

private:
    struct LayoutBinding {
        uint32_t binding = 0;
        enum Flags : uint32_t {
            TypeUniform         = 0x00,  // default type
            TypeDynamicUniform  = 0x01,
            TypeImageSampler    = 0x02,
            TypeStorageBuffer   = 0x03,
            TypeMask            = 0x03,

            StageVertex         = 0x04,
            StageFragment       = 0x08,
            StageMask           = 0x0C,
        };
        uint32_t flags = 0;
        VkDescriptorType vk_descriptor_type() const {
            switch (flags & TypeMask) {
                case TypeUniform: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case TypeDynamicUniform: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                case TypeImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case TypeStorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            }
            XCI_UNREACHABLE;
        }
        bool operator==(const LayoutBinding& rhs) const = default;
    };
    std::vector<LayoutBinding> m_layout_bindings;

    struct PushConstantRange {
        uint32_t offset;
        uint32_t size;
        PushConstantRange(uint32_t offset, uint32_t size) : offset(offset), size(size) {}
        bool operator==(const PushConstantRange& rhs) const = default;
    };
    std::vector<PushConstantRange> m_push_constant_ranges;
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
            const Attachments& attachments,
            VkShaderModule vertex_shader, VkShaderModule fragment_shader,
            VkPipelineLayout layout);

    void set_vertex_format(VertexFormat format);
    void set_color_blend(BlendFunc blend_func, unsigned attachment = 0);
    void set_depth_test(DepthTest depth_test);

    const VkGraphicsPipelineCreateInfo& vk() const { return m_pipeline_ci; }

    size_t hash() const;

    bool operator==(const PipelineCreateInfo& rhs) const;

private:
    // only for hash and operator==
    VertexFormat m_format = static_cast<VertexFormat>(-1);
    BlendFunc m_blend_func = static_cast<BlendFunc>(-1);

    std::array<VkPipelineShaderStageCreateInfo, 2> m_shader_stages;
    VkVertexInputBindingDescription m_binding_desc {};
    std::array<VkVertexInputAttributeDescription, 6> m_attr_descs;
    VkPipelineVertexInputStateCreateInfo m_vertex_input_ci;
    VkPipelineInputAssemblyStateCreateInfo m_input_assembly_ci;
    VkPipelineViewportStateCreateInfo m_viewport_state_ci;
    VkPipelineRasterizationStateCreateInfo m_rasterization_ci;
    VkPipelineMultisampleStateCreateInfo m_multisample_ci;
    VkPipelineDepthStencilStateCreateInfo m_depth_stencil_ci;
    std::vector<VkPipelineColorBlendAttachmentState> m_color_blend;
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
} // namespace std


#endif // include guard
