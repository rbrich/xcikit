// epoll/IOWatch.cpp created on 2019-03-29, part of XCI toolkit
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

#include "IOWatch.h"

namespace xci::core {


IOWatch::IOWatch(EventLoop& loop, int fd, Flags flags, Callback cb)
        : Watch(loop), m_fd(fd), m_cb(std::move(cb))
{
    uint32_t pevents = 0;
    if (flags & Read)  pevents |= EPOLLIN;
    if (flags & Write) pevents |= EPOLLOUT;
    loop._register(fd, *this, pevents);
}


IOWatch::~IOWatch()
{
    m_loop._unregister(m_fd, *this);
}


void IOWatch::_notify(uint32_t epoll_events)
{
    // translate events
    if (epoll_events & EPOLLIN)
        m_cb(m_fd, Event::Read);
    if (epoll_events & EPOLLOUT)
        m_cb(m_fd, Event::Write);
    if (epoll_events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP))
        m_cb(m_fd, Event::Error);
}


} // namespace xci::core
