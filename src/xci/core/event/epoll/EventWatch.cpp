// epoll/EventWatch.cpp created on 2019-03-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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
        log::error("EventWatch: eventfd: {m}");
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
    while (::write(m_fd, &value, sizeof(value)) == -1 && errno == EINTR);
}


void EventWatch::_notify(uint32_t epoll_events)
{
    uint64_t value;
    ssize_t readlen = ::read(m_fd, &value, sizeof(value));
    if (readlen < 0) {
        log::error("EventWatch: read: {m}");
        return;
    }
    if (value > 0)
        m_cb();
}


} // namespace xci::core
