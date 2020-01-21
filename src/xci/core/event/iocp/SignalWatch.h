// iocp/SignalWatch.h created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_IOCP_SIGNALWATCH_H
#define XCI_CORE_IOCP_SIGNALWATCH_H

#include "EventLoop.h"
#include <functional>

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
