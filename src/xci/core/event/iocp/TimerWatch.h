// iocp/TimerWatch.h created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_IOCP_TIMERWATCH_H
#define XCI_CORE_IOCP_TIMERWATCH_H

#include "EventLoop.h"
#include <functional>
#include <chrono>

namespace xci::core {


/// Run a callback periodically
/// IOCP-based timer (using the GetQueuedCompletionStatus' timeout parameter)
class TimerWatch: public Watch {
public:
    using Callback = std::function<void()>;
    enum Type { Periodic, OneShot };

    /// Creates new monotonic timer, which will start immediately.
    /// \param  loop        EventLoop which will run the timer
    /// \param  interval    Timer will trigger after this interval expires
    /// \param  type        Periodic timer restarts automatically.
    ///                     OneShot timer may be restarted with explicit `restart()`
    /// \param  cb          The callback run by timer.
    TimerWatch(EventLoop& loop, std::chrono::milliseconds interval, Type type, Callback cb);
    TimerWatch(EventLoop& loop, std::chrono::milliseconds interval, Callback cb)
        : TimerWatch(loop, interval, Type::Periodic, std::move(cb)) {}
    ~TimerWatch() override { stop(); }

    void stop();
    void restart();

    void _notify(LPOVERLAPPED) override;

private:
    std::chrono::milliseconds m_interval;
    Type m_type;
    Callback m_cb;
};


} // namespace xci::core

#endif // include guard
