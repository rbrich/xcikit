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


class CommandBuffer {
public:
    explicit CommandBuffer(VkCommandBuffer vk) : m_vk_command_buffer(vk) {}

    void begin();
    void end();
    void submit(VkQueue queue);
    void submit(VkQueue queue, VkSemaphore wait, VkPipelineStageFlags wait_stage,
                VkSemaphore signal, VkFence fence);

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

    VkCommandBuffer vk() const { return m_vk_command_buffer; }

private:
    VkCommandBuffer m_vk_command_buffer;
};


class CommandBuffers : private core::NonCopyable {
public:
    explicit CommandBuffers(Renderer& renderer) : m_renderer(renderer) {}
    ~CommandBuffers();

    void create(VkCommandPool command_pool, uint32_t count);
    void destroy();

    void reset();
    void submit(unsigned idx = 0);

    CommandBuffer buffer(unsigned i = 0) const { return CommandBuffer(m_command_buffers[i]); }
    CommandBuffer operator[](size_t i) const { return CommandBuffer(m_command_buffers[i]); }

    // Cleanup routines for resources used by current command buffer
    template <typename T> void add_resource(size_t i, const T& resource) { add_cleanup(i, [resource]{}); }
    void add_cleanup(size_t i, std::function<void()>&& cb) { m_resources[i].push_back(std::move(cb)); }
    void release_resources(size_t i);

    using CommandBufferCallback = std::function<void(CommandBuffer& cmd_buf)>;
    enum class Event { Init, Finish };
    void add_callback(Event event, void* owner, CommandBufferCallback cb) { m_callbacks.push_back({std::move(cb), owner, event}); }
    void remove_callbacks(void* owner);
    void trigger_callbacks(Event event, unsigned i = 0);

    VkCommandBuffer vk(unsigned i = 0) const { return m_command_buffers[i]; }

private:
    Renderer& m_renderer;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    static constexpr size_t max_count = 2;
    std::array<VkCommandBuffer, max_count> m_command_buffers {};
    std::array<std::vector<std::function<void()>>, max_count> m_resources;

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
