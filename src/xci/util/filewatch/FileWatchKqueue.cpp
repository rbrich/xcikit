// FileWatchKqueue.cpp created on 2018-04-14, part of XCI toolkit
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

#include "FileWatchKqueue.h"
#include <xci/util/log.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <fcntl.h>

namespace xci {
namespace util {


FileWatchKqueue::FileWatchKqueue()
{
    m_queue_fd = kqueue();
    if (m_queue_fd < 0 ) {
        log_error("FileWatchKqueue: kqueue: {:m}");
        return;
    }

    m_thread = std::thread([this]() {
        log_debug("FileWatchKqueue: starting");
        struct kevent ke;
        for (;;) {
            int rc = kevent(m_queue_fd, nullptr, 0, &ke, 1, nullptr);
            if (rc == -1) {
                if (errno == EBADF)
                    break;  // kqueue closed, quit
                log_error("FileWatchKqueue: kevent(): {:m}");
                break;
            }
            if (rc == 1 && ke.filter == EVFILT_VNODE) {
                std::lock_guard<std::mutex> lock_guard(m_mutex);
                auto& cb = m_callback[ke.ident];
                if (cb) {
                    if (ke.fflags & NOTE_WRITE)
                        cb(Event::Modify);
                    if (ke.fflags & NOTE_DELETE || ke.fflags & NOTE_RENAME) {
                        cb(Event::Delete);
                        // The file is gone, stop watching it
                        remove_watch((int) ke.ident);
                    }
                }
            }
        }
        log_debug("FileWatchKqueue: quit");
    });

    // Needed for Linux implementation but unused here (this avoids warning)
    (void) m_quit_pipe;
}


FileWatchKqueue::~FileWatchKqueue()
{
    // Cleanup
    ::close(m_queue_fd);
    m_thread.join();
}


int FileWatchKqueue::add_watch(const std::string& filename, std::function<void(Event)> cb)
{
    if (m_queue_fd < 0)
        return -1;

    int fd = ::open(filename.c_str(), O_EVTONLY);
    if (fd < 0) {
        log_error("FileWatchKqueue: open({}, O_EVTONLY): {:m}", filename.c_str());
        return -1;
    }

    struct kevent kev;
    kev.ident = (uintptr_t) fd;
    kev.filter = EVFILT_VNODE;
    kev.flags = EV_ADD | EV_CLEAR;
    kev.fflags = NOTE_WRITE | NOTE_RENAME | NOTE_DELETE;
    kev.data = 0;
    kev.udata = nullptr;

    if (kevent(m_queue_fd, &kev, 1, nullptr, 0, nullptr) == -1)
        log_error("FileWatchKqueue: kevent(EV_ADD, {}): {:m}", filename.c_str());

    std::lock_guard<std::mutex> lock_guard(m_mutex);
    m_callback[fd] = std::move(cb);
    return fd;
}


void FileWatchKqueue::remove_watch(int handle)
{
    std::lock_guard<std::mutex> lock_guard(m_mutex);
    if (handle == -1 || m_callback.find(handle) == m_callback.end())
        return;

    struct kevent kev = {};
    kev.ident = (uintptr_t) handle;
    kev.filter = EVFILT_VNODE;
    kev.flags = EV_DELETE;

    if (kevent(m_queue_fd, &kev, 1, nullptr, 0, nullptr) == -1)
        log_error("FileWatchKqueue: kevent(EV_DELETE): {:m}");

    ::close(handle);
    m_callback.erase(handle);
}


}}  // namespace xci::util
