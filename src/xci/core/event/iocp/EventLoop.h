// iocp/EventLoop.h created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_IOCP_EVENTLOOP_H
#define XCI_CORE_IOCP_EVENTLOOP_H

#include <xci/compat/windows.h>
#include <chrono>
#include <deque>

namespace xci::core {

class EventLoop;


class Watch {
public:
    explicit Watch(EventLoop& loop) : m_loop(loop) {}
    virtual ~Watch() = default;

    // -------------------------------------------------------------------------
    // Methods called by EventLoop

    virtual void _notify(LPOVERLAPPED overlapped) = 0;

protected:
    EventLoop& m_loop;
};


/// System event loop. Uses Windows I/O Completion Ports API.
/// This implementation is single-threaded - the event loop runs
/// in a single thread. Do not try to run this in a threadpool.
/// Inter-thread signalling can be implemented using EventWatch.
///
/// References:
/// - https://docs.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    /// Start the event loop.
    /// Blocks until explicitly terminated (see `terminate()`).
    void run();

    /// Terminate running loop.
    /// Even though this specific implementation is thread-safe, this should not be considered
    /// thread-safe in general. Use with EventWatch for portable thread-safe termination.
    /// See also `EpollEventLoop::terminate()`.
    void terminate();

    // -------------------------------------------------------------------------
    // Methods called by Watch sub-classes

    bool _associate(HANDLE handle, Watch& watch);
    void _post(Watch& watch, LPOVERLAPPED overlapped);

    // Add the Watch to timer queue, notify it after `timeout` milliseconds.
    // (This emulates timers in IOCP, which is missing the feature.)
    void _add_timer(std::chrono::milliseconds timeout, Watch& watch);

    // Remove the Watch from timer queue.
    void _remove_timer(Watch& watch);

private:
    DWORD next_timeout();

    HANDLE m_port;

    struct Timer {
        std::chrono::steady_clock::time_point time_point;
        Watch* watch;

        bool operator<(const Timer& rhs) const {
            return time_point < rhs.time_point;
        }
    };
    std::deque<Timer> m_timer_queue;
};


} // namespace xci::core

#endif // include guard
