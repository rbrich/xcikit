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

#include <xci/util/FileWatch.h>

namespace xci {
namespace util {


class FileWatchKqueue: public FileWatch {
public:
    FileWatchKqueue();
    ~FileWatchKqueue();

    // Watch file `filename` for changes (content modified or file deleted)
    // and call `cb` when such event occurs. Note that the callback might be called
    // in another thread context.
    // Returns watch handle on success, -1 on error.
    int add_watch(const std::string& filename, Callback cb) override;
    void remove_watch(int handle) override;

private:
    int m_queue_fd;  // inotify or kqueue FD
    int m_quit_pipe[2];
    std::thread m_thread;
    std::mutex m_mutex;
    std::map<int, Callback> m_callback;
};


}} // namespace xci::util



#endif // XCIKIT_FILEWATCHKQUEUE_H
