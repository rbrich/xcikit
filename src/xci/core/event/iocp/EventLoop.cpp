// iocp/EventLoop.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EventLoop.h"
#include <xci/core/log.h>

#include <cassert>

namespace xci::core {


EventLoop::EventLoop()
{
    m_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
            nullptr,
            (ULONG_PTR) 0,
            1);
    if (m_port == nullptr) {
        log_error("EventLoop: CreateIoCompletionPort: {mm}");
        return;
    }
}


EventLoop::~EventLoop()
{
    if (m_port != nullptr)
        CloseHandle(m_port);
}


void EventLoop::run()
{
    for (;;) {
        DWORD num_bytes_transferred = 0;
        ULONG_PTR completion_key = 0;
        LPOVERLAPPED overlapped = nullptr;
        BOOL res = GetQueuedCompletionStatus(m_port,
                &num_bytes_transferred,
                &completion_key,
                &overlapped,
                INFINITE);
        if (!res) {
            log_error("EventLoop: GetQueuedCompletionStatus: {mm}");
            return;
        }

        // check terminate() event
        if (completion_key == 0) {
            assert(overlapped == nullptr);
            break;
        }

        // notify event watcher
        reinterpret_cast<Watch*>(completion_key)->_notify(overlapped);
    }
}


void EventLoop::terminate()
{
    PostQueuedCompletionStatus(m_port, 0, 0, nullptr);
}


bool EventLoop::_associate(HANDLE handle, Watch& watch)
{
    auto ret = CreateIoCompletionPort(handle,m_port,
            (ULONG_PTR) &watch,0);
    if (!ret) {
        log_error("EventLoop: CreateIoCompletionPort(associate): {mm}");
        return false;
    }
    return true;
}


void EventLoop::_post(Watch& watch, LPOVERLAPPED overlapped)
{
    PostQueuedCompletionStatus(m_port, 0,
            (ULONG_PTR) &watch, overlapped);
}


}  // namespace xci::core
