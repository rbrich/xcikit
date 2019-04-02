// epoll/IOWatch.h created on 2019-03-29, part of XCI toolkit
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

#ifndef XCI_CORE_EPOLL_IOWATCH_H
#define XCI_CORE_EPOLL_IOWATCH_H

#include "EventLoop.h"
#include <functional>

namespace xci::core {


/// `IOWatch` implementation for epoll(7) using EPOLLIN / EPOLLOUT events

class IOWatch: public Watch {
public:

    using Flags = unsigned int;
    static const Flags Read = (1 << 0);
    static const Flags Write = (1 << 1);

    enum class Event {
        Read,   ///< data available to read
        Write,  ///< ready to write
        Error,  ///< error condition (FD closed etc.)
    };

    using Callback = std::function<void(int fd, Event event)>;

    /// Start watching `events` on `fd`, call `cb` on event.
    /// Note that there are no checks if the FD is already watched.
    /// Don't register same FD multiple times.
    /// \param  fd      FD to be watched
    /// \param  flags   IOFlags (ORed)
    /// \param  cb      called when an event occurs
    IOWatch(EventLoop& loop, int fd, Flags flags, Callback cb);
    ~IOWatch() override;

    void _notify(uint32_t epoll_events) override;

private:
    int m_fd;
    Callback m_cb;
};


} // namespace xci::core

#endif // include guard
