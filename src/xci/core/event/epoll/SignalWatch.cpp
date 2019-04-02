// epoll/SignalWatch.cpp created on 2019-03-29, part of XCI toolkit
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

#include "SignalWatch.h"
#include <xci/core/log.h>

#include <csignal>
#include <cassert>
#include <unistd.h>
#include <sys/signalfd.h>

namespace xci::core {


SignalWatch::SignalWatch(EventLoop& loop, std::initializer_list<int> signums,
                             Callback cb)
        : Watch(loop), m_cb(std::move(cb))
{
    sigset_t sigset;
    sigemptyset(&sigset);
    for (int sig: signums)
        sigaddset(&sigset, sig);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
    m_fd = signalfd(-1, &sigset, 0);
    if (m_fd == -1) {
        log_error("SignalWatch: signalfd: {m}");
        return;
    }
    m_loop._register(m_fd, *this, EPOLLIN);
}


SignalWatch::~SignalWatch()
{
    if (m_fd != -1) {
        m_loop._unregister(m_fd, *this);
        ::close(m_fd);
    }
}


void SignalWatch::_notify(uint32_t epoll_events)
{
    if (epoll_events & EPOLLIN) {
        signalfd_siginfo si = {};
        ssize_t readlen = read(m_fd, &si, sizeof(si));
        if (readlen < 0) {
            log_error("SignalWatch: read: {m}");
            return;
        }

        assert(readlen == sizeof(si));
        if (m_cb) {
            m_cb(si.ssi_signo);
        }
    }
}


} // namespace xci::core
