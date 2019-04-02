// epoll/TimerWatch.h created on 2019-04-01, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
