// EventWatch.cpp created on 2019-03-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EventWatch.h"
#include <xci/core/log.h>

#include <cassert>
#include <unistd.h>
#include <sys/event.h>

namespace xci::core {


EventWatch::EventWatch(EventLoop& loop, Callback cb)
        : Watch(loop), m_cb(std::move(cb))
{
    struct kevent kev = {};
    EV_SET(&kev, (uintptr_t) this, EVFILT_USER, EV_ADD, NOTE_FFNOP, 0, this);
    if (!m_loop._kevent(kev)) {
        log::error("EventWatch: kevent(EV_ADD): {m}");
    }
}


EventWatch::~EventWatch()
{
    struct kevent kev = {};
    EV_SET(&kev, (uintptr_t) this, EVFILT_USER, EV_DELETE, 0, 0, this);
    if (!m_loop._kevent(kev)) {
        log::error("EventWatch: kevent(EV_DELETE): {m}");
    }
}


void EventWatch::fire()
{
    struct kevent kev = {};
    EV_SET(&kev, (uintptr_t) this, EVFILT_USER, EV_ENABLE, NOTE_TRIGGER, 0, this);
    if (!m_loop._kevent(kev)) {
        log::error("EventWatch: kevent(EV_ENABLE): {m}");
    }
}


void EventWatch::_notify(const struct kevent& event)
{
    assert((uintptr_t) this == event.ident);
    struct kevent kev = {};
    EV_SET(&kev, (uintptr_t) this, EVFILT_USER, EV_DISABLE, 0, 0, this);
    if (!m_loop._kevent(kev)) {
        log::error("EventWatch: kevent(EV_DISABLE): {m}");
    }

    m_cb();
}


} // namespace xci::core
