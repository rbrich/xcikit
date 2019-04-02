// epoll/TimerWatch.cpp created on 2019-04-01, part of XCI toolkit
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

#include "TimerWatch.h"
#include <xci/core/log.h>

#include <sys/timerfd.h>
#include <unistd.h>

namespace xci::core {

using namespace std::chrono;


TimerWatch::TimerWatch(EventLoop& loop, milliseconds interval,
                           Type type, Callback cb)
        : Watch(loop), m_interval(interval), m_type(type), m_cb(std::move(cb))
{
    m_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (m_timer_fd == -1) {
        log_error("TimerWatch: timerfd_create: {m}");
        return;
    }
    m_loop._register(m_timer_fd, *this, EPOLLIN);
    restart();
}


TimerWatch::~TimerWatch()
{
    if (m_timer_fd != -1) {
        m_loop._unregister(m_timer_fd, *this);
        ::close(m_timer_fd);
    }
}


void TimerWatch::stop()
{
    struct itimerspec value = {};
    if (timerfd_settime(m_timer_fd, 0, &value, nullptr) == -1) {
        log_error("TimerWatch: timerfd_settime(<reset>): {m}");
    }
}


void TimerWatch::restart()
{
    struct itimerspec value = {};
    auto secs = duration_cast<seconds>(m_interval);
    value.it_value.tv_sec = secs.count();
    value.it_value.tv_nsec = duration_cast<nanoseconds>(m_interval - secs).count();

    if (m_type == Type::Periodic) {
        value.it_interval = value.it_value;
    }

    if (timerfd_settime(m_timer_fd, 0, &value, nullptr) == -1) {
        log_error("TimerWatch: timerfd_settime(<reset>): {m}");
    }
}


void TimerWatch::_notify(uint32_t epoll_events)
{
    uint64_t value;
    ssize_t readlen = ::read(m_timer_fd, &value, sizeof(value));
    if (readlen < 0) {
        log_error("TimerWatch: read: {m}");
        return;
    }
    if (value > 0 && m_cb)
        m_cb();
}


} // namespace xci::core
