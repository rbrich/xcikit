// CommandBuffer.h created on 2019-12-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_COMMAND_BUFFER_H
#define XCI_GRAPHICS_VULKAN_COMMAND_BUFFER_H

#include <vulkan/vulkan.h>

namespace xci::graphics {

class Renderer;


class CommandBuffer {
public:
    explicit CommandBuffer(Renderer& renderer);
    ~CommandBuffer();

    void begin();
    void end();
    void submit();

    void transition_image_layout(VkImage image,
            VkImageLayout old_layout, VkImageLayout new_layout);
    void copy_buffer_to_image(VkBuffer buffer, VkImage image,
            uint32_t width, uint32_t height);

private:
    Renderer& m_renderer;
    VkCommandBuffer m_command_buffer;
};


} // namespace xci::graphics

#endif // include guard
