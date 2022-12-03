// epoll/EventLoop.cpp created on 2019-03-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EventLoop.h"
#include <xci/core/log.h>
#include <algorithm>
#include <sys/inotify.h>
#include <unistd.h>
#include <cassert>

namespace xci::core {


EventLoop::EventLoop()
{
    m_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (m_epoll_fd == -1) {
        log::error("EventLoop: epoll_create1: {m}");
        return;
    }
}


EventLoop::~EventLoop()
{
    if (m_epoll_fd != -1)
        ::close(m_epoll_fd);
}


void EventLoop::run()
{
    constexpr int maxevents = 10;
    struct epoll_event events[maxevents] = {};
    while (!m_terminate) {
        int rnum = epoll_wait(m_epoll_fd, events, maxevents, -1);
        if (rnum == -1) {
            if (errno == EINTR)
                continue;
            log::error("EventLoop: poll: {m}");
            break;
        }

        // check events
        assert(rnum <= maxevents);
        for (int i = 0; i < rnum; i++) {
            const auto& ev = events[i];

            if (ev.events != 0) {
                static_cast<Watch*>(ev.data.ptr)->_notify(ev.events);
            }
        }
    }
}


void EventLoop::_register(int fd, Watch& watch, uint32_t epoll_events)
{
    if (m_epoll_fd == -1)
        return;
    struct epoll_event ev = {};
    ev.events = epoll_events;
    ev.data.ptr = &watch;
    int rc = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (rc == -1) {
        log::error("EventLoop: epoll_ctl(ADD): {m}");
    }
}


void EventLoop::_unregister(int fd, Watch& watch)
{
    if (m_epoll_fd == -1)
        return;
    int rc = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    if (rc == -1 && errno != EBADF) {
        log::error("EventLoop: epoll_ctl(DEL): {m}");
    }
}


} // namespace xci::core
