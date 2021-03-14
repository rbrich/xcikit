// epoll/TimerWatch.h created on 2019-04-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_EPOLL_TIMERWATCH_H
#define XCI_CORE_EPOLL_TIMERWATCH_H

#include "EventLoop.h"
#include <functional>
#include <chrono>

namespace xci::core {


/// Run callback periodically
/// `TimerWatch` implementation for kqueue(2) using EVFILT_TIMER

class TimerWatch: public Watch {
public:
    using Callback = std::function<void()>;
    enum Type { Periodic, OneShot };

    /// Creates new monotonic timer, which will start immediately.
    /// \param  loop        EventLoop which will run the timer
    /// \param  interval    Timer will trigger after this interval expires
    /// \param  type        Periodic timer restarts automatically. OneShot timer has to be restarted
    ///                     by calling `restart()` explicitly
    /// \param  cb          The callback run by timer.
    TimerWatch(EventLoop& loop, std::chrono::milliseconds interval, Type type, Callback cb);
    TimerWatch(EventLoop& loop, std::chrono::milliseconds interval, Callback cb)
            : TimerWatch(loop, interval, Type::Periodic, std::move(cb)) {}
    ~TimerWatch() override;

    void stop();
    void restart();

    void _notify(uint32_t epoll_events) override;

private:
    int m_timer_fd;
    std::chrono::milliseconds m_interval;
    Type m_type;
    Callback m_cb;
};


} // namespace xci::core



#endif // include guard
