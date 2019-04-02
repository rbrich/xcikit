// SignalWatch.cpp created on 2019-03-30, part of XCI toolkit
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
#include <sys/event.h>

namespace xci::core {


SignalWatch::SignalWatch(EventLoop& loop, std::initializer_list<int> signums,
                         SignalWatch::Callback cb)
    : Watch(loop), m_cb(std::move(cb))
{
    // ignore signals for the set
    for (int sig: signums) {
        auto osig = signal(sig, SIG_IGN);
        if (osig == SIG_ERR) {
            log_error("SignalWatch: signal(%d, SIG_IGN): {m}", sig);
            return;
        }
        m_signals.push_back({sig, osig});
    }

    // monitor signals
    std::vector<struct kevent> kevs(signums.size());
    auto kev = kevs.begin();
    for (int sig: signums) {
        EV_SET(&*kev++, sig, EVFILT_SIGNAL, EV_ADD, 0, 0, this);
    }
    if (!m_loop._kevent(kevs.data(), kevs.size())) {
        log_error("SignalWatch: kevent: {m}");
        return;
    }
}


SignalWatch::~SignalWatch()
{
    // stop monitoring signals
    std::vector<struct kevent> kevs(m_signals.size());
    auto kev = kevs.begin();
    for (const auto& sig : m_signals) {
        EV_SET(&*kev++, sig.signum, EVFILT_SIGNAL, EV_DELETE, 0, 0, this);
    }
    if (!m_loop._kevent(kevs.data(), kevs.size())) {
        if (errno != EBADF)  // event already removed
            log_error("SignalWatch: kevent: {m}");
    }

    // unblock signals
    for (const auto& sig: m_signals) {
        if (signal(sig.signum, sig.func) == SIG_ERR) {
            log_error("SignalWatch: signal(%d, <orig>): {m}", sig.signum);
        }
    }
}


void SignalWatch::_notify(const struct kevent& event)
{
    if (m_cb)
        m_cb(event.ident);
}


} // namespace xci::core
