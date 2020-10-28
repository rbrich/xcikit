// FileTree.cpp created on 2020-10-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FileTree.h"
#include <xci/core/log.h>
#include <xci/core/sys.h>  // get_thread_id
#include <fmt/format.h>

namespace xci::core {

static constinit const char* s_default_ignore_list[] = {
    #if defined(__APPLE__)
        "/dev",
        "/System/Volumes",
    #elif defined(__linux__)
        "/dev",
        "/proc",
        "/sys",
        "/mnt",
        "/media",
    #endif
};

bool FileTree::is_default_ignored(const std::string& path)
{
    return std::any_of(std::begin(s_default_ignore_list), std::end(s_default_ignore_list),
            [&path](const char* ignore_path) { return path == ignore_path; });
}

std::string FileTree::default_ignore_list(const char* sep)
{
    return fmt::format("{}", fmt::join(std::begin(s_default_ignore_list), std::end(s_default_ignore_list), sep));
}


void FileTree::worker()
{
    (void) get_thread_id();
    TRACE("[{}] worker start", get_thread_id());
    while(m_busy.load(std::memory_order_acquire) != 0) {
        std::unique_lock lock(m_mutex);
        while (!m_job) {
            m_cv.wait(lock);
            if (m_busy.load(std::memory_order_acquire) == 0) {
                lock.unlock();
                goto finish;
            }
        }
        // clear m_job by moving it away - it's important to move into temp variable
        auto path = std::move(m_job);
        assert(!m_job);  // cleared
        lock.unlock();

        TRACE("[{}] worker read start ({} busy)", get_thread_id(), m_busy);
        read(std::move(path));
        m_busy.fetch_sub(1, std::memory_order_release);
        TRACE("[{}] worker read finish ({} busy)", get_thread_id(), m_busy);
    }
finish:
    m_cv.notify_all();
    TRACE("[{}] worker finish", get_thread_id());
}


} // namespace xci::core
