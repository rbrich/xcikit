// iocp/FSWatch.h created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_IOCP_FSWATCH_H
#define XCI_CORE_IOCP_FSWATCH_H

#include "EventLoop.h"
#include <string>
#include <list>
#include <map>
#include <functional>

namespace xci::core {


/// IOCP-based filesystem watcher (using ReadDirectoryChangesW)
class FSWatch: public Watch {
public:

    enum class Event {
        Create,     ///< File was created or moved in
        Delete,     ///< File was deleted or moved away
        Modify,     ///< File content was modified
        Attrib,     ///< File attributes were changed
        Stopped,    ///< The file is no longer watched (containing directory was deleted or moved)
    };

    using Callback = std::function<void(const std::string& pathname, Event event)>;
    using PathCallback = std::function<void(Event event)>;

    /// Watch file system for changes and run a callback when an event occurs.
    /// \param cb       Callback function called for any event.
    explicit FSWatch(EventLoop& loop, Callback cb = nullptr);
    ~FSWatch() override;

    /// Watch file or directory for changes and run a callback when an event occurs.
    /// It's not an error if the file does not exist (yet).
    /// \param pathname File or directory to be watched.
    /// \param cb       Callback function called for each event.
    bool add(const std::string& pathname, PathCallback cb);

    /// Stop watching file or directory.
    /// \param pathname File or directory to remove. Must be same path as given to `add`.
    bool remove(const std::string& pathname);

    void _notify(LPOVERLAPPED overlapped) override;

private:
    struct Dir;
    static bool _request_notification(Dir& dir);

private:
    Callback m_main_cb;

    struct File {
        HANDLE dir_h;
        std::string name;  // filename without dir part
        PathCallback cb;
    };
    std::list<File> m_file;

    // inherit OVERLAPPED to get a pointer to Dir from the notification
    struct Dir: public OVERLAPPED {
        Dir(HANDLE h, const std::string& name) : OVERLAPPED{}, h(h), name(name) {}  // NOLINT

        HANDLE h;   // directory handle (INVALID_HANDLE_VALUE => invalid record)
        std::string name;  // watched directory
        std::byte notif_buffer[4000] = {};

        bool is_invalid() const { return h == INVALID_HANDLE_VALUE; }
    };
    std::list<Dir> m_dir;
};


} // namespace xci::core

#endif // include guard
