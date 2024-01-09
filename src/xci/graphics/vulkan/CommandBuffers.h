// CommandBuffers.h created on 2019-12-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_COMMAND_BUFFERS_H
#define XCI_GRAPHICS_VULKAN_COMMAND_BUFFERS_H

#include <vulkan/vulkan.h>
#include <xci/core/mixin.h>
#include <xci/math/Vec2.h>
#include <xci/math/Rect.h>
#include <array>
#include <vector>
#include <memory>
#include <functional>

namespace xci::graphics {

class Renderer;


// Inherit this to add support for resource management
class Resource {};
using ResourcePtr = std::shared_ptr<Resource>;


class CommandBuffers : private core::NonCopyable {
public:
    explicit CommandBuffers(Renderer& renderer) : m_renderer(renderer) {}
    ~CommandBuffers();

    void create(VkCommandPool command_pool, uint32_t count);
    void destroy();

    void reset();

    void begin(unsigned idx = 0);
    void end(unsigned idx = 0);
    void submit(unsigned idx = 0);

    void transition_image_layout(VkImage image,
            VkAccessFlags src_access, VkAccessFlags dst_access,
            VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
            VkImageLayout old_layout, VkImageLayout new_layout,
            uint32_t mip_base = 0, uint32_t mip_count = 1);
    void transition_buffer(VkBuffer buffer, VkDeviceSize size,
            VkAccessFlags src_access, VkAccessFlags dst_access,
            VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);
    void copy_buffer_to_image(VkBuffer buffer,
            VkDeviceSize buffer_offset, uint32_t buffer_row_len,
            VkImage image, const Rect_u& region);

    // Resources used by current command buffer
    void add_resource(size_t i, const ResourcePtr& resource) { m_resources[i].push_back([resource]{}); }
    void add_resource_deleter(size_t i, std::function<void()>&& deleter) { m_resources[i].push_back(std::move(deleter)); }
    void release_resources(size_t i);

    VkCommandBuffer vk() const { return m_command_buffers[0]; }
    VkCommandBuffer operator[](size_t i) const { return m_command_buffers[i]; }

private:
    Renderer& m_renderer;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    static constexpr size_t max_count = 2;
    std::array<VkCommandBuffer, max_count> m_command_buffers {};
    std::array<std::vector<std::function<void()>>, max_count> m_resources;
    unsigned m_count = 0;
};


} // namespace xci::graphics

#endif // include guard
