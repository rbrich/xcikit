// SignalWatch.cpp created on 2019-03-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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
            log::error("SignalWatch: signal(%d, SIG_IGN): {m}", sig);
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
        log::error("SignalWatch: kevent: {m}");
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
            log::error("SignalWatch: kevent: {m}");
    }

    // unblock signals
    for (const auto& sig: m_signals) {
        if (signal(sig.signum, sig.func) == SIG_ERR) {
            log::error("SignalWatch: signal({}, <orig>): {m}", sig.signum);
        }
    }
}


void SignalWatch::_notify(const struct kevent& event)
{
    if (m_cb)
        m_cb(int(event.ident));
}


} // namespace xci::core
