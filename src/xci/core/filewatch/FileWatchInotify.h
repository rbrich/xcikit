// FileWatchInotify.h created on 2018-04-14, part of XCI toolkit
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

#ifndef XCI_CORE_FILEWATCH_INOTIFY_H
#define XCI_CORE_FILEWATCH_INOTIFY_H

#include <xci/core/FileWatch.h>
#include <list>
#include <thread>
#include <map>
#include <mutex>

namespace xci::core {


class FileWatchInotify: public FileWatch {
public:
    FileWatchInotify();
    ~FileWatchInotify() override;

    int add_watch(const std::string& filename, Callback cb) override;
    void remove_watch(int handle) override;

private:
    void remove_watch_nolock(int handle);
    void handle_event(int wd, uint32_t mask, const std::string& name);

private:
    int m_inotify_fd;
    int m_quit_fd;
    std::thread m_thread;
    std::mutex m_mutex;
    int m_next_handle = 0;

    struct Watch {
        int handle;
        std::string dir;  // directory containing the file
        std::string name;  // filename without dir part
        Callback cb;
    };
    std::list<Watch> m_watch;

    struct Dir {
        std::string dir;  // watched directory
        int wd;  // inotify watch descriptor
    };
    std::list<Dir> m_dir;
};


} // namespace xci::core

#endif // XCI_CORE_FILEWATCH_INOTIFY_H
