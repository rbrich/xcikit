// iocp/FSWatch.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// References:
// - https://github.com/jimbeveridge/readdirectorychanges

#include "FSWatch.h"
#include <xci/core/file.h>
#include <xci/core/string.h>
#include <xci/core/log.h>
#include <algorithm>
#include <cassert>


namespace xci::core {


FSWatch::FSWatch(EventLoop& loop, FSWatch::Callback cb)
    : Watch(loop), m_main_cb(std::move(cb))
{}


FSWatch::~FSWatch()
{
    for (const auto& dir : m_dir) {
        CloseHandle(dir.h);
    }
}


bool FSWatch::add(const std::string& pathname, FSWatch::PathCallback cb)
{
    // Is the directory already watched?
    auto dir = path::dirname(pathname);
    auto it = std::find_if(m_dir.begin(), m_dir.end(),
            [&dir](const Dir& d) { return d.name == dir; });
    HANDLE dir_h;
    if (it == m_dir.end()) {
        // No, start watching it
        dir_h = CreateFileA(dir.c_str(), FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr, OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                nullptr);
        if (dir_h == INVALID_HANDLE_VALUE) {
            log_error("FSWatch: CreateFileA({}, FILE_LIST_DIRECTORY): {mm}", dir);
            return false;
        }

        // associate the file handle with completion port
        // to get the asynchronous notifications
        if (!m_loop._associate(dir_h, *this))
            return false;

        m_dir.emplace_back(dir_h, dir);
        if (!_request_notification(m_dir.back())) {
            m_dir.pop_back();
            CloseHandle(dir_h);
            return false;
        }

        log_debug("EventLoop: Watching dir {} ({})", dir, (intptr_t) dir_h);
    } else {
        dir_h = it->h;
    }

    // Directory is now watched, add the new watch to it
    auto filename = path::base_name(pathname);
    m_file.push_back({dir_h, filename, std::move(cb)});
    log_debug("FSWatch: Watching file {}/{}", dir, filename);
    return true;
}


bool FSWatch::remove(const std::string& pathname)
{
    // Find dir record
    auto dir = path::dirname(pathname);
    HANDLE dir_h;
    {
        auto it = std::find_if(m_dir.begin(), m_dir.end(),
                [&dir](const Dir& d) { return d.name == dir; });
        if (it == m_dir.end()) {
            // not found
            return false;
        }
        dir_h = it->h;
    }

    // Find file record
    auto filename = path::base_name(pathname);
    auto it = std::find_if(m_file.begin(), m_file.end(),
            [&filename, dir_h](const File& f) {
                return f.dir_h == dir_h && f.name == filename;
            });
    if (it == m_file.end()) {
        // not found
        return false;
    }

    // Remove file record
    log_debug("FSWatch: Removing watch {}/{}", dir, filename);
    m_file.erase(it);

    // If there are more watches on the same dir, we're finished
    if (std::count_if(m_file.begin(), m_file.end(),
            [dir_h](const File& f)
            { return f.dir_h == dir_h; }) > 0)
        return true;

    // Otherwise, remove also the watched dir
    auto it_dir = std::find_if(m_dir.begin(), m_dir.end(),
            [dir_h](const Dir& d) { return d.h == dir_h; });
    if (it_dir == m_dir.end()) {
        assert(!"FSWatch: Watched dir not found!");
    } else {
        // closing the handle also removes IOCP association,
        // so we won't get any more notifications
        CloseHandle(it_dir->h);
        m_dir.erase(it_dir);
    }
    log_debug("FSWatch: Stopped watching dir {} ({})", dir, (intptr_t) dir_h);
    return true;
}


void FSWatch::_notify(LPOVERLAPPED overlapped)
{
    Dir* dir = static_cast<Dir*>(overlapped);

    std::byte* buf = dir->notif_buffer;
    for (;;) {
        auto& fni = reinterpret_cast<FILE_NOTIFY_INFORMATION&>(*buf);
        std::wstring filename(fni.FileName, fni.FileNameLength/sizeof(wchar_t));
        auto name = to_utf8(filename);

        TRACE("{} / {} (action: {})", dir->name, name, fni.Action);

        // Lookup file callback
        auto it_file = std::find_if(m_file.begin(), m_file.end(),
            [&name, dir](const File& f) {
                return f.dir_h == dir->h && f.name == name;
            });
        if (it_file != m_file.end() && it_file->cb) {
            auto& cb = it_file->cb;
            switch (fni.Action) {
                case FILE_ACTION_ADDED: cb(Event::Create); break;
                case FILE_ACTION_REMOVED: cb(Event::Delete); break;
                case FILE_ACTION_MODIFIED: cb(Event::Modify); break;
                case FILE_ACTION_RENAMED_OLD_NAME: cb(Event::Delete); break;
                case FILE_ACTION_RENAMED_NEW_NAME: cb(Event::Create); break;
            }
        }

        if (!fni.NextEntryOffset)
            break;
        buf += fni.NextEntryOffset;
    }

    // reissue the notification request
    _request_notification(*dir);
}


bool FSWatch::_request_notification(Dir& dir)
{
    BOOL ok = ::ReadDirectoryChangesW(dir.h,
            dir.notif_buffer,
            sizeof(dir.notif_buffer),
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME
            | FILE_NOTIFY_CHANGE_DIR_NAME
            | FILE_NOTIFY_CHANGE_ATTRIBUTES
            | FILE_NOTIFY_CHANGE_SIZE
            | FILE_NOTIFY_CHANGE_LAST_WRITE
            | FILE_NOTIFY_CHANGE_CREATION,
            nullptr,
            &dir,
            nullptr);
    if (!ok) {
        log_error("FSWatch: ReadDirectoryChangesW({}): {mm}", dir.name);
        return false;
    }
    return true;
}


} // namespace xci::core
