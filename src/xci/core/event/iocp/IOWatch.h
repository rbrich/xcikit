// iocp/IOWatch.h created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_IOCP_IOWATCH_H
#define XCI_CORE_IOCP_IOWATCH_H

#include "EventLoop.h"
#include <functional>

namespace xci::core {


/// Watch for read/write events

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

    void _notify(LPOVERLAPPED overlapped) override;

private:
    int m_fd;
    Callback m_cb;
};


} // namespace xci::core

#endif // include guard
