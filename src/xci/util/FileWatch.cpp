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

#include <unistd.h>

#ifdef __linux__

#include <sys/inotify.h>
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
    m_queue_fd = inotify_init();
    if (m_queue_fd < 0 ) {
        log_error("FileWatch: inotify_init: {:m}");
        return;
    }

    if (pipe(m_quit_pipe) == -1) {
        log_error("FileWatch: pipe: {:m}");
        return;
    }

    m_thread = std::thread([this]() {
        log_debug("FileWatch: starting");
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
                log_error("FileWatch: poll: {:m}");
                break;
            }
            if (rc > 0) {
                if (fds[1].revents != 0)
                    break;  // quit
                if (fds[0].revents & POLLIN) {
                    ssize_t readlen = read(m_queue_fd, buffer, buflen);
                    if (readlen < 0) {
                        log_error("FileWatch: read: {} {:m}", errno);
                        break;
                    }

                    int ofs = 0;
                    while (ofs < readlen) {
                        auto* event = (inotify_event*) &buffer[ofs];
                        auto& cb = m_callback[event->wd];
                        if (cb) {
                            if (event->mask & IN_MODIFY)
                                cb(Event::Modify);
                            if (event->mask & IN_DELETE_SELF)
                                cb(Event::Delete);
                        }
                        ofs += sizeof(inotify_event) + event->len;
                    }
                }
            }
        }
        log_debug("FileWatch: quit");
    });
}


FileWatch::~FileWatch()
{
    // Signal the thread to quit
    ::write(m_quit_pipe[1], "\n", 1);
    m_thread.join();
    // Cleanup
    ::close(m_queue_fd);
    ::close(m_quit_pipe[0]);
    ::close(m_quit_pipe[1]);
}


int FileWatch::add_watch(const std::string& filename, std::function<void(Event)> cb)
{
    int wd = inotify_add_watch(m_queue_fd, filename.c_str(),
                               IN_MODIFY | IN_DELETE_SELF);
    if (wd < 0) {
        log_error("FileWatch: inotify_add_watch({}): {:m}", filename.c_str());
        return -1;
    }
    m_callback[wd] = std::move(cb);
    return wd;
}


void FileWatch::remove_watch(int handle)
{
    if (handle != -1) {
        inotify_rm_watch(m_queue_fd, handle);
        m_callback.erase(handle);
    }
}


}}  // namespace xci::util

// endif defined(__linux__)
#elif defined(__bsdi__) || defined(__APPLE__)

#include <sys/types.h>
#include <sys/event.h>
#include <fcntl.h>

namespace xci {
namespace util {


FileWatch& FileWatch::default_instance()
{
    static FileWatch instance;
    return instance;
}


FileWatch::FileWatch()
{
    m_queue_fd = kqueue();
    if (m_queue_fd < 0 ) {
        log_error("FileWatch: kqueue: {:m}");
        return;
    }

    m_thread = std::thread([this]() {
        log_debug("FileWatch: starting");
        struct kevent ke;
        for (;;) {
            int rc = kevent(m_queue_fd, nullptr, 0, &ke, 1, nullptr);
            if (rc == -1) {
                if (errno == EBADF)
                    break;  // kqueue closed, quit
                log_error("FileWatch: kevent(): {:m}");
                break;
            }
            if (rc == 1 && ke.filter == EVFILT_VNODE) {
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
        log_debug("FileWatch: quit");
    });

    // Needed for Linux implementation but unused here (this avoids warning)
    (void) m_quit_pipe;
}


FileWatch::~FileWatch()
{
    // Cleanup
    ::close(m_queue_fd);
    m_thread.join();
}


int FileWatch::add_watch(const std::string& filename, std::function<void(Event)> cb)
{
    if (m_queue_fd < 0)
        return -1;

    int fd = ::open(filename.c_str(), O_EVTONLY);
    if (fd < 0) {
        log_error("FileWatch: open({}, O_EVTONLY): {:m}", filename.c_str());
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
        log_error("FileWatch: kevent(EV_ADD, {}): {:m}", filename.c_str());

    m_callback[fd] = std::move(cb);
    return fd;
}


void FileWatch::remove_watch(int handle)
{
    if (handle == -1 || m_callback.find(handle) == m_callback.end())
        return;

    struct kevent kev = {};
    kev.ident = (uintptr_t) handle;
    kev.filter = EVFILT_VNODE;
    kev.flags = EV_DELETE;

    if (kevent(m_queue_fd, &kev, 1, nullptr, 0, nullptr) == -1)
        log_error("FileWatch: kevent(EV_DELETE): {:m}");

    ::close(handle);
    m_callback.erase(handle);
}


}}  // namespace xci::util

// endif defined(__bsdi__) || defined(__APPLE__)
#else

#warning "FileWatch not implemented for this platform!"

namespace xci {
namespace util {

FileWatch& FileWatch::default_instance()
{
    static FileWatch instance;
    return instance;
}

FileWatch::FileWatch() {}
FileWatch::~FileWatch() {}

int FileWatch::add_watch(const std::string& filename, std::function<void(Event)> cb)
{
    return -1;
}

void FileWatch::remove_watch(int handle)
{

}

}}  // namespace xci::util

#endif
