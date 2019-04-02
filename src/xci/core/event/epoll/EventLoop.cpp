// epoll/EventLoop.cpp created on 2019-03-26, part of XCI toolkit
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
        log_error("EventLoop: epoll_create1: {m}");
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
            if (errno == EBADF)
                break;  // epoll is closed (terminate() was called)
            log_error("EventLoop: poll: {m}");
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
        log_error("EventLoop: epoll_ctl(ADD): {m}");
    }
}


void EventLoop::_unregister(int fd, Watch& watch)
{
    if (m_epoll_fd == -1)
        return;
    int rc = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    if (rc == -1 && errno != EBADF) {
        log_error("EventLoop: epoll_ctl(DEL): {m}");
    }
}


} // namespace xci::core
