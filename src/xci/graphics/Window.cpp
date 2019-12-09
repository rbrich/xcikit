// Window.cpp created on 2019-10-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Window.h"
#include "Renderer.h"
#include "vulkan/VulkanError.h"
#include <xci/core/log.h>

namespace xci::graphics {

using namespace xci::core::log;
using namespace std::chrono;


Window::Window(Renderer& renderer)
  : m_renderer(renderer) {}


Window::~Window()
{
    for (auto* fence : m_cmd_buf_fences)
        vkDestroyFence(m_renderer.vk_device(), fence, nullptr);
    for (auto* sem : m_render_semaphore)
        vkDestroySemaphore(m_renderer.vk_device(), sem, nullptr);
    for (auto* sem : m_image_semaphore)
        vkDestroySemaphore(m_renderer.vk_device(), sem, nullptr);
    if (m_window != nullptr)
        glfwDestroyWindow(m_window);
}


void Window::create(const Vec2u& size, const std::string& title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(size.x, size.y, title.c_str(),
                                nullptr, nullptr);
    if (!m_window) {
        log_error("Couldn't create GLFW window...");
        exit(1);
    }
    glfwSetWindowUserPointer(m_window, this);

    m_renderer.init(m_window);
}


void Window::display()
{
    setup_view();

    auto t_last = steady_clock::now();
    while (!glfwWindowShouldClose(m_window)) {
        if (m_update_cb) {
            auto t_now = steady_clock::now();
            m_update_cb(m_view, t_now - t_last);
            t_last = t_now;
        }
        switch (m_mode) {
            case RefreshMode::OnDemand:
            case RefreshMode::OnEvent:
                if (m_mode == RefreshMode::OnEvent || m_view.pop_refresh())
                    draw();
                if (m_timeout == 0us) {
                    glfwWaitEvents();
                } else {
                    glfwWaitEventsTimeout(double(m_timeout.count()) / 1e6);
                    if (m_clear_timeout)
                        m_timeout = 0us;
                }
                break;
            case RefreshMode::Periodic:
                draw();
                glfwPollEvents();
                break;
        }
    }
    vkDeviceWaitIdle(m_renderer.vk_device());
}


void Window::set_draw_callback(Window::DrawCallback draw_cb)
{
    m_draw_cb = std::move(draw_cb);
    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow* window) {
        auto self = (Window*) glfwGetWindowUserPointer(window);
        self->draw();
    });
}


void Window::set_mouse_position_callback(Window::MousePosCallback mpos_cb)
{
    m_mpos_cb = std::move(mpos_cb);
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos) {
        auto self = (Window*) glfwGetWindowUserPointer(window);
        if (self->m_mpos_cb) {
            auto pos = self->m_view.coords_to_viewport(ScreenCoords{float(xpos), float(ypos)});
            self->m_mpos_cb(self->m_view, {pos});
        }
    });
}


void Window::set_mouse_button_callback(Window::MouseBtnCallback mbtn_cb)
{
    m_mbtn_cb = std::move(mbtn_cb);
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
        auto self = (Window*) glfwGetWindowUserPointer(window);
        if (self->m_mbtn_cb) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            auto pos = self->m_view.coords_to_viewport(ScreenCoords{float(xpos), float(ypos)});
            self->m_mbtn_cb(self->m_view,
                            {(MouseButton) button, (Action) action, pos});
        }
    });
}


void Window::set_scroll_callback(Window::ScrollCallback scroll_cb)
{
    m_scroll_cb = std::move(scroll_cb);
    if (m_scroll_cb) {
        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset,
                                           double yoffset) {
            auto self = (Window*) glfwGetWindowUserPointer(window);
            self->m_scroll_cb(self->m_view, {{float(xoffset), float(yoffset)}});
        });
    } else {
        glfwSetScrollCallback(m_window, nullptr);
    }
}


void Window::set_refresh_interval(int interval)
{
    glfwSwapInterval(interval);
}


void Window::set_refresh_timeout(std::chrono::microseconds timeout,
                                       bool periodic)
{
    m_timeout = timeout;
    m_clear_timeout = !periodic;
}


void Window::set_view_mode(ViewOrigin origin, ViewScale scale)
{
    m_view.set_viewport_mode(origin, scale);
}


void Window::set_debug_flags(View::DebugFlags flags)
{
    m_view.set_debug_flags(flags);
}


Renderer& Window::renderer()
{
    return m_renderer;
}


void Window::setup_view()
{
    auto fsize = m_renderer.vk_image_extent();
    m_view.set_framebuffer_size({float(fsize.width), float(fsize.height)});

    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    m_view.set_screen_size({float(width), float(height)});
    if (m_size_cb)
        m_size_cb(m_view);

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        TRACE("Framebuffer resize: {} {}", w, h);
        auto self = (Window*) glfwGetWindowUserPointer(win);
        if (self->m_view.set_framebuffer_size({float(w), float(h)}) && self->m_size_cb)
            self->m_size_cb(self->m_view);
        self->m_renderer.reset_framebuffer({uint32_t(w), uint32_t(h)});
        self->draw();
    });

    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        TRACE("Window resize: {} {}", w, h);
        auto self = (Window*) glfwGetWindowUserPointer(win);
        if (self->m_view.set_screen_size({float(w), float(h)}) && self->m_size_cb)
            self->m_size_cb(self->m_view);
    });

    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow* win) {
        TRACE("Window refresh");
        auto self = (Window*) glfwGetWindowUserPointer(win);
        self->draw();
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode,
                                    int action, int mods) {
        auto self = (Window*) glfwGetWindowUserPointer(window);

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            return;
        }

        if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
            // Toggle fullscreen / windowed mode
            auto& pos = self->m_window_pos;
            auto& size = self->m_window_size;
            if (glfwGetWindowMonitor(window)) {
                glfwSetWindowMonitor(window, nullptr, pos.x, pos.y, size.x, size.y, 0);
            } else {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                if (monitor) {
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    glfwGetWindowPos(window, &pos.x, &pos.y);
                    glfwGetWindowSize(window, &size.x, &size.y);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                }
            }
            return;
        }

        if (self->m_key_cb) {
            Key ev_key;
            if ((key == GLFW_KEY_SPACE)
            ||  (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
            ||  (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
            ||  (key >= GLFW_KEY_LEFT_BRACKET && key <= GLFW_KEY_RIGHT_BRACKET)) {
                ev_key = Key(key);

            } else if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
                ev_key = Key(key - GLFW_KEY_F1 + (int)Key::F1);

            } else if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
                ev_key = Key(key - GLFW_KEY_KP_0 + (int)Key::Keypad0);

            } else {
                switch (key) {
                    case GLFW_KEY_ESCAPE: ev_key = Key::Escape; break;
                    case GLFW_KEY_ENTER: ev_key = Key::Enter; break;
                    case GLFW_KEY_BACKSPACE: ev_key = Key::Backspace; break;
                    case GLFW_KEY_TAB: ev_key = Key::Tab; break;
                    case GLFW_KEY_INSERT: ev_key = Key::Insert; break;
                    case GLFW_KEY_DELETE: ev_key = Key::Delete; break;
                    case GLFW_KEY_HOME: ev_key = Key::Home; break;
                    case GLFW_KEY_END: ev_key = Key::End; break;
                    case GLFW_KEY_PAGE_UP: ev_key = Key::PageUp; break;
                    case GLFW_KEY_PAGE_DOWN: ev_key = Key::PageDown; break;
                    case GLFW_KEY_LEFT: ev_key = Key::Left; break;
                    case GLFW_KEY_RIGHT: ev_key = Key::Right; break;
                    case GLFW_KEY_UP: ev_key = Key::Up; break;
                    case GLFW_KEY_DOWN: ev_key = Key::Down; break;
                    case GLFW_KEY_CAPS_LOCK: ev_key = Key::CapsLock; break;
                    case GLFW_KEY_SCROLL_LOCK: ev_key = Key::ScrollLock; break;
                    case GLFW_KEY_NUM_LOCK: ev_key = Key::NumLock; break;
                    case GLFW_KEY_PRINT_SCREEN: ev_key = Key::PrintScreen; break;
                    case GLFW_KEY_PAUSE: ev_key = Key::Pause; break;
                    case GLFW_KEY_SPACE: ev_key = Key::Space; break;
                    case GLFW_KEY_KP_ADD: ev_key = Key::KeypadPlus; break;
                    case GLFW_KEY_KP_SUBTRACT: ev_key = Key::KeypadMinus; break;
                    case GLFW_KEY_KP_MULTIPLY: ev_key = Key::KeypadAsterisk; break;
                    case GLFW_KEY_KP_DIVIDE: ev_key = Key::KeypadSlash; break;
                    case GLFW_KEY_KP_DECIMAL: ev_key = Key::KeypadDecimalPoint; break;
                    case GLFW_KEY_KP_ENTER: ev_key = Key::KeypadEnter; break;
                    default:
                        log_debug("GlWindow: unknown key: {}", key);
                        ev_key = Key::Unknown; break;
                }
            }

            static_assert(int(Action::Release) == GLFW_RELEASE, "GLFW_RELEASE");
            static_assert(int(Action::Press) == GLFW_PRESS, "GLFW_PRESS");
            static_assert(int(Action::Repeat) == GLFW_REPEAT, "GLFW_REPEAT");
            ModKey mod = {
                    bool(mods & GLFW_MOD_SHIFT),
                    bool(mods & GLFW_MOD_CONTROL),
                    bool(mods & GLFW_MOD_ALT),
            };
            self->m_key_cb(self->m_view, KeyEvent{ev_key, mod, Action(action)});
        }
    });

    glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int codepoint) {
        auto self = (Window*) glfwGetWindowUserPointer(window);
        if (self->m_char_cb) {
            self->m_char_cb(self->m_view, CharEvent{codepoint});
        }
    });

    create_command_buffers();
}


void Window::create_command_buffers()
{
    VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_renderer.vk_command_pool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = cmd_buf_count,
    };
    VK_TRY("vkAllocateCommandBuffers",
            vkAllocateCommandBuffers(m_renderer.vk_device(), &alloc_info,
                    m_command_buffers));

    VkFenceCreateInfo fence_ci {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkSemaphoreCreateInfo semaphore_ci = {
           .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for (size_t i = 0; i < cmd_buf_count; ++i) {
        VK_TRY("vkCreateFence",
                vkCreateFence(m_renderer.vk_device(), &fence_ci,
                        nullptr, &m_cmd_buf_fences[i]));
        VK_TRY("vkCreateSemaphore",
                vkCreateSemaphore(m_renderer.vk_device(), &semaphore_ci,
                        nullptr, &m_image_semaphore[i]));
        VK_TRY("vkCreateSemaphore",
                vkCreateSemaphore(m_renderer.vk_device(), &semaphore_ci,
                        nullptr, &m_render_semaphore[i]));
    }
}


void Window::reset_command_buffers()
{
    vkDeviceWaitIdle(m_renderer.vk_device());

    for (size_t i = 0; i < cmd_buf_count; ++i) {
        VK_TRY("vkBeginCommandBuffer",
                vkResetCommandBuffer(m_command_buffers[i], 0));
    }
}


void Window::draw()
{
    uint32_t image_index;
    auto rc = vkAcquireNextImageKHR(m_renderer.vk_device(),
            m_renderer.vk_swapchain(), UINT64_MAX,
            m_image_semaphore[m_current_cmd_buf],
            VK_NULL_HANDLE, &image_index);
    if (rc == VK_ERROR_OUT_OF_DATE_KHR) {
        m_renderer.reset_framebuffer();
        wakeup();
        return;
    }
    if (rc != VK_SUCCESS && rc != VK_SUBOPTIMAL_KHR) {
        // VK_SUBOPTIMAL_KHR handled later with vkQueuePresentKHR
        log_error("vkAcquireNextImageKHR failed: {}", rc);
        return;
    }

    auto* cmd_buf = m_command_buffers[m_current_cmd_buf];

    {
        VK_TRY("vkWaitForFences",
                vkWaitForFences(m_renderer.vk_device(),
                        1, &m_cmd_buf_fences[m_current_cmd_buf], VK_TRUE, UINT64_MAX));
        VK_TRY("vkResetFences",
                vkResetFences(m_renderer.vk_device(),
                        1, &m_cmd_buf_fences[m_current_cmd_buf]));

        VkCommandBufferBeginInfo begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        VK_TRY("vkBeginCommandBuffer",
                vkBeginCommandBuffer(cmd_buf, &begin_info));

        VkClearValue clear_value = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkRenderPassBeginInfo render_pass_info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = m_renderer.vk_render_pass(),
                .framebuffer = m_renderer.vk_framebuffer(image_index),
                .renderArea = {
                        .offset = {0, 0},
                        .extent = m_renderer.vk_image_extent(),
                },
                .clearValueCount = 1,
                .pClearValues = &clear_value,
        };
        vkCmdBeginRenderPass(cmd_buf, &render_pass_info,
                VK_SUBPASS_CONTENTS_INLINE);

        if (m_draw_cb)
            m_draw_cb(m_view);

        vkCmdEndRenderPass(cmd_buf);

        VK_TRY("vkEndCommandBuffer", vkEndCommandBuffer(cmd_buf));
    }

    VkSemaphore wait_semaphores[] = {m_image_semaphore[m_current_cmd_buf]};
    VkSemaphore signal_semaphores[] = {m_render_semaphore[m_current_cmd_buf]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = wait_semaphores,
            .pWaitDstStageMask = wait_stages,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buf,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signal_semaphores,
    };
    VK_TRY("vkQueueSubmit",
            vkQueueSubmit(m_renderer.vk_queue(), 1, &submit_info,
                    m_cmd_buf_fences[m_current_cmd_buf]));

    VkSwapchainKHR swapchains[] = {m_renderer.vk_swapchain()};
    VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signal_semaphores,
            .swapchainCount = 1,
            .pSwapchains = swapchains,
            .pImageIndices = &image_index,
    };
    rc = vkQueuePresentKHR(m_renderer.vk_queue(), &present_info);
    if (rc == VK_ERROR_OUT_OF_DATE_KHR || rc == VK_SUBOPTIMAL_KHR)
        m_renderer.reset_framebuffer();
    else if (rc != VK_SUCCESS)
        log_error("vkQueuePresentKHR failed: {}", rc);

    m_current_cmd_buf = (m_current_cmd_buf + 1) % cmd_buf_count;
}


} // namespace xci::graphics
