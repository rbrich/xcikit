// CommandBuffer.h created on 2019-12-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_COMMAND_BUFFER_H
#define XCI_GRAPHICS_VULKAN_COMMAND_BUFFER_H

#include <vulkan/vulkan.h>
#include <xci/core/geometry.h>

namespace xci::graphics {

class Renderer;

using xci::core::Rect_u;


class CommandBuffer {
public:
    explicit CommandBuffer(Renderer& renderer);
    ~CommandBuffer();

    void begin();
    void end();
    void submit();

    void transition_image_layout(VkImage image,
            VkAccessFlags src_access, VkAccessFlags dst_access,
            VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
            VkImageLayout old_layout, VkImageLayout new_layout);
    void transition_buffer(VkBuffer buffer, VkDeviceSize size,
            VkAccessFlags src_access, VkAccessFlags dst_access,
            VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);
    void copy_buffer_to_image(VkBuffer buffer,
            VkDeviceSize buffer_offset, uint32_t buffer_row_len,
            VkImage image, const Rect_u& region);

    VkCommandBuffer vk_command_buffer() const { return m_command_buffer; }

private:
    Renderer& m_renderer;
    VkCommandBuffer m_command_buffer;
};


} // namespace xci::graphics

#endif // include guard
