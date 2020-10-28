// FileTree.cpp created on 2020-10-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FileTree.h"
#include <xci/core/log.h>
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


void FileTree::enqueue(std::shared_ptr<PathNode>&& path)
{
    // Skip locking and queuing if all workers are (apparently) busy.
    if (m_busy.load(std::memory_order_relaxed) >= m_busy_max) {
        TRACE("skip lock ({} busy)", m_busy);
        read(std::move(path));
        return;
    }

    std::unique_lock lock(m_mutex);
    if (!m_job) {
        m_busy.fetch_add(1, std::memory_order_release);
        m_job = std::move(path);
        lock.unlock();
        m_cv.notify_one();
    } else {
        // process the item in this thread
        // (better than blocking and doing nothing)
        lock.unlock();
        read(std::move(path));
    }
}


void FileTree::worker(int num)
{
    TRACE("[{}] worker start", num);
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

        TRACE("[{}] worker read ({} busy)", num, m_busy);
        read(std::move(path));
        m_busy.fetch_sub(1, std::memory_order_release);
    }
finish:
    m_cv.notify_all();
    TRACE("[{}] worker finish", num);
}


} // namespace xci::core
