// EventLoop.cpp created on 2018-04-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EventLoop.h"
#include <xci/core/file.h>
#include <xci/core/log.h>

#include <cassert>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

namespace xci::core {


EventLoop::EventLoop()
{
    m_kqueue_fd = kqueue();
    if (m_kqueue_fd == -1) {
        log::error("EventLoop: kqueue: {m}");
        return;
    }
}


EventLoop::~EventLoop()
{
    if (m_kqueue_fd != -1)
        ::close(m_kqueue_fd);
}


void EventLoop::run()
{
    constexpr size_t ke_size = 10;
    struct kevent ke[ke_size] = {};
    for (;;) {
        int rc = kevent(m_kqueue_fd, nullptr, 0, ke, ke_size, nullptr);
        if (rc == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EBADF)
                break;  // kqueue is closed (terminate() was called)
            log::error("EventLoop: kevent(): {m}");
            break;
        }
        for (int i = 0; i < rc; i++) {
            const auto& ev = ke[i];
            static_cast<Watch*>(ev.udata)->_notify(ev);
        }
    }
}


bool EventLoop::_kevent(struct kevent* evlist, size_t nevents)
{
    if (m_kqueue_fd == -1)
        return true;
    return ::kevent(m_kqueue_fd, evlist, int(nevents), nullptr, 0, nullptr) != -1;
}


void EventLoop::terminate()
{
    ::close(m_kqueue_fd);
    m_kqueue_fd = -1;
}


}  // namespace xci::core
