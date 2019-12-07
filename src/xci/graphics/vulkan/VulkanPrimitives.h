// VulkanPrimitives.h created on 2019-10-24, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_VULKAN_PRIMITIVES_H
#define XCI_GRAPHICS_VULKAN_PRIMITIVES_H

#include <xci/graphics/Primitives.h>
#include "VulkanWindow.h"
#include "VulkanMemory.h"

#include <vulkan/vulkan.h>
#include <array>

namespace xci::graphics {

class VulkanShader;
class VulkanRenderer;


class VulkanPrimitives : public Primitives {
public:
    explicit VulkanPrimitives(VulkanRenderer& renderer, VertexFormat format, PrimitiveType type);
    ~VulkanPrimitives() override;

    void reserve(size_t primitives, size_t vertices) override;

    void begin_primitive() override;
    void end_primitive() override;
    void add_vertex(ViewportCoords xy, float u, float v) override;
    void add_vertex(ViewportCoords xy, float u1, float v1, float u2, float v2) override;
    void add_vertex(ViewportCoords xy, Color c, float u, float v) override;
    void add_vertex(ViewportCoords xy, Color c, float u1, float v1, float u2, float v2) override;
    void clear() override;
    bool empty() const override;

    void set_shader(Shader& shader) override;
    void set_uniform(const char* name, float f) override;
    void set_uniform(const char* name, float f1, float f2, float f3, float f4) override;
    void set_blend(BlendFunc func) override;

    void draw(View& view) override;

private:
    VkDevice device() const;
    void create_pipeline();
    void create_buffers();
    void create_descriptor_set_layout();
    void create_descriptor_sets();
    void destroy_pipeline();

    auto make_binding_desc() -> VkVertexInputBindingDescription;
    uint32_t get_attr_desc_count();
    static constexpr size_t max_attr_descs = 4;
    auto make_attr_descs() -> std::array<VkVertexInputAttributeDescription, max_attr_descs>;

private:
    VertexFormat m_format;
    int m_closed_vertices = 0;
    int m_open_vertices = -1;
    std::vector<float> m_vertex_data;
    std::vector<uint16_t> m_index_data;
    static constexpr VkDeviceSize m_mvp_size = sizeof(float) * 16;

    VulkanRenderer& m_renderer;
    VulkanShader* m_shader = nullptr;
    VkDescriptorSetLayout m_descriptor_set_layout {};
    VkDescriptorPool m_descriptor_pool {};
    VkDescriptorSet m_descriptor_sets[VulkanWindow::cmd_buf_count] {};
    VkPipelineLayout m_pipeline_layout {};
    VkPipeline m_pipeline {};
    VkBuffer m_vertex_buffer {};
    VkBuffer m_index_buffer {};
    VkBuffer m_uniform_buffers[VulkanWindow::cmd_buf_count] {};
    VkDeviceSize m_uniform_offsets[VulkanWindow::cmd_buf_count] {};
    VulkanMemory m_device_memory;
};


} // namespace xci::graphics

#endif // include guard
