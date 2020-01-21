// iocp/SignalWatch.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
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

}


SignalWatch::~SignalWatch()
{

}


void SignalWatch::_notify(const struct kevent& event)
{
    if (m_cb)
        m_cb(event.ident);
}


} // namespace xci::core
