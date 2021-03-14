// TimerWatch.cpp created on 2019-04-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TimerWatch.h"
#include <xci/core/log.h>

namespace xci::core {


TimerWatch::TimerWatch(EventLoop& loop, std::chrono::milliseconds interval, Type type,
                       TimerWatch::Callback cb)
    : Watch(loop), m_interval(interval), m_type(type), m_cb(std::move(cb))
{
    restart();
}


void TimerWatch::stop()
{
    struct kevent kev = {};
    EV_SET(&kev, (uintptr_t)this, EVFILT_TIMER, EV_DELETE, 0, 0, this);
    if (!m_loop._kevent(kev)) {
        log::error("TimerWatch: kevent(EVFILT_TIMER, EV_DELETE): {m}");
        return;
    }
}


void TimerWatch::restart()
{
    struct kevent kev = {};
    EV_SET(&kev, (uintptr_t)this, EVFILT_TIMER, EV_ADD,
           (m_type == Type::OneShot) ? EV_ONESHOT : 0, m_interval.count(), this);
    if (!m_loop._kevent(kev)) {
        log::error("TimerWatch: kevent(EVFILT_TIMER, EV_ADD): {m}");
        return;
    }
}


void TimerWatch::_notify(const struct kevent& event)
{
    if (m_cb)
        m_cb();
}


} // namespace xci::core
