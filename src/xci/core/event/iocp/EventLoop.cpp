// iocp/EventLoop.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EventLoop.h"
#include <xci/core/log.h>

#include <cassert>
#include <algorithm>

namespace xci::core {

using namespace std::chrono;


EventLoop::EventLoop()
{
    m_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
            nullptr,
            (ULONG_PTR) 0,
            1);
    if (m_port == nullptr) {
        log::error("EventLoop: CreateIoCompletionPort: {m:l}");
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
                next_timeout());
        if (!res && GetLastError() == WAIT_TIMEOUT)
            continue;
        if (!res) {
            log::error("EventLoop: GetQueuedCompletionStatus: {m:l}");
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
        log::error("EventLoop: CreateIoCompletionPort(associate): {m:l}");
        return false;
    }
    return true;
}


void EventLoop::_post(Watch& watch, LPOVERLAPPED overlapped)
{
    PostQueuedCompletionStatus(m_port, 0,
            (ULONG_PTR) &watch, overlapped);
}


void EventLoop::_add_timer(std::chrono::milliseconds timeout, Watch& watch)
{
    auto t = steady_clock::now() + timeout;
    m_timer_queue.emplace_back(Timer{t, &watch});
    std::sort(m_timer_queue.begin(), m_timer_queue.end());
}


void EventLoop::_remove_timer(Watch& watch)
{
    m_timer_queue.erase(
            std::remove_if(
                    m_timer_queue.begin(), m_timer_queue.end(),
                    [&watch](const Timer& t) { return t.watch == &watch; }),
            m_timer_queue.end());
}


DWORD EventLoop::next_timeout()
{
    if (m_timer_queue.empty())
        return INFINITE;

    auto now = steady_clock::now();
    for(;;) {
        const auto& front = m_timer_queue.front();
        if (front.time_point <= now) {
            // remove expired Timer before notifying it to allow the callback
            // to add new Timer to the queue (i.e. restart)
            auto* watch = front.watch;
            m_timer_queue.pop_front();
            watch->_notify(nullptr);
        } else
            break;
        if (m_timer_queue.empty())
            return INFINITE;
    }
    return duration_cast<milliseconds>(
            m_timer_queue.front().time_point - now).count();
}


}  // namespace xci::core
