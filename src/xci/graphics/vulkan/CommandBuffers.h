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
class CommandBuffers;


class CommandBuffer {
public:
    void begin();
    void end();

    void reset();

    void submit(VkQueue queue, VkFence fence = VK_NULL_HANDLE);
    void submit(VkQueue queue, VkSemaphore wait, VkPipelineStageFlags wait_stage,
                VkSemaphore signal, VkFence fence);

    /// Set viewport (vkCmdSetViewport)
    /// \param size         Viewport size (framebuffer size)
    /// \param flipped_y    Flip viewport Y for OpenGL compatibility
    void set_viewport(Vec2f size, bool flipped_y);

    /// Set scissor region
    void set_scissor(const Rect_u& region);

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
    void copy_image_to_buffer(VkImage image, const Rect_u& region,
            VkBuffer buffer, VkDeviceSize buffer_offset = 0, uint32_t buffer_row_len = 0);

     // Cleanup routines for resources used by the command buffer
     template <typename T> void add_resource(const T& resource) { add_cleanup([resource]{}); }
     void add_cleanup(std::function<void()>&& cb) { m_resources.push_back(std::move(cb)); }
     void release_resources();

    VkCommandBuffer vk() const { return m_vk_command_buffer; }

private:
    friend class CommandBuffers;
    VkCommandBuffer m_vk_command_buffer {};
    std::vector<std::function<void()>> m_resources;
};


class CommandBuffers : private core::NonCopyable {
public:
    explicit CommandBuffers(Renderer& renderer) : m_renderer(renderer) {}
    ~CommandBuffers() { destroy(); }

    void create(VkCommandPool command_pool, uint32_t count = 1);
    void destroy();

    void reset();
    void submit(unsigned idx = 0);

    CommandBuffer& operator[](size_t i) { return m_command_buffers[i]; }

    using CommandBufferCallback = std::function<void(CommandBuffer& cmd_buf, uint32_t image_index)>;
    enum class Event { Init, Finish };
    void add_callback(Event event, void* owner, CommandBufferCallback cb) { m_callbacks.push_back({std::move(cb), owner, event}); }
    void remove_callbacks(void* owner);
    void trigger_callbacks(Event event, unsigned i, uint32_t image_index);

    VkCommandBuffer vk(unsigned i = 0) const { return m_command_buffers[i].vk(); }

private:
    static constexpr size_t max_count = 2;

    Renderer& m_renderer;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    std::array<CommandBuffer, max_count> m_command_buffers;

    struct CallbackInfo {
        CommandBufferCallback cb;
        void* owner;
        Event event;
    };
    std::vector<CallbackInfo> m_callbacks;

    unsigned m_count = 0;
};


} // namespace xci::graphics

#endif // include guard
