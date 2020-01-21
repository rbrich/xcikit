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
{

}


EventWatch::~EventWatch()
{

}


void EventWatch::fire()
{

}


void EventWatch::_notify(const struct kevent& event)
{

}


} // namespace xci::core
