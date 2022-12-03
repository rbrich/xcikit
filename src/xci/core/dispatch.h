// dispatch.h created on 2018-03-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_DISPATCH_H
#define XCI_CORE_DISPATCH_H

#include "event.h"
#include <string>
#include <functional>
#include <memory>
#include <thread>

namespace xci::core {


/// EventLoop in a thread

class Dispatch {
public:
    Dispatch();
    ~Dispatch();

    EventLoop& loop() { return m_loop; }

    void terminate() { m_quit_event.fire(); }

private:
    std::thread m_thread;
    EventLoop m_loop;
    EventWatch m_quit_event {m_loop, [this]{ m_loop.terminate(); }};
};


/// Convenient Dispatch thread with embedded FSWatch.
/// This may be used for auto-reloading of resource files.

class FSDispatch: private Dispatch {
public:
    using Event = FSWatch::Event;
    using Callback = FSWatch::PathCallback;

    /// Watch file for changes and run a callback when an event occurs.
    /// It's possible to add more than one callback for the same `filename`.
    /// Note that the callback might be called from another thread.
    /// \param pathname File to be watched.
    /// \param cb       Callback function called for each event.
    /// \return         New watch handle on success, -1 on error.
    bool add_watch(const fs::path& pathname, Callback cb);

    /// Remove previously added watch. Does nothing for handle -1.
    /// In case the same file has multiple callbacks installed, this removes
    /// just the one identified by `handle`.
    /// \param handle Handle to the watch as returned from add_watch.
    bool remove_watch(const fs::path& pathname);

private:
    FSWatch m_fs_watch { loop() };
};


using FSDispatchPtr = std::shared_ptr<FSDispatch>;


}  // namespace xci::core

#endif // include guard
