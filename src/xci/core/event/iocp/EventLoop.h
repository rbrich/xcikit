// iocp/EventLoop.h created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_IOCP_EVENTLOOP_H
#define XCI_CORE_IOCP_EVENTLOOP_H


namespace xci::core {


class EventLoop;


class Watch {
public:
    explicit Watch(EventLoop& loop) : m_loop(loop) {}
    virtual ~Watch() = default;

    // -------------------------------------------------------------------------
    // Methods called by EventLoop

    virtual void _notify(const struct kevent& event) = 0;

protected:
    EventLoop& m_loop;
};


/// System event loop. Uses BSD kqueue(2) API.
/// Generally not thread-safe, inter-thread signalling can be implemented using EventWatch.

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

    // register / unregister / fire event (depending on flags inside struct kevent)
    bool _kevent(struct kevent& event) { return _kevent(&event, 1); }
    bool _kevent(struct kevent* evlist, size_t nevents);

private:
    HANDLE m_iocp;
};


} // namespace xci::core

#endif // include guard
