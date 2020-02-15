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


/// `SignalWatch` implementation for IOCP using signal() handler
/// This uses static storage, so there can be only a single
/// instance per process.

class SignalWatch: public Watch {
public:
    using Callback = std::function<void(int signo)>;

    /// Watch for Unix signal.
    /// \param cb       Called in reaction to matched wake() call
    SignalWatch(EventLoop& loop, std::initializer_list<int> signums, Callback cb);
    ~SignalWatch() override;

    void _notify(LPOVERLAPPED overlapped) override;

    EventLoop& _loop() const { return m_loop; }

private:
    std::vector<int> m_signals;
    Callback m_cb;
};


} // namespace xci::core

#endif // include guard
