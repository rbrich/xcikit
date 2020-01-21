// iocp/EventLoop.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EventLoop.h"
#include <xci/core/file.h>
#include <xci/core/log.h>

namespace xci::core {


EventLoop::EventLoop()
{
    m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, (ULONG_PTR) 0, 0);
    if (m_iocp == nullptr) {
        log_error("EventLoop: CreateIoCompletionPort: {m}");
        return;
    }
}


EventLoop::~EventLoop()
{
    CloseHandle(m_iocp);
}


void EventLoop::run()
{

}


bool EventLoop::_kevent(struct kevent* evlist, size_t nevents)
{

}


void EventLoop::terminate()
{

}


}  // namespace xci::core
