// FileWatchKqueue.h created on 2018-04-14, part of XCI toolkit
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

#ifndef XCIKIT_FILEWATCHKQUEUE_H
#define XCIKIT_FILEWATCHKQUEUE_H

#include <xci/core/FileWatch.h>
#include <thread>
#include <mutex>
#include <map>
#include <list>

namespace xci::core {


class FileWatchKqueue: public FileWatch {
public:
    FileWatchKqueue();
    ~FileWatchKqueue() override;

    // Watch file `filename` for changes (content modified or file deleted)
    // and call `cb` when such event occurs. Note that the callback might be called
    // in another thread context.
    // Returns watch handle on success, -1 on error.
    int add_watch(const std::string& filename, Callback cb) override;
    void remove_watch(int handle) override;

private:
    void remove_watch_nolock(int handle);
    int register_kevent(const std::string& path, uint32_t fflags);
    void unregister_kevent(int fd);
    void handle_event(int fd, uint32_t fflags);

private:
    int m_kqueue_fd;  // inotify or kqueue FD
    int m_quit_pipe[2];
    std::thread m_thread;
    std::mutex m_mutex;

    struct Watch {
        int fd;
        std::string dir;  // directory containing the file
        std::string name;  // filename without dir part
        Callback cb;
    };
    std::list<Watch> m_watch;

    struct Dir {
        int fd;
        std::string dir;  // watched directory
    };
    std::list<Dir> m_dir;
};


} // namespace xci::core



#endif // XCIKIT_FILEWATCHKQUEUE_H
