// iocp/EventWatch.h created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_IOCP_EVENTWATCH_H
#define XCI_CORE_IOCP_EVENTWATCH_H

#include "EventLoop.h"
#include <functional>

namespace xci::core {


/// `EventWatch` implementation for kqueue(2) using EVFILT_USER

class EventWatch: public Watch {
public:

    //using EventId = uint64_t;
    using Callback = std::function<void()>;

    /// Watch for wake event.
    /// \param cb       Called in reaction to matched wake() call
    /// \return         New watch handle on success, -1 on error.
    EventWatch(EventLoop& loop, Callback cb);
    ~EventWatch() override;

    /// Fire the event. Wakes running loop.
    ///
    /// This method is THREAD-SAFE. It can be used for inter-thread
    /// synchronization, i.e. thread A runs the loop, thread B sends the wake.
    /// The loop receives the wake event and runs registered callback (in thread A).
    void fire();

    void _notify(const struct kevent& event) override;

private:
    Callback m_cb;
};


} // namespace xci::core

#endif // include guard
