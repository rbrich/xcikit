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
#include <xci/util/file.h>
#include <xci/util/log.h>

#include <vector>
#include <cassert>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <fcntl.h>
#include <dirent.h>

namespace xci {
namespace util {


static constexpr uint32_t fflags_file = NOTE_WRITE | NOTE_RENAME | NOTE_DELETE | NOTE_ATTRIB;
static constexpr uint32_t fflags_dir = NOTE_WRITE | NOTE_RENAME | NOTE_DELETE;


FileWatchKqueue::FileWatchKqueue()
{
    m_kqueue_fd = kqueue();
    if (m_kqueue_fd < 0 ) {
        log_error("FileWatchKqueue: kqueue: {m}");
        return;
    }

    m_thread = std::thread([this]() {
        log_debug("FileWatchKqueue: Thread starting");
        struct kevent ke;
        for (;;) {
            int rc = kevent(m_kqueue_fd, nullptr, 0, &ke, 1, nullptr);
            if (rc == -1) {
                if (errno == EBADF)
                    break;  // kqueue closed, quit
                log_error("FileWatchKqueue: kevent(): {m}");
                break;
            }
            if (rc == 1 && ke.filter == EVFILT_VNODE) {
                handle_event((int) ke.ident, ke.fflags);
            }
        }
        log_debug("FileWatchKqueue: Thread finished");
    });

    // Needed for Linux implementation but unused here (this avoids warning)
    (void) m_quit_pipe;
}


FileWatchKqueue::~FileWatchKqueue()
{
    // Cleanup
    ::close(m_kqueue_fd);
    m_thread.join();
}


int FileWatchKqueue::add_watch(const std::string& filename, std::function<void(Event)> cb)
{
    if (m_kqueue_fd < 0)
        return -1;

    std::lock_guard<std::mutex> lock_guard(m_mutex);

    // Is the directory already watched?
    auto dir = path_dirname(filename);
    auto it = std::find_if(m_dir.begin(), m_dir.end(),
                           [&dir](const Dir& d) { return d.dir == dir; });
    if (it == m_dir.end()) {
        // No, start watching it
        int fd = register_kevent(dir, fflags_dir);
        if (fd == -1)
            return -1;
        m_dir.push_back({fd, dir});
        log_debug("FileWatchKqueue: Watching dir {} ({})", dir, fd);
    }

    // Directory is now watched, add watch for the file
    int fd = register_kevent(filename, fflags_file);
    if (fd == -1)
        return -1;
    auto base = path_basename(filename);
    m_watch.push_back({fd, dir, base, std::move(cb)});
    log_debug("FileWatchKqueue: Added watch {} / {} ({})", dir, base, fd);
    return fd;
}


void FileWatchKqueue::remove_watch(int handle)
{
    if (handle == -1)
        return;

    std::lock_guard<std::mutex> lock_guard(m_mutex);

    remove_watch_nolock(handle);
}


void FileWatchKqueue::remove_watch_nolock(int handle)
{
    // Remove the watch
    auto it = std::find_if(m_watch.begin(), m_watch.end(),
                           [handle](const Watch& w) { return w.fd == handle; });
    if (it == m_watch.end())
        return;

    auto dir = it->dir;

    unregister_kevent(handle);
    log_debug("FileWatchKqueue: Removed watch {} / {} ({})",
              it->dir, it->name, it->fd);
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

    unregister_kevent(it_dir->fd);
    log_debug("FileWatchKqueue: Stopped watching dir {} ({})",
              it_dir->dir, it_dir->fd);
    m_dir.erase(it_dir);
}



int FileWatchKqueue::register_kevent(const std::string& path, uint32_t fflags)
{
    int fd = ::open(path.c_str(), O_EVTONLY);
    if (fd == -1) {
        log_error("FileWatchKqueue: open({}, O_EVTONLY): {m}", path.c_str());
        return -1;
    }

    struct kevent kev;
    kev.ident = (uintptr_t) fd;
    kev.filter = EVFILT_VNODE;
    kev.flags = EV_ADD | EV_CLEAR;
    kev.fflags = fflags;
    kev.data = 0;
    kev.udata = nullptr;

    if (kevent(m_kqueue_fd, &kev, 1, nullptr, 0, nullptr) == -1) {
        log_error("FileWatchKqueue: kevent(EV_ADD, {}): {m}", path.c_str());
        ::close(fd);
        return -1;
    }

    return fd;
}


void FileWatchKqueue::unregister_kevent(int fd)
{
    struct kevent kev = {};
    kev.ident = (uintptr_t) fd;
    kev.filter = EVFILT_VNODE;
    kev.flags = EV_DELETE;
    if (kevent(m_kqueue_fd, &kev, 1, nullptr, 0, nullptr) == -1) {
        log_error("FileWatchKqueue: kevent(EV_DELETE): {m}");
    }

    ::close(fd);
}


void FileWatchKqueue::handle_event(int fd, uint32_t fflags)
{
    std::lock_guard<std::mutex> lock_guard(m_mutex);

    // Is this a dir?
    auto it_dir = std::find_if(m_dir.begin(), m_dir.end(),
                               [fd](const Dir& d) { return d.fd == fd; });
    if (it_dir != m_dir.end()) {
        auto dir = it_dir->dir;
        if (fflags & NOTE_WRITE) {
            // Directory content has changed, look for newly created files
            DIR *dirp = opendir(dir.c_str());
            if (dirp == nullptr) {
                log_error("FileWatchKqueue: opendir({}): {:m}", dir);
                return;
            }
            struct dirent *dp;
            while ((dp = readdir(dirp)) != nullptr) {
                if (dp->d_type != DT_REG)
                    continue;
                std::string name(dp->d_name);
                for (auto& w : m_watch) {
                    if (w.dir == dir && w.name == name && w.fd == -1) {
                        w.fd = register_kevent(w.dir + "/" + w.name, fflags_file);
                        w.cb(Event::Create);
                    }
                }
            }
            closedir(dirp);
            return;
        }
        if (fflags & NOTE_DELETE || fflags & NOTE_RENAME) {
            // Directory itself is gone
            std::vector<int> remove_list;
            for (auto& w : m_watch) {
                if (w.dir == dir) {
                    w.cb(Event::Stopped);
                    if (w.fd != -1)
                        remove_list.push_back(w.fd);
                }
            }
            for (auto handle : remove_list) {
                remove_watch_nolock(handle);
            }
        }
    }

    // Is this a file?
    auto it_file = std::find_if(m_watch.begin(), m_watch.end(),
                               [fd](const Watch& w) { return w.fd == fd; });
    if (it_file != m_watch.end()) {
        auto& w = *it_file;
        if (w.cb) {
            if (fflags & NOTE_ATTRIB) {
                w.cb(Event::Attrib);
            }
            if (fflags & NOTE_WRITE) {
                w.cb(Event::Modify);
            }
            if (fflags & NOTE_DELETE || fflags & NOTE_RENAME) {
                w.cb(Event::Delete);
                // The file is gone, try to reinstall watch (this may be atomic save)
                unregister_kevent(fd);
                w.fd = -1;
            }
        }
    }
}


}}  // namespace xci::util
