// SignalWatch.h created on 2019-03-30, part of XCI toolkit
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

#ifndef XCI_CORE_KQUEUE_SIGNALWATCH_H
#define XCI_CORE_KQUEUE_SIGNALWATCH_H

#include "EventLoop.h"
#include <functional>
#include <vector>

namespace xci::core {


/// `SignalWatch` implementation for kqueue(2) using EVFILT_SIGNAL

class SignalWatch: public Watch {
public:
    using Callback = std::function<void(int signo)>;

    /// Watch for Unix signal.
    /// \param cb       Called in reaction to matched wake() call
    SignalWatch(EventLoop& loop, std::initializer_list<int> signums, Callback cb);
    ~SignalWatch() override;

    void _notify(const struct kevent& event) override;

private:
    struct Signal {
        int signum;
        sig_t func;
    };
    std::vector<Signal> m_signals;
    Callback m_cb;
};


} // namespace xci::core

#endif // include guard
