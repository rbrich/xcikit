// epoll/SignalWatch.h created on 2019-03-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_EPOLL_SIGNALWATCH_H
#define XCI_CORE_EPOLL_SIGNALWATCH_H

#include "EventLoop.h"
#include <functional>

namespace xci::core {


/// `SignalWatch` implementation for epoll(7) using signalfd(2)

class SignalWatch: public Watch {
public:
    using Callback = std::function<void(int signo)>;

    /// Watch for Unix signal.
    /// \param cb       Called in reaction to matched wake() call
    SignalWatch(EventLoop& loop, std::initializer_list<int> signums, Callback cb);

    ~SignalWatch() override;

    void _notify(uint32_t epoll_events) override;

private:
    int m_fd;
    Callback m_cb;
};


} // namespace xci::core

#endif // include guard
