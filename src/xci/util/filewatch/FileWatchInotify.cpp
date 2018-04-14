// FileWatchInotifyInotify.cpp created on 2018-04-14, part of XCI toolkit
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

#include "FileWatchInotify.h"
#include <xci/util/log.h>

#include <unistd.h>
#include <sys/inotify.h>
#include <climits>
#include <poll.h>
#include <cassert>

namespace xci {
namespace util {


FileWatchInotify::FileWatchInotify()
{
    m_queue_fd = inotify_init();
    if (m_queue_fd < 0 ) {
        log_error("FileWatchInotify: inotify_init: {:m}");
        return;
    }

    if (pipe(m_quit_pipe) == -1) {
        log_error("FileWatchInotify: pipe: {:m}");
        return;
    }

    m_thread = std::thread([this]() {
        log_debug("FileWatchInotify: starting");
        constexpr size_t buflen = sizeof(inotify_event) + NAME_MAX + 1;
        char buffer[buflen];
        struct pollfd fds[2] = {};
        fds[0].fd = m_queue_fd;
        fds[0].events = POLLIN;
        fds[1].fd = m_quit_pipe[0];
        fds[1].events = POLLIN;
        for (;;) {
            int rc = poll(fds, 2, -1);
            if (rc == -1) {
                log_error("FileWatchInotify: poll: {:m}");
                break;
            }
            if (rc > 0) {
                if (fds[1].revents != 0)
                    break;  // quit
                if (fds[0].revents & POLLIN) {
                    ssize_t readlen = read(m_queue_fd, buffer, buflen);
                    if (readlen < 0) {
                        log_error("FileWatchInotify: read: {} {:m}", errno);
                        break;
                    }

                    int ofs = 0;
                    while (ofs < readlen) {
                        auto* event = (inotify_event*) &buffer[ofs];
                        std::lock_guard<std::mutex> lock_guard(m_mutex);
                        auto& cb = m_callback[event->wd];
                        log_debug("xxx {}", event->mask);
                        if (cb) {
                            if (event->mask & IN_MODIFY)
                                cb(Event::Modify);
                            if (event->mask & IN_DELETE_SELF)
                                cb(Event::Delete);
                            if (event->mask & IN_IGNORED) {
                                // Reinstall watch
                                inotify_rm_watch(m_queue_fd, event->wd);
                                int wd = inotify_add_watch(m_queue_fd, m_filename[event->wd].c_str(),
                                                           IN_MODIFY | IN_DELETE_SELF | IN_OPEN | IN_CLOSE_WRITE
                                                           | IN_ATTRIB | IN_MOVE_SELF);
                                log_debug("reinst {} {}", wd, event->wd);
                            }
                        }
                        ofs += sizeof(inotify_event) + event->len;
                    }
                }
            }
        }
        log_debug("FileWatchInotify: quit");
    });
}


FileWatchInotify::~FileWatchInotify()
{
    // Signal the thread to quit
    ::write(m_quit_pipe[1], "\n", 1);
    m_thread.join();
    // Cleanup
    ::close(m_queue_fd);
    ::close(m_quit_pipe[0]);
    ::close(m_quit_pipe[1]);
}


int FileWatchInotify::add_watch(const std::string& filename, std::function<void(Event)> cb)
{
    int wd = inotify_add_watch(m_queue_fd, filename.c_str(),
                               IN_MODIFY | IN_DELETE_SELF | IN_OPEN | IN_CLOSE_WRITE
                               | IN_ATTRIB | IN_MOVE_SELF);
    if (wd < 0) {
        log_error("FileWatchInotify: inotify_add_watch({}): {:m}", filename.c_str());
        return -1;
    }

    std::lock_guard<std::mutex> lock_guard(m_mutex);
    m_callback[wd] = std::move(cb);
    m_filename[wd] = filename;
    return wd;
}


void FileWatchInotify::remove_watch(int handle)
{
    std::lock_guard<std::mutex> lock_guard(m_mutex);
    if (handle == -1 || m_callback.find(handle) == m_callback.end())
        return;
    inotify_rm_watch(m_queue_fd, handle);
    m_callback.erase(handle);
    m_filename.erase(handle);
}


}} // namespace xci::util
