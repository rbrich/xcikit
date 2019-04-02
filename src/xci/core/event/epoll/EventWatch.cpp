// epoll/EventWatch.cpp created on 2019-03-29, part of XCI toolkit
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

#include "EventWatch.h"
#include <xci/core/log.h>

#include <unistd.h>
#include <sys/eventfd.h>

namespace xci::core {


EventWatch::EventWatch(EventLoop& loop, Callback cb)
        : Watch(loop), m_cb(std::move(cb))
{
    m_fd = eventfd(0, 0);
    if (m_fd == -1) {
        log_error("EventWatch: eventfd: {m}");
        return;
    }

    loop._register(m_fd, *this, EPOLLIN);
}


EventWatch::~EventWatch()
{
    if (m_fd != -1) {
        m_loop._unregister(m_fd, *this);
        ::close(m_fd);
    }
}


void EventWatch::fire()
{
    uint64_t value = 1;
    ::write(m_fd, &value, sizeof(value));
}


void EventWatch::_notify(uint32_t epoll_events)
{
    uint64_t value;
    ssize_t readlen = ::read(m_fd, &value, sizeof(value));
    if (readlen < 0) {
        log_error("EventWatch: read: {m}");
        return;
    }
    if (value > 0)
        m_cb();
}


} // namespace xci::core
