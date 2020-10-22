// epoll/FSWatch.cpp created on 2018-04-14, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#include "FSWatch.h"
#include <xci/core/event.h>
#include <xci/core/file.h>
#include <xci/core/log.h>

#include <algorithm>
#include <vector>
#include <climits>
#include <cassert>
#include <unistd.h>
#include <poll.h>
#include <sys/inotify.h>

namespace xci::core {


FSWatch::FSWatch(EventLoop& loop, Callback cb)
        : Watch(loop), m_main_cb(std::move(cb))
{
    m_inotify_fd = inotify_init();
    if (m_inotify_fd < 0 ) {
        log::error("FSWatch: inotify_init: {m}");
        return;
    }
    loop._register(m_inotify_fd, *this, POLLIN);
}


FSWatch::~FSWatch()
{
    if (m_inotify_fd != -1)
        ::close(m_inotify_fd);
}


bool FSWatch::add(const fs::path& pathname, FSWatch::PathCallback cb)
{
    if (m_inotify_fd < 0)
        return false;

    // Is the directory already watched?
    auto dir = pathname.parent_path();
    auto it = std::find_if(m_dir.begin(), m_dir.end(),
                           [&dir](const Dir& d) { return d.name == dir; });
    int dir_wd;
    if (it == m_dir.end()) {
        // No, start watching it
        int wd = inotify_add_watch(m_inotify_fd, dir.c_str(),
                                   IN_CREATE | IN_DELETE | IN_MODIFY |
                                   IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO |
                                   IN_DELETE_SELF | IN_MOVE_SELF | IN_ONLYDIR);
        if (wd < 0) {
            log::error("FSWatch: inotify_add_watch({}): {m}", dir);
            return false;
        }
        m_dir.push_back({wd, dir});
        log::debug("FSWatch: Watching dir {} ({})", dir, wd);
        dir_wd = wd;
    } else {
        dir_wd = it->wd;
    }

    // Directory is now watched, add the new watch to it
    auto filename = pathname.filename();
    m_file.push_back({dir_wd, filename, std::move(cb)});
    log::debug("FSWatch: Watching file {}/{}", dir, filename);
    return true;
}


bool FSWatch::remove(const fs::path& pathname)
{
    // Find dir record
    auto dir = pathname.parent_path();
    int dir_wd;
    {
        auto it = std::find_if(m_dir.begin(), m_dir.end(),
                               [&dir](const Dir& d) { return d.name == dir; });
        if (it == m_dir.end()) {
            // not found
            return false;
        }
        dir_wd = it->wd;
    }

    // Find file record
    auto filename = pathname.filename();
    auto it = std::find_if(m_file.begin(), m_file.end(),
                           [&filename, dir_wd](const File& f) {
                               return f.dir_wd == dir_wd && f.name == filename;
                           });
    if (it == m_file.end()) {
        // not found
        return false;
    }

    // Remove file record
    log::debug("FSWatch: Removing watch {}/{}", dir, filename);
    m_file.erase(it);

    // If there are more watches on the same dir, we're finished
    if (std::count_if(m_file.begin(), m_file.end(),
                      [dir_wd](const File& f)
                      { return f.dir_wd == dir_wd; }) > 0)
        return true;

    // Otherwise, remove also the watched dir
    auto it_dir = std::find_if(m_dir.begin(), m_dir.end(),
                               [dir_wd](const Dir& d) { return d.wd == dir_wd; });
    if (it_dir == m_dir.end()) {
        assert(!"FSWatch: Watched dir not found!");
    } else {
        m_dir.erase(it_dir);
    }
    inotify_rm_watch(m_inotify_fd, dir_wd);
    log::debug("FSWatch: Stopped watching dir {} ({})", dir, dir_wd);
    return true;
}


void FSWatch::_notify(uint32_t epoll_events)
{
    if (epoll_events & EPOLLIN) {
        constexpr size_t buflen = sizeof(inotify_event) + NAME_MAX + 1;
        char buffer[buflen];
        ssize_t readlen = read(m_inotify_fd, buffer, buflen);
        if (readlen < 0) {
            log::error("FSWatch: read: {m}");
            return;
        }

        int ofs = 0;
        while (ofs < readlen) {
            auto* event = (inotify_event*) &buffer[ofs];
            std::string name(event->name);
            //log::debug("FSWatch: event {:x} for {}",
            //          event->mask, name);
            handle_event(event->wd, event->mask, name);
            ofs += sizeof(inotify_event) + event->len;
        }
    }
}


void FSWatch::handle_event(int wd, uint32_t mask, const std::string& name)
{
    // Lookup dir name
    auto it_dir = std::find_if(m_dir.begin(), m_dir.end(),
                               [wd](const Dir& d) { return d.wd == wd; });
    if (it_dir == m_dir.end()) {
        return;
    }

    // Lookup file callback
    auto it_file = std::find_if(m_file.begin(), m_file.end(),
                                [&name, wd](const File& f) {
                                    return f.dir_wd == wd && f.name == name;
                                });
    if (it_file != m_file.end()) {
        if (it_file->cb) {
            auto& cb = it_file->cb;
            if (mask & IN_CREATE)  cb(Event::Create);
            if (mask & IN_DELETE)  cb(Event::Delete);
            if (mask & IN_MODIFY)  cb(Event::Modify);
            if (mask & IN_ATTRIB)  cb(Event::Attrib);
            if (mask & IN_MOVED_FROM)  cb(Event::Delete);
            if (mask & IN_MOVED_TO)  cb(Event::Create);
        }
    }

    // Watched directory itself was deleted / moved
    if (mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
        std::vector<std::string> remove_list;
        for (auto& w : m_file) {
            if (w.dir_wd == wd) {
                if (w.cb) {
                    w.cb(Event::Stopped);
                }
                remove_list.push_back(it_dir->name / w.name);
            }
        }
        for (auto& path : remove_list) {
            remove(path);
        }
    }
}


} // namespace xci::core
