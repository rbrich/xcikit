// FSWatch.h created on 2019-03-29, part of XCI toolkit
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

#ifndef XCI_CORE_KQUEUE_FSWATCH_H
#define XCI_CORE_KQUEUE_FSWATCH_H

#include "EventLoop.h"
#include <string>
#include <list>
#include <map>
#include <functional>
#include <filesystem>

namespace xci::core {

namespace fs = std::filesystem;


/// Kqueue based filesystem watcher (using EVFILT_VNODE)
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
    /// Note that this might add watch for parent directory, which will trigger
    /// events in main Callback.
    /// \param pathname File or directory to be watched.
    /// \param cb       Callback function called for each event.
    bool add(const fs::path& pathname, PathCallback cb);

    /// Stop watching file or directory.
    /// \param pathname File or directory to remove. Must be same path as given to `add`.
    bool remove(const fs::path& pathname);

    void _notify(const struct kevent& event) override;

private:
    int register_kevent(const fs::path& path, uint32_t fflags, bool no_exist_ok=false);
    void unregister_kevent(int fd);

private:
    Callback m_main_cb;

    struct File {
        int fd;         // -1 if the file is watched but doesn't exist yet (kevent not registered)
        int dir_fd;
        fs::path name;  // filename without dir part
        PathCallback cb;
    };
    std::list<File> m_file;

    struct Dir {
        int fd;
        fs::path name;  // watched directory
    };
    std::list<Dir> m_dir;
};


} // namespace xci::core

#endif // include guard
