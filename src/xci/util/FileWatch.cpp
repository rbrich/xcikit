// FileWatch.cpp created on 2018-03-30, part of XCI toolkit
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

#include "FileWatch.h"

#include <xci/util/log.h>

#ifdef __linux__

#include <sys/inotify.h>
#include <unistd.h>
#include <climits>
#include <poll.h>

namespace xci {
namespace util {


FileWatch& FileWatch::default_instance()
{
    static FileWatch instance;
    return instance;
}


FileWatch::FileWatch()
{
    m_inotify = inotify_init();
    if (m_inotify < 0 ) {
        log_error("FileWatch: inotify_init: {:m}");
        return;
    }

    if (pipe(m_quit_pipe) == -1) {
        log_error("FileWatch: pipe: {:m}");
        return;
    }

    m_thread = std::move(std::thread([this]() {
        log_debug("FileWatch: starting");
        constexpr size_t buflen = sizeof(inotify_event) + NAME_MAX + 1;
        char buffer[buflen];
        struct pollfd fds[2] = {};
        fds[0].fd = m_inotify;
        fds[0].events = POLLIN;
        fds[1].fd = m_quit_pipe[0];
        fds[1].events = POLLIN;
        for (;;) {
            int rc = poll(fds, 2, -1);
            if (rc == -1) {
                log_error("FileWatch: poll: {:m}");
                return;
            }
            if (rc > 0) {
                if (fds[1].revents != 0) {
                    log_debug("FileWatch: quit");
                    return;
                }
                if (fds[0].revents & POLLIN) {
                    ssize_t readlen = read(m_inotify, buffer, buflen);
                    if (readlen < 0) {
                        log_error("FileWatch: read: {} {:m}", errno);
                        return;
                    }

                    int ofs = 0;
                    while (ofs < readlen) {
                        auto* event = (inotify_event*) &buffer[ofs];
                        auto& cb = m_callback[event->wd];
                        if (cb) {
                            if (event->mask & IN_MODIFY)
                                cb(Event::Modify);
                            if (event->mask & IN_CLOSE_WRITE)
                                cb(Event::CloseWrite);
                            if (event->mask & IN_DELETE_SELF)
                                cb(Event::Delete);
                        }
                        ofs += sizeof(inotify_event) + event->len;
                    }
                }
            }
        }
    }));
}


FileWatch::~FileWatch()
{
    // Signal the thread to quit
    ::write(m_quit_pipe[1], "\n", 1);
    m_thread.join();
    // Cleanup
    ::close(m_inotify);
    ::close(m_quit_pipe[0]);
    ::close(m_quit_pipe[1]);
}


int FileWatch::add_watch(const std::string& filename, std::function<void(Event)> cb)
{
    int wd = inotify_add_watch(m_inotify, filename.c_str(),
                               IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF);
    m_callback[wd] = std::move(cb);
    return wd;
}


void FileWatch::remove_watch(int handle)
{
    if (handle != -1) {
        inotify_rm_watch(m_inotify, handle);
        m_callback.erase(handle);
    }
}


}}  // namespace xci::util

// end ifdef __linux__
#elif defined(__APPLE__)

// TODO

// end ifdef __APPLE__
#else
#error "FileWatch not implemented for this platform!"
#endif
