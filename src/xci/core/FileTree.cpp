// FileTree.cpp created on 2020-10-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FileTree.h"
#include <xci/core/log.h>

namespace xci::core {


void FileTree::enqueue(int tn, RcPtr<PathNode> path_node)
{
    // Skip locking and queuing if all workers are (apparently) busy.
    if (m_busy.load(std::memory_order_relaxed) >= m_busy_max) {
        TRACE("skip lock ({} busy)", m_busy);
        read(tn, std::move(path_node));
        return;
    }

    std::unique_lock lock(m_mutex);
    if (!m_job) {
        m_busy.fetch_add(1, std::memory_order_release);
        m_job = std::move(path_node);
        lock.unlock();
        m_cv.notify_one();
    } else {
        // process the item in this thread
        // (better than blocking and doing nothing)
        lock.unlock();
        read(tn, std::move(path_node));
    }
}


void FileTree::worker(int tn)
{
    TRACE("[{}] worker start", tn);
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

        TRACE("[{}] worker read ({} busy)", tn, m_busy);
        read(tn, std::move(path));
        m_busy.fetch_sub(1, std::memory_order_release);
    }
finish:
    m_cv.notify_all();
    TRACE("[{}] worker finish", tn);
}


} // namespace xci::core
