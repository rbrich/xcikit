// dispatch.h created on 2018-03-30, part of XCI toolkit
// Copyright 2018 Radek Brich
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
