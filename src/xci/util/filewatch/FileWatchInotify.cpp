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
#include <xci/util/file.h>
#include <xci/util/log.h>

#include <algorithm>
#include <vector>
#include <climits>
#include <cassert>
#include <unistd.h>
#include <poll.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>

namespace xci {
namespace util {


FileWatchInotify::FileWatchInotify()
{
    m_inotify_fd = inotify_init();
    if (m_inotify_fd < 0 ) {
        log_error("FileWatchInotify: inotify_init: {m}");
        return;
    }

    m_quit_fd = eventfd(0, 0);
    if (m_quit_fd == -1) {
        log_error("FileWatchInotify: eventfd: {m}");
        return;
    }

    m_thread = std::thread([this]() {
        log_debug("FileWatchInotify: Thread starting");
        constexpr size_t buflen = sizeof(inotify_event) + NAME_MAX + 1;
        char buffer[buflen];
        struct pollfd fds[2] = {};
        fds[0].fd = m_inotify_fd;
        fds[0].events = POLLIN;
        fds[1].fd = m_quit_fd;
        fds[1].events = POLLIN;
        for (;;) {
            int rc = poll(fds, 2, -1);
            if (rc == -1) {
                log_error("FileWatchInotify: poll: {m}");
                break;
            }
            if (rc > 0) {
                if (fds[1].revents != 0) {
                    uint64_t value;
                    ssize_t readlen = read(m_quit_fd, &value, sizeof(value));
                    if (readlen < 0) {
                        log_error("FileWatchInotify: read(quit_fd): {m}");
                        break;
                    }
                    if (value > 0)
                        break;  // quit
                }
                if (fds[0].revents & POLLIN) {
                    ssize_t readlen = read(m_inotify_fd, buffer, buflen);
                    if (readlen < 0) {
                        log_error("FileWatchInotify: read(inotify_fd): {m}");
                        break;
                    }

                    int ofs = 0;
                    while (ofs < readlen) {
                        auto* event = (inotify_event*) &buffer[ofs];
                        std::string name(event->name);
                        //log_debug("FileWatchInotify: event {:x} for {}",
                        //          event->mask, name);
                        handle_event(event->wd, event->mask, name);
                        ofs += sizeof(inotify_event) + event->len;
                    }
                }
            }
        }
        log_debug("FileWatchInotify: Thread finished");
    });
}


FileWatchInotify::~FileWatchInotify()
{
    // Signal the thread to quit
    uint64_t value = 1;
    ::write(m_quit_fd, &value, sizeof(value));
    m_thread.join();
    // Cleanup
    ::close(m_inotify_fd);
    ::close(m_quit_fd);
}


int FileWatchInotify::add_watch(const std::string& filename,
                                std::function<void(Event)> cb)
{
    if (m_inotify_fd < 0)
        return -1;

    std::lock_guard<std::mutex> lock_guard(m_mutex);

    // Is the directory already watched?
    auto dir = path_dirname(filename);
    auto it = std::find_if(m_dir.begin(), m_dir.end(),
                           [&dir](const Dir& d) { return d.dir == dir; });
    if (it == m_dir.end()) {
        // No, start watching it
        int wd = inotify_add_watch(m_inotify_fd, dir.c_str(),
                                   IN_CREATE | IN_DELETE | IN_MODIFY |
                                   IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO |
                                   IN_DELETE_SELF | IN_MOVE_SELF | IN_ONLYDIR);
        if (wd < 0) {
            log_error("FileWatchInotify: inotify_add_watch({}): {m}", dir);
            return -1;
        }
        m_dir.push_back({dir, wd});
        log_debug("FileWatchInotify: Watching dir {} ({})", dir, wd);
    }

    // Directory is now watched, add the new watch to it
    auto base = path_basename(filename);
    auto handle = m_next_handle++;
    m_watch.push_back({handle, dir, base, std::move(cb)});
    log_debug("FileWatchInotify: Added watch {} / {} ({})", dir, base, handle);
    return handle;
}


void FileWatchInotify::remove_watch(int handle)
{
    if (handle == -1)
        return;

    std::lock_guard<std::mutex> lock_guard(m_mutex);

    remove_watch_nolock(handle);
}


void FileWatchInotify::remove_watch_nolock(int handle)
{
    // Remove the watch
    auto it = std::find_if(m_watch.begin(), m_watch.end(),
                           [handle](const Watch& w) { return w.handle == handle; });
    if (it == m_watch.end())
        return;
    auto dir = it->dir;
    log_debug("FileWatchInotify: Removed watch {} / {} ({})",
              it->dir, it->name, it->handle);
    m_watch.erase(it);

    // Remove also the watched dir, if it's not used by any other watches
    if (std::count_if(m_watch.begin(), m_watch.end(),
                      [&dir](const Watch& w) { return w.dir == dir; }) > 0)
        return;

    auto it_dir = std::find_if(m_dir.begin(), m_dir.end(),
                               [&dir](const Dir& d) { return d.dir == dir; });
    if (it_dir == m_dir.end()) {
        assert(!"Watched dir not found!");
        return;
    }
    inotify_rm_watch(m_inotify_fd, it_dir->wd);
    log_debug("FileWatchInotify: Stopped watching dir {} ({})",
              it_dir->dir, it_dir->wd);
    m_dir.erase(it_dir);
}


void FileWatchInotify::handle_event(int wd, uint32_t mask, const std::string& name)
{
    std::lock_guard<std::mutex> lock_guard(m_mutex);

    // Lookup dir name
    auto it_dir = std::find_if(m_dir.begin(), m_dir.end(),
                               [wd](const Dir& d) { return d.wd == wd; });
    if (it_dir == m_dir.end()) {
        return;
    }

    // Fire callbacks registered for the file
    for (auto& w : m_watch) {
        if (w.dir == it_dir->dir && w.name == name && w.cb) {
            if (mask & IN_CREATE)  w.cb(Event::Create);
            if (mask & IN_DELETE)  w.cb(Event::Delete);
            if (mask & IN_MODIFY)  w.cb(Event::Modify);
            if (mask & IN_ATTRIB)  w.cb(Event::Attrib);
            if (mask & IN_MOVED_FROM)  w.cb(Event::Delete);
            if (mask & IN_MOVED_TO)  w.cb(Event::Create);
        }
    }

    // Watched directory itself was deleted / moved
    if (mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
        std::vector<int> remove_list;
        for (auto& w : m_watch) {
            if (w.dir == it_dir->dir) {
                if (w.cb)
                    w.cb(Event::Stopped);
                remove_list.push_back(w.handle);
            }
        }
        for (int handle : remove_list) {
            remove_watch_nolock(handle);
        }
    }
}


}} // namespace xci::util
