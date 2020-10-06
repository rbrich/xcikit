// FileTree.h created on 2020-10-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_FILETREE_H
#define XCI_CORE_FILETREE_H

#include <xci/core/string.h>

#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <cassert>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef TRACE
#define TRACE(fmt, ...)  ((void)0)
#define NO_TRACE_PROVIDED
#endif

namespace xci::core {

class FileTree {
public:
    struct PathNode {
        explicit PathNode(std::string_view component) : component(component) {}  // NOLINT
        PathNode(std::string_view component, const std::shared_ptr<PathNode>& parent) : parent(parent), component(component) {}  // NOLINT

        /// Convert contained directory path to a string:
        /// - no parent, component "."          => ""
        /// - no parent, component ""           => "/"
        /// - no parent, component "foo"        => "foo/"
        /// - parent "", component "bar"        => "/bar/"
        /// - parent "foo", component "bar"     => "foo/bar/"
        /// - parent "/foo", component "bar"    => "/foo/bar/"
        std::string dir_name() const {
            if (!parent && component == ".")
                return {};
            return parent_dir_name() + component + '/';
        }

        /// Same as dir_name, but the first variant is invalid
        /// and '/' is not appended.
        std::string file_name() const {
            assert(!component.empty());
            return parent_dir_name() + component;
        }

        /// Get parent dir part of contained path:
        /// - no parent, component ""           => "" (meaning "/")
        /// - no parent, component "foo"        => ""
        /// - parent "", component "bar"        => "/"
        /// - parent "foo", component "bar"     => "foo/"
        /// - parent "/foo", component "bar"    => "/foo/"
        std::string parent_dir_name() const {
            if (!parent)
                return {};
            return parent->dir_name();
        }

        bool is_root() const {
            return !parent && component.empty();
        }

        bool stat(struct stat& st) const {
            int rc;
            if (fd == -1) {
                assert(parent);  // don't call stat on incomplete PathNode
                rc = fstatat(parent->fd, component.c_str(), &st, AT_SYMLINK_NOFOLLOW);
            } else {
                rc = fstat(fd, &st);
            }
            return rc == 0;
        }

        /// Is this a node from input, i.e. `walk()`?
        bool is_input() const {
            return flags & f_input;
        }

        std::shared_ptr<PathNode> parent;
        std::string component;
        int fd = -1;

        using Flags = unsigned int;
        static constexpr Flags f_input = 1;
        Flags flags = 0;
    };

    enum Type {
        File,
        Directory,
        OpenError,
        OpenDirError,
        ReadDirError,
    };

    /// for Directories, return true to descend, false to skip
    using Callback = std::function<bool(const PathNode&, Type)>;

    /// \param max_threads      Number of threads FileTree can spawn.
    explicit FileTree(unsigned max_threads, unsigned queue_size, Callback&& cb) : m_max_threads(max_threads), m_cb(std::move(cb)) {
        assert(m_cb);
        m_workers.reserve(max_threads);
        m_queue.reserve(queue_size);
        assert(m_queue.capacity() == queue_size);
    }

    ~FileTree() {
        // join threads
        for (auto& t : m_workers)
            t.join();
    }

    /// Allows disabling default ignored paths like /dev
    void set_default_ignore(bool enabled) { m_default_ignore = enabled; }
    static bool is_default_ignored(const std::string& path);
    static std::string default_ignore_list(const char* sep);

    void walk_cwd() {
        auto path = std::make_shared<FileTree::PathNode>(".");
        path->flags = PathNode::f_input;
        int fd = open(".", O_DIRECTORY | O_NOFOLLOW | O_NOCTTY, O_RDONLY);
        if (fd == -1) {
            m_cb(*path, OpenError);
            return;
        }
        path->fd = fd;
        enqueue(std::move(path));
    }

    void walk(const std::string& pathname) {
        auto pathname_clean = rstripped(pathname, '/');
        if (pathname.empty() || pathname_clean == ".") {
            walk_cwd();
            return;
        }

        // create PathNode also for parent, so the reporting is consistent
        // (component in each reported PathNode is always cleaned basename)
        std::shared_ptr<PathNode> path;
        auto components = rsplit(pathname_clean, '/', 1);
        if (components.size() == 2) {
            // relative or absolute path, e.g.:
            // - relative "foo/bar/", processed to components ["foo", "bar"]
            // - absolute "/foo/bar/", processed to components ["/foo", "bar"]
            // - absolute in root: "/foo/", processed to components ["", "bar"]
            auto parent = std::make_shared<PathNode>(components[0]);
            path = std::make_shared<PathNode>(components[1], parent);
        } else {
            // relative or absolute path, e.g.:
            // - relative path "foo/", cleaned to "foo"
            // - absolute root "/", cleaned to ""
            path = std::make_shared<PathNode>(pathname_clean);
        }
        path->flags = PathNode::f_input;

        if (!open_and_report(pathname.c_str(), *path))
            return;
        enqueue(std::move(path));
    }

    void worker() {
        TRACE("[{}] worker start", get_thread_id());
        std::unique_lock lock(m_mutex);
        while(!m_queue.empty() || m_busy != 0) {
            while (m_queue.empty()) {
                m_cv.wait(lock);
                if (m_busy == 0 && m_queue.empty()) {
                    lock.unlock();
                    m_cv.notify_all();
                    TRACE("[{}] worker finish", get_thread_id());
                    return;
                }
            }

            auto path = std::move(m_queue.back());
            m_queue.pop_back();
            TRACE("[{}] worker read start ({} busy, {} queue)", get_thread_id(), m_busy, m_queue.size());
            ++m_busy;
            lock.unlock();
            read(path);
            lock.lock();
            --m_busy;
            TRACE("[{}] worker read finish ({} busy, {} queue)", get_thread_id(), m_busy, m_queue.size());
        }
        lock.unlock();
        m_cv.notify_all();
        TRACE("[{}] worker finish", get_thread_id());
    }

private:
    void start_worker() {
        m_workers.emplace_back(std::thread{[this]{
            worker();
        }});
    }

    void enqueue(std::shared_ptr<PathNode>&& path) {
        std::unique_lock lock(m_mutex);
        if (m_queue.size() < m_queue.capacity()) {
            m_queue.emplace_back(std::move(path));
            lock.unlock();
            m_cv.notify_one();
        } else {
            if (m_workers.size() < m_max_threads)
                start_worker();
            // process the item in this thread
            // (better than blocking and doing nothing)
            TRACE("[{}] enqueue read start ({} busy, {} queue)", get_thread_id(), m_busy, m_queue.size());
            ++m_busy;
            lock.unlock();
            read(path);
            lock.lock();
            --m_busy;
            TRACE("[{}] enqueue read finish ({} busy, {} queue)", get_thread_id(), m_busy, m_queue.size());
        }
    }

    void read(const std::shared_ptr<PathNode>& path) {
        DIR* dirp = fdopendir(path->fd);
        if (dirp == nullptr) {
            m_cb(*path, OpenDirError);
            close(path->fd);
            return;
        }

        for (;;) {
            errno = 0;
            auto* dir_entry = readdir(dirp);
            if (dir_entry == nullptr) {
                // end or error
                if (errno != 0) {
                    m_cb(*path, ReadDirError);
                }
                break;
            }
            if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
                continue;

            // Check ignore list
            std::string entry_pathname = path->dir_name() + dir_entry->d_name;
            if (m_default_ignore && is_default_ignored(entry_pathname))
                continue;

            auto entry_path = std::make_shared<PathNode>(dir_entry->d_name, path);

            if ((dir_entry->d_type & DT_DIR) == DT_DIR || dir_entry->d_type == DT_UNKNOWN) {
                // readdir says it's a dir or it doesn't know
                if (open_and_report(dir_entry->d_name, *entry_path, path->fd))
                    enqueue(std::move(entry_path));
                continue;
            }
            m_cb(*entry_path, File);
        }
        closedir(dirp);
    }

    /// \param pathname     path to open, may be relative
    /// \param node         PathNode associated with the path, for reporting
    /// \param at_fd        if path is relative, this can be open parent FD or AT_FDCWD
    /// \return     true if opened as a directory and callback returned true (i.e. descend)
    bool open_and_report(const char* pathname, PathNode& node, int at_fd = AT_FDCWD) {
        // try to open as a directory, if it fails with ENOTDIR, it is a file
        int fd = openat(at_fd, pathname, O_DIRECTORY | O_NOFOLLOW | O_NOCTTY, O_RDONLY);
        if (fd == -1) {
            if (errno == ENOTDIR) {
                // it's a file - report it
                m_cb(node, File);
                return false;
            }
            if (!m_cb(node, Directory))
                return false;
            m_cb(node, OpenError);
            return false;
        }
        node.fd = fd;

        if (!m_cb(node, Directory)) {
            close(fd);
            return false;
        }
        return true;
    }

    const unsigned m_max_threads;
    Callback m_cb;
    std::vector<std::shared_ptr<PathNode>> m_queue;
    std::vector<std::thread> m_workers;
    std::mutex m_mutex;  // for both queue and workers
    std::condition_variable m_cv;
    int m_busy = 0;  // number of threads in `read`
    bool m_default_ignore = true;
};

} // namespace xci::core

#ifdef NO_TRACE_PROVIDED
#undef TRACE
#undef NO_TRACE_PROVIDED
#endif

#endif // include guard
