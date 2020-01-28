// iocp/TimerWatch.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
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
    m_loop._remove_timer(*this);
}


void TimerWatch::restart()
{
    m_loop._add_timer(m_interval, *this);
}


void TimerWatch::_notify(LPOVERLAPPED)
{
    if (m_cb)
        m_cb();
    if (m_type == Periodic)
        restart();
}


} // namespace xci::core
