// IOWatch.cpp created on 2019-03-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "IOWatch.h"
#include <xci/core/log.h>

namespace xci::core {


IOWatch::IOWatch(EventLoop& loop, int fd, Flags flags, Callback cb)
    : Watch(loop), m_fd(fd), m_cb(std::move(cb))
{
    size_t nevents = 0;
    struct kevent kev[2] = {};
    if (flags & Read) {
        EV_SET(&kev[nevents++], fd, EVFILT_READ, EV_ADD, 0, 0, this);
    }
    if (flags & Write) {
        EV_SET(&kev[nevents++], fd, EVFILT_WRITE, EV_ADD, 0, 0, this);
    }
    if (!m_loop._kevent(kev, nevents)) {
        log::error("IOWatch: kevent: {m}");
    }
}


IOWatch::~IOWatch()
{
    const size_t nevents = 2;
    struct kevent kev[nevents] = {};
    EV_SET(&kev[0], m_fd, EVFILT_READ, EV_DELETE, 0, 0, this);
    EV_SET(&kev[1], m_fd, EVFILT_WRITE, EV_DELETE, 0, 0, this);
    if (!m_loop._kevent(kev, nevents)) {
        if (errno != EBADF)  // event already removed
            log::error("IOWatch: kevent: {m}");
    }
}


void IOWatch::_notify(const struct kevent& event)
{
    // NOTE: event.ident is actually an FD (int) we passed above
    if (event.filter == EVFILT_READ && (event.flags & EV_EOF) != EV_EOF)
        m_cb(int(event.ident), Event::Read);
    if (event.filter == EVFILT_WRITE && (event.flags & EV_EOF) != EV_EOF)
        m_cb(int(event.ident), Event::Write);
    if (event.flags & EV_EOF)
        m_cb(int(event.ident), Event::Error);
}


} // namespace xci::core
