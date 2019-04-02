// EventWatch.h created on 2019-03-29, part of XCI toolkit
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

#ifndef XCI_CORE_KQUEUE_EVENTWATCH_H
#define XCI_CORE_KQUEUE_EVENTWATCH_H

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
