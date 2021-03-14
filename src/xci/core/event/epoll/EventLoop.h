// epoll/EventLoop.h created on 2019-03-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_EPOLL_EVENTLOOP_H
#define XCI_CORE_EPOLL_EVENTLOOP_H

#include <vector>
#include <sys/epoll.h>

namespace xci::core {


class EventLoop;


class Watch {
public:
    explicit Watch(EventLoop& loop) : m_loop(loop) {}
    virtual ~Watch() = default;

    // -------------------------------------------------------------------------
    // Methods called by EventLoop

    virtual void _notify(uint32_t epoll_events) = 0;

protected:
    EventLoop& m_loop;
};


/// System event loop. Uses Linux epoll(7) API.
/// Generally not thread-safe, inter-thread signalling can be implemented using EventWatch.

class EventLoop {
public:

    EventLoop();
    ~EventLoop();

    /// Start the event loop.
    /// Blocks until explicitly terminated (see `terminate()`).
    void run();

    /// Terminate running loop.
    /// This is not thread-safe by itself. It has to be run from an event callback
    /// (because the run() method blocks current thread and gives back control only
    /// through the callbacks). Use in combination with EventWatch to
    /// implement thread-save termination.
    void terminate() { m_terminate = true; }

    // -------------------------------------------------------------------------
    // Methods called by Watch sub-classes

    void _register(int fd, Watch& watch, uint32_t epoll_events);
    void _unregister(int fd, Watch& watch);

private:
    int m_epoll_fd;
    bool m_terminate = false;
};


} // namespace xci::core

#endif // include guard
