// epoll/IOWatch.cpp created on 2019-03-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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
