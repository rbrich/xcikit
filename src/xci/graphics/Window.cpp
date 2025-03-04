// Window.cpp created on 2019-10-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Window.h"
#include "Renderer.h"
#include "vulkan/VulkanError.h"
#include <xci/core/log.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace xci::graphics {

using namespace xci::core;
using namespace std::chrono;


Window::Window(Renderer& renderer) : m_renderer(renderer), m_command_buffers(renderer)
{
    m_sdl_wakeup_event = SDL_RegisterEvents(1);
}

Window::~Window()
{
    const auto vk_device = m_renderer.vk_device();
    if (vk_device != VK_NULL_HANDLE) {
        for (VkFence fence : m_cmd_buf_fences)
            vkDestroyFence(vk_device, fence, nullptr);
        for (VkSemaphore sem : m_render_semaphore)
            vkDestroySemaphore(vk_device, sem, nullptr);
        for (VkSemaphore sem : m_image_semaphore)
            vkDestroySemaphore(vk_device, sem, nullptr);
    }

    m_command_buffers.destroy();
    m_renderer.destroy_surface();

    if (m_window != nullptr)
        SDL_DestroyWindow(m_window);
}


bool Window::create(const Vec2u& size, const std::string& title)
{
    // See https://wiki.libsdl.org/SDL3/README/highdpi
    float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (scale == 0.0f)
        scale = 1.0f;
    m_window = SDL_CreateWindow(title.c_str(),
        int(size.x * scale), int(size.y * scale),
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_window) {
        log::error("{} failed: {}", "SDL_CreateWindow", SDL_GetError());
        return false;
    }

    return m_renderer.create_surface(m_window);
}


void Window::display()
{
    setup_view();

    auto t_last = steady_clock::now();
    while (!m_quit) {
        if (m_update_cb) {
            auto t_now = steady_clock::now();
            m_update_cb(m_view, t_now - t_last);
            t_last = t_now;
        }
        switch (m_refresh_mode) {
            case RefreshMode::OnDemand:
            case RefreshMode::OnEvent:
                if (m_refresh_mode == RefreshMode::OnEvent || m_view.pop_refresh())
                    draw();
                if (m_timeout == 0us) {
                    SDL_Event event;
                    if (SDL_WaitEvent(&event))
                        handle_event(event);
                } else {
                    SDL_Event event;
                    if (SDL_WaitEventTimeout(&event, int(m_timeout.count()) / 1000))
                        handle_event(event);
                    if (m_clear_timeout)
                        m_timeout = 0us;
                }
                break;
            case RefreshMode::Periodic: {
                draw();
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    handle_event(event);
                }
                break;
            }
        }
    }
    vkDeviceWaitIdle(m_renderer.vk_device());
}


void Window::wakeup() const
{
    SDL_Event event {
        .type = m_sdl_wakeup_event
    };
    if (!SDL_PushEvent(&event)) {
        log::error("{} failed: {}", "SDL_PushEvent", SDL_GetError());
    }
}


void Window::close()
{
    m_quit = true;
    wakeup();
}


void Window::set_fullscreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
    auto fullscreen_mode = m_fullscreen_mode;
    if (fullscreen_mode == FullscreenMode::Default) {
        fullscreen_mode = FullscreenMode::Desktop;
    }

    if (fullscreen_mode == FullscreenMode::BorderlessWindow) {
        if (m_fullscreen) {
            SDL_SetWindowBordered(m_window, false);
            SDL_MaximizeWindow(m_window);
        } else {
            SDL_RestoreWindow(m_window);
            SDL_SetWindowBordered(m_window, true);
        }
        return;
    }

    if (m_fullscreen && fullscreen_mode == FullscreenMode::Exclusive) {
        const SDL_DisplayMode* video_mode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(m_window));
        if (video_mode != nullptr) {
            SDL_SetWindowFullscreenMode(m_window, video_mode);
        }
    } else {
        SDL_SetWindowFullscreenMode(m_window, nullptr);
    }

    if (!SDL_SetWindowFullscreen(m_window, m_fullscreen)) {
        log::error("{} failed: {}", "SDL_SetWindowFullscreen", SDL_GetError());
    }
}


Vec2u Window::get_size() const
{
    Vec2i size;
    if (!SDL_GetWindowSize(m_window, &size.x, &size.y)) {
        log::error("{} failed: {}", "SDL_GetWindowSize", SDL_GetError());
        return Vec2u{};
    }
    // See https://wiki.libsdl.org/SDL3/README/highdpi
    float scale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(m_window));
    if (scale != 0.0f && scale != 1.0f)
        return Vec2u{unsigned(size.x / scale), unsigned(size.y / scale)};
    return Vec2u{size};
}


bool Window::set_clipboard_text(const std::string& text) const
{
    if (!SDL_SetClipboardText(text.c_str())) {
        log::error("{} failed: {}", "SDL_SetClipboardText", SDL_GetError());
        return false;
    }
    return true;
}


std::string Window::get_clipboard_text() const
{
    char* text = SDL_GetClipboardText();
    std::string res(text);
    SDL_free(text);
    return res;
}


void Window::set_draw_callback(Window::DrawCallback draw_cb)
{
    m_draw_cb = std::move(draw_cb);
}


void Window::set_clear_color(Color color)
{
    LinearColor c(color);
    m_renderer.swapchain().attachments().set_clear_color_value(0, {c.r, c.g, c.b, c.a});
}


void Window::set_refresh_timeout(std::chrono::microseconds timeout,
                                       bool periodic)
{
    m_timeout = timeout;
    m_clear_timeout = !periodic;
}


void Window::set_view_origin(ViewOrigin origin)
{
    m_view.set_origin(origin);
}


void Window::set_debug_flags(View::DebugFlags flags)
{
    m_view.set_debug_flags(flags);
}


Renderer& Window::renderer() const
{
    return m_renderer;
}


void Window::setup_view()
{
    auto fsize = m_renderer.vk_image_extent();
    m_view.set_framebuffer_size({float(fsize.width), float(fsize.height)});

    const auto sc_size = get_size();
    m_view.set_screen_size({float(sc_size.x), float(sc_size.y)});
    if (m_size_cb)
        m_size_cb(m_view);

    create_command_buffers();

    // This is a workaround for https://github.com/libsdl-org/SDL/issues/1059
    // (Still doesn't get resize events on Mac nor Windows with SDL 3.2.0,
    // this workaround still helps)
    SDL_SetEventFilter([](void* data, SDL_Event* event){
        if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            auto* self = (Window*) data;
            self->handle_event(*event);
            return false;
        }
        return true;
    }, this);
}


static Key translate_sdl_keycode(SDL_Keycode key)
{
    // Printable keys
    if (key >= SDLK_0 && key <= SDLK_9)
        return Key(key - SDLK_0 + int(Key::Num0));
    if (key >= SDLK_A && key <= SDLK_Z)
        return Key(key - SDLK_A + int(Key::A));

    // Function keys
    if (key >= SDLK_F1 && key <= SDLK_F12)
        return Key(key - SDLK_F1 + int(Key::F1));

    switch (key) {
        // Printable keys (continued)
        case SDLK_SPACE: return Key::Space;
        case SDLK_APOSTROPHE: return Key::Apostrophe;
        case SDLK_COMMA: return Key::Comma;
        case SDLK_MINUS: return Key::Minus;
        case SDLK_PERIOD: return Key::Period;
        case SDLK_SLASH: return Key::Slash;
        case SDLK_SEMICOLON: return Key::Semicolon;
        case SDLK_EQUALS: return Key::Equal;
        case SDLK_GRAVE: return Key::Backtick;
        case SDLK_LEFTBRACKET: return Key::LeftBracket;
        case SDLK_BACKSLASH: return Key::Backslash;
        case SDLK_RIGHTBRACKET: return Key::RightBracket;
        // Function keys (continued)
        case SDLK_ESCAPE: return Key::Escape;
        case SDLK_RETURN: return Key::Return;
        case SDLK_BACKSPACE: return Key::Backspace;
        case SDLK_TAB: return Key::Tab;
        case SDLK_INSERT: return Key::Insert;
        case SDLK_DELETE: return Key::Delete;
        case SDLK_HOME: return Key::Home;
        case SDLK_END: return Key::End;
        case SDLK_PAGEUP: return Key::PageUp;
        case SDLK_PAGEDOWN: return Key::PageDown;
        case SDLK_LEFT: return Key::Left;
        case SDLK_RIGHT: return Key::Right;
        case SDLK_UP: return Key::Up;
        case SDLK_DOWN: return Key::Down;
        case SDLK_CAPSLOCK: return Key::CapsLock;
        case SDLK_SCROLLLOCK: return Key::ScrollLock;
        case SDLK_NUMLOCKCLEAR: return Key::NumLock;
        case SDLK_PRINTSCREEN: return Key::PrintScreen;
        case SDLK_PAUSE: return Key::Pause;
        // Keypad
        case SDLK_KP_0: return Key::Keypad0;
        case SDLK_KP_1: return Key::Keypad1;
        case SDLK_KP_2: return Key::Keypad2;
        case SDLK_KP_3: return Key::Keypad3;
        case SDLK_KP_4: return Key::Keypad4;
        case SDLK_KP_5: return Key::Keypad5;
        case SDLK_KP_6: return Key::Keypad6;
        case SDLK_KP_7: return Key::Keypad7;
        case SDLK_KP_8: return Key::Keypad8;
        case SDLK_KP_9: return Key::Keypad9;
        case SDLK_KP_PLUS: return Key::KeypadPlus;
        case SDLK_KP_MINUS: return Key::KeypadMinus;
        case SDLK_KP_MULTIPLY: return Key::KeypadMultiply;
        case SDLK_KP_DIVIDE: return Key::KeypadDivide;
        case SDLK_KP_DECIMAL: return Key::KeypadDecimalPoint;
        case SDLK_KP_ENTER: return Key::KeypadEnter;
        // Modifier keys
        case SDLK_LSHIFT: return Key::LeftShift;
        case SDLK_RSHIFT: return Key::RightShift;
        case SDLK_LCTRL: return Key::LeftControl;
        case SDLK_RCTRL: return Key::RightControl;
        case SDLK_LALT: return Key::LeftAlt;
        case SDLK_RALT: return Key::RightAlt;
        case SDLK_LGUI: return Key::LeftSuper;
        case SDLK_RGUI: return Key::RightSuper;
        case SDLK_MENU: return Key::Menu;
        // Unknown keys
        case SDLK_UNKNOWN: return Key::Unknown;
        default:
            log::debug("GlWindow: unknown key: {}", key);
            return Key::Unknown; break;
    }
}


static MouseButton translate_sdl_mouse_button(uint8_t button)
{
    switch (button) {
        case SDL_BUTTON_LEFT: return MouseButton::Left;
        case SDL_BUTTON_RIGHT: return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_X1: return MouseButton::Ext1;
        case SDL_BUTTON_X2: return MouseButton::Ext2;
        default:
            assert(false);
            return MouseButton(button);
    }
}


void Window::handle_event(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_EVENT_QUIT:
            TRACE("SDL quit event");
            m_quit = true;
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            m_quit = true;
            break;

        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
            TRACE("Window display or size changed ({})", event.type);
            resize_framebuffer();
            draw();
            break;
        }

        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_SHOWN:
            TRACE("Window refresh event: ({})", event.type);
            m_view.refresh();
            break;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (m_key_cb) {
                Key ev_key = translate_sdl_keycode(event.key.key);
                Action action = event.key.down ? Action::Press : Action::Release;
                if (event.key.repeat)
                    action = Action::Repeat;
                const ModKey mod = {
                    bool(event.key.mod & SDL_KMOD_SHIFT),
                    bool(event.key.mod & SDL_KMOD_CTRL),
                    bool(event.key.mod & SDL_KMOD_ALT),
                    bool(event.key.mod & SDL_KMOD_GUI),
                };
                m_key_cb(m_view, KeyEvent{ev_key, mod, action});
            }
            break;

        case SDL_EVENT_TEXT_INPUT:
            if (m_text_cb) {
                TextInputEvent ev{};
                std::memcpy(ev.text, event.text.text,
                            std::min(sizeof(ev.text), std::strlen(event.text.text) + 1));
                m_text_cb(m_view, ev);
            }
            break;

        case SDL_EVENT_TEXT_EDITING:
            if (m_text_cb) {
                TextInputEvent ev{};
                std::memcpy(ev.text, event.edit.text,
                            std::min(sizeof(ev.text), std::strlen(event.edit.text) + 1));
                ev.edit_cursor = event.edit.start;
                ev.edit_length = event.edit.length;
                m_text_cb(m_view, ev);
            }
            break;

        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            TRACE("Window mouse enter/leave: ({})", event.type);
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            TRACE("Window focus gained/lost: ({})", event.type);
            break;

        case SDL_EVENT_MOUSE_MOTION:
            if (m_mpos_cb) {
                float scale = SDL_GetWindowPixelDensity(m_window);
                if (scale == 0.0f)
                    scale = 1.0f;
                const auto pos = FramebufferCoords(event.motion.x * scale, event.motion.y * scale) - m_view.framebuffer_origin();
                const FramebufferCoords rel {event.motion.xrel * scale, event.motion.yrel * scale};
                m_mpos_cb(m_view, MousePosEvent{pos, rel});
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (m_mbtn_cb) {
                float scale = SDL_GetWindowPixelDensity(m_window);
                if (scale == 0.0f)
                    scale = 1.0f;
                const auto pos = FramebufferCoords(event.button.x * scale, event.button.y * scale) - m_view.framebuffer_origin();
                const Action action = event.button.down ? Action::Press : Action::Release;
                const MouseButton button = translate_sdl_mouse_button(event.button.button);
                m_mbtn_cb(m_view, MouseBtnEvent{button, action, pos});
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            if (m_scroll_cb) {
                m_scroll_cb(m_view, ScrollEvent{ .offset={event.wheel.x, event.wheel.y} });
            }
            break;

        case SDL_EVENT_USER:
            assert(event.type == m_sdl_wakeup_event);
            TRACE("Window wakeup event");
            break;

        default:
            TRACE("SDL event not handled: {}", event.type);
            break;
    }
}


void Window::create_command_buffers()
{
    m_command_buffers.create(m_renderer.vk_command_pool(), cmd_buf_count);

    const VkFenceCreateInfo fence_ci {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    const VkSemaphoreCreateInfo semaphore_ci = {
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


void Window::finish_draw()
{
    if (m_draw_finished)
        return;
    m_draw_finished = true;

    vkDeviceWaitIdle(m_renderer.vk_device());

    m_command_buffers.reset();
}


void Window::resize_framebuffer()
{
    int fb_width, fb_height;
    SDL_GetWindowSizeInPixels(m_window, &fb_width, &fb_height);
    const VkExtent2D fb_size {uint32_t(fb_width), uint32_t(fb_height)};
    {
        const auto& current_size = m_renderer.vk_image_extent();
        if (fb_size.width == current_size.width && fb_size.height == current_size.height)
            return;  // no change
    }

    m_renderer.reset_framebuffer(fb_size);

    const auto& actual_size = m_renderer.vk_image_extent();  // normally same as fb_size
    m_view.set_framebuffer_size({float(actual_size.width), float(actual_size.height)});

    const auto sc_size = get_size();
    m_view.set_screen_size({float(sc_size.x), float(sc_size.y)});

    if (m_size_cb)
        m_size_cb(m_view);
}


void Window::draw()
{
    VK_TRY("vkWaitForFences",
            vkWaitForFences(m_renderer.vk_device(),
                    1, &m_cmd_buf_fences[m_current_cmd_buf], VK_TRUE, UINT64_MAX));
    VK_TRY("vkResetFences",
            vkResetFences(m_renderer.vk_device(),
                    1, &m_cmd_buf_fences[m_current_cmd_buf]));

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
        log::error("vkAcquireNextImageKHR failed: {}", int(rc));
        return;
    }

    {
        m_command_buffers[m_current_cmd_buf].release_resources();

        auto& cmd_buf = m_command_buffers[m_current_cmd_buf];
        cmd_buf.begin();
        m_command_buffers.trigger_callbacks(CommandBuffers::Event::Init, m_current_cmd_buf, image_index);

        auto clear_values = m_renderer.swapchain().attachments().vk_clear_values();

        const VkRenderPassBeginInfo render_pass_info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = m_renderer.vk_render_pass(),
                .framebuffer = m_renderer.vk_framebuffer(image_index),
                .renderArea = {
                        .offset = {0, 0},
                        .extent = m_renderer.vk_image_extent(),
                },
                .clearValueCount = (uint32_t) clear_values.size(),
                .pClearValues = clear_values.data(),
        };
        vkCmdBeginRenderPass(cmd_buf.vk(), &render_pass_info,
                VK_SUBPASS_CONTENTS_INLINE);

        // Set viewport
        cmd_buf.set_viewport(Vec2f(m_view.framebuffer_size()), false);

        if (m_draw_cb)
            m_draw_cb(m_view);

        vkCmdEndRenderPass(cmd_buf.vk());

        m_command_buffers.trigger_callbacks(CommandBuffers::Event::Finish, m_current_cmd_buf, image_index);
        cmd_buf.end();

        cmd_buf.submit(m_renderer.vk_queue(),
                       m_image_semaphore[m_current_cmd_buf],
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       m_render_semaphore[m_current_cmd_buf],
                       m_cmd_buf_fences[m_current_cmd_buf]);
    }

    VkSwapchainKHR swapchains[] = {m_renderer.vk_swapchain()};
    const VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_render_semaphore[m_current_cmd_buf],
            .swapchainCount = 1,
            .pSwapchains = swapchains,
            .pImageIndices = &image_index,
    };
    rc = vkQueuePresentKHR(m_renderer.vk_queue(), &present_info);
    if (rc == VK_ERROR_OUT_OF_DATE_KHR || rc == VK_SUBOPTIMAL_KHR)
        m_renderer.reset_framebuffer();
    else if (rc != VK_SUCCESS)
        log::error("vkQueuePresentKHR failed: {}", int(rc));

    m_current_cmd_buf = (m_current_cmd_buf + 1) % cmd_buf_count;
    m_draw_finished = false;
}


} // namespace xci::graphics
