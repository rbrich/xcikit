// EventLoop.cpp created on 2018-04-14, part of XCI toolkit
// Copyright 2018 Radek Brich
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
#include <xci/core/file.h>
#include <xci/core/log.h>

#include <vector>
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
        log_error("EventLoop: kqueue: {m}");
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
            log_error("EventLoop: kevent(): {m}");
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
    return ::kevent(m_kqueue_fd, evlist, nevents, nullptr, 0, nullptr) != -1;
}


void EventLoop::terminate()
{
    ::close(m_kqueue_fd);
    m_kqueue_fd = -1;
}


}  // namespace xci::core
