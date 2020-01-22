// iocp/EventWatch.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EventWatch.h"
#include <xci/core/log.h>


namespace xci::core {


EventWatch::EventWatch(EventLoop& loop, Callback cb)
    : Watch(loop), m_cb(std::move(cb))
{}


void EventWatch::fire()
{
    m_loop._post(*this, nullptr);
}


void EventWatch::_notify(LPOVERLAPPED)
{
    m_cb();
}


} // namespace xci::core
