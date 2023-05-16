// FSWatch.cpp created on 2019-03-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FSWatch.h"
#include <xci/core/file.h>
#include <xci/core/log.h>

#include <cassert>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <fcntl.h>
#include <dirent.h>

namespace xci::core {


static constexpr uint32_t fflags_file = NOTE_WRITE | NOTE_RENAME | NOTE_DELETE | NOTE_ATTRIB;
static constexpr uint32_t fflags_dir = NOTE_WRITE | NOTE_RENAME | NOTE_DELETE;


FSWatch::FSWatch(EventLoop& loop, FSWatch::Callback cb)
    : Watch(loop), m_main_cb(std::move(cb))
{}


FSWatch::~FSWatch()
{
    for (const auto& file : m_file)
        unregister_kevent(file.fd);
    for (const auto& dir : m_dir)
        unregister_kevent(dir.fd);
}


bool FSWatch::add(const fs::path& pathname, FSWatch::PathCallback cb)
{
    // Is the directory already watched?
    auto dir = pathname.parent_path();
    auto it = std::find_if(m_dir.begin(), m_dir.end(),
                           [&dir](const Dir& d) { return d.name == dir; });
    int dir_fd;
    if (it == m_dir.end()) {
        // No, start watching it
        int fd = register_kevent(dir, fflags_dir);
        if (fd == -1)
            return false;
        m_dir.push_back(Dir{fd, dir});
        log::debug("EventLoop: Watching dir {} ({})", dir, fd);
        dir_fd = fd;
    } else {
        dir_fd = it->fd;
    }

    // Directory is now watched, add watch for the file
    int fd = register_kevent(pathname, fflags_file, /*no_exist_ok=*/true);
    auto filename = pathname.filename();
    m_file.push_back(File{fd, dir_fd, filename, std::move(cb)});
    log::debug("EventLoop: Added watch {} / {} ({})", dir, filename, fd);
    return true;
}


bool FSWatch::remove(const fs::path& pathname)
{
    // Find dir record
    auto dir = pathname.parent_path();
    int dir_fd;
    {
        auto it = std::find_if(m_dir.begin(), m_dir.end(),
                               [&dir](const Dir& d) { return d.name == dir; });
        if (it == m_dir.end()) {
            // not found
            return false;
        }
        dir_fd = it->fd;
    }

    // Find file record
    auto filename = pathname.filename();
    auto it = std::find_if(m_file.begin(), m_file.end(),
                           [&filename, dir_fd](const File& w) {
                               return w.dir_fd == dir_fd && w.name == filename;
                           });
    if (it == m_file.end()) {
        // not found
        return false;
    }

    // Remove file record
    if (it->fd != -1)
        unregister_kevent(it->fd);
    log::debug("FSWatch: Removed watch {} ({})", pathname, it->fd);
    m_file.erase(it);

    // If there are more watches on the same dir, we're finished
    if (std::count_if(m_file.begin(), m_file.end(),
                      [dir_fd](const File& w) { return w.dir_fd == dir_fd; }) > 0)
        return true;

    // Otherwise, remove also the watched dir
    auto it_dir = std::find_if(m_dir.begin(), m_dir.end(),
                               [dir_fd](const Dir& d) { return d.fd == dir_fd; });
    if (it_dir == m_dir.end()) {
        assert(!"FSWatch: Watched dir not found!");
    } else {
        m_dir.erase(it_dir);
    }

    unregister_kevent(dir_fd);
    log::debug("FSWatch: Stopped watching dir {} ({})", dir, dir_fd);
    return true;
}


void FSWatch::_notify(const struct kevent& event)
{
    const auto fd = int(event.ident);

    // Is this a dir?
    auto it_dir = std::find_if(m_dir.begin(), m_dir.end(),
                               [fd](const Dir& d) { return d.fd == fd; });
    if (it_dir != m_dir.end()) {
        auto dir = it_dir->name;
        if (event.fflags & NOTE_WRITE) {
            // Directory content has changed, look for newly created files
            DIR *dirp = opendir(dir.c_str());
            if (dirp == nullptr) {
                log::error("FSWatch: opendir({}): {m}", dir);
                return;
            }
            struct dirent *dp;
            while ((dp = readdir(dirp)) != nullptr) {
                if (dp->d_type != DT_REG)
                    continue;
                for (auto& w : m_file) {
                    if (w.dir_fd == fd && w.name == dp->d_name && w.fd == -1) {
                        w.fd = register_kevent(dir / w.name, fflags_file);
                        w.cb(Event::Create);
                    }
                }
            }
            closedir(dirp);
            return;
        }
        if (event.fflags & NOTE_DELETE || event.fflags & NOTE_RENAME) {
            // Directory itself is gone - remove all file watches
            std::erase_if(m_file, [this, fd](const auto& w) {
                bool belongs_to_dir = (w.dir_fd == fd);
                if (belongs_to_dir) {
                    w.cb(Event::Stopped);
                    if (w.fd != -1)
                        unregister_kevent(w.fd);
                    log::debug("FSWatch: Removed watch {} ({})", w.name, w.fd);
                }
                return belongs_to_dir;
            });
            // Remove directory watch itself
            unregister_kevent(fd);
            log::debug("FSWatch: Stopped watching dir {} ({})", dir, fd);
            m_dir.erase(it_dir);
        }
        return;
    }

    // Is this a file?
    auto it_file = std::find_if(m_file.begin(), m_file.end(),
                                [fd](const File& w) { return w.fd == fd; });
    if (it_file != m_file.end()) {
        auto& w = *it_file;
        if (w.cb) {
            if (event.fflags & NOTE_ATTRIB) {
                w.cb(Event::Attrib);
            }
            if (event.fflags & NOTE_WRITE) {
                w.cb(Event::Modify);
            }
            if (event.fflags & NOTE_DELETE || event.fflags & NOTE_RENAME) {
                w.cb(Event::Delete);
                // The file is gone, try to reinstall watch (this may be atomic save)
                unregister_kevent(fd);
                w.fd = -1;
            }
        }
    }
}


int FSWatch::register_kevent(const fs::path& path, uint32_t fflags, bool no_exist_ok)
{
    int fd = ::open(path.c_str(), O_EVTONLY);
    if (fd == -1) {
        if (!no_exist_ok)
            log::error("FSWatch: open(\"{}\", O_EVTONLY): {m}", path);
        return -1;
    }

    struct kevent kev = {};
    kev.ident = (uintptr_t) fd;
    kev.filter = EVFILT_VNODE;
    kev.flags = EV_ADD | EV_CLEAR;
    kev.fflags = fflags;
    kev.data = 0;
    kev.udata = this;

    if (!m_loop._kevent(kev)) {
        log::error("EventLoop: kevent(EV_ADD, {}): {m}", path);
        ::close(fd);
        return -1;
    }

    return fd;
}


void FSWatch::unregister_kevent(int fd)
{
    struct kevent kev = {};
    kev.ident = (uintptr_t) fd;
    kev.filter = EVFILT_VNODE;
    kev.flags = EV_DELETE;
    if (!m_loop._kevent(kev)) {
        if (errno != EBADF)  // event already removed
            log::error("EventLoop: kevent(EV_DELETE, {}): {m}", fd);
    }

    ::close(fd);
}


} // namespace xci::core
