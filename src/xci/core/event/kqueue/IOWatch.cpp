// IOWatch.cpp created on 2019-03-29, part of XCI toolkit
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
        log_error("IOWatch: kevent: {m}");
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
            log_error("IOWatch: kevent: {m}");
    }
}


void IOWatch::_notify(const struct kevent& event)
{
    if (event.filter == EVFILT_READ && (event.flags & EV_EOF) != EV_EOF)
        m_cb(event.ident, Event::Read);
    if (event.filter == EVFILT_WRITE && (event.flags & EV_EOF) != EV_EOF)
        m_cb(event.ident, Event::Write);
    if (event.flags & EV_EOF)
        m_cb(event.ident, Event::Error);
}


} // namespace xci::core
