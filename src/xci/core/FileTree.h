// FileTree.h created on 2020-10-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_FILETREE_H
#define XCI_CORE_FILETREE_H

#include <xci/core/string.h>
#include <xci/core/listdir.h>
#include <xci/config.h>

#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <cstring>
#include <cassert>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace xci::core {

namespace fs = std::filesystem;


class FileTree {
public:
    struct PathNode {
        explicit PathNode(std::string_view component) : component(component) {}
        PathNode(std::string_view component, const std::shared_ptr<PathNode>& parent)
            : parent(parent), component(component), depth(parent->depth + 1) {}

        /// Convert contained directory path to a string:
        /// - no parent, component ""           => ""
        /// - no parent, component "."          => "./"
        /// - no parent, component "/"          => "/"
        /// - no parent, component "foo"        => "foo/"
        /// - parent "", component "bar"        => "bar/"
        /// - parent ".", component "bar"       => "./bar/"
        /// - parent "/", component "bar"       => "/bar/"
        /// - parent "foo", component "bar"     => "foo/bar/"
        /// - parent "/foo", component "bar"    => "/foo/bar/"
        std::string dir_name() const {
            if (!parent && (component.empty() || component == "/"))
                return component;
            return parent_dir_name() + component + '/';
        }

        /// Same as dir_name, but the first variant is invalid
        /// and '/' is not appended.
        std::string file_name() const {
            return parent_dir_name() + component;
        }

        /// Get parent dir part of contained path
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
                if (!parent || parent->fd == -1)
                    rc = ::lstat(file_name().c_str(), &st);
                else
                    rc = ::fstatat(parent->fd, component.c_str(), &st, AT_SYMLINK_NOFOLLOW);
            } else {
                rc = ::fstat(fd, &st);
            }
            return rc == 0;
        }

        /// Is this a node from input, i.e. `walk()`?
        bool is_input() const {
            return depth == 0;
        }

        std::shared_ptr<PathNode> parent;
        std::string component;
        int fd = -1;
        int depth = 0;  // depth from input
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

    /// \param num_threads      Number of threads FileTree should spawn.
    ///                         Can be zero, but don't forget to call worker() from main thread.
    explicit FileTree(unsigned num_threads, Callback&& cb)
        : m_cb(std::move(cb)), m_busy_max((int)num_threads + 1)
    {
        assert(m_cb);
        m_workers.reserve(num_threads);
        for (unsigned i = 0; i != num_threads; ++i) {
            m_workers.emplace_back([this, i]{ worker(i + 1); });
        }
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
        // open "." but show entries without "./" prefix in reporting
        walk("", ".");
    }

    void walk(const fs::path& pathname) {
        walk(pathname, pathname.c_str());
    }

    void walk(const fs::path& pathname, const char* open_path) {
        // normalize and remove trailing slash
        auto node_path = pathname.lexically_normal().string();
        if (node_path.size() > 1) { // size of "/" is 1
            rstrip(node_path, '/');
        }
        // create base node from relative or absolute path, e.g.:
        // - relative: "foo/bar", "foo", ".", ".."
        // - absolute: "/foo/bar", "/foo", "/"
        auto node = std::make_shared<PathNode>(node_path);
        // open original, uncleaned path (required to support root "/")
        if (!open_and_report(open_path, *node, AT_FDCWD))
            return;
        enqueue(std::move(node));
    }

    void main_worker() {
        m_busy.fetch_sub(1, std::memory_order_release);  // main worker is counted as busy from start (see ctor)
        worker(0);
    }

private:
    void enqueue(std::shared_ptr<PathNode>&& path);

    void worker(int num);

    void read(std::shared_ptr<PathNode>&& path) {
        thread_local DirEntryArena arena;
        DirEntryArenaGuard arena_guard(arena);
        std::vector<sys_dirent_t*> entries;

#ifdef XCI_LISTDIR_GETDENTS
        if (!list_dir_sys(path->fd, arena, entries)) {
            m_cb(*path, OpenDirError);
            close(path->fd);
            return;
        }
#else
        DIR* dirp;
        if (!list_dir_posix(path->fd, dirp, arena, entries)) {
            m_cb(*path, OpenDirError);
            return;
        }
#endif

        std::sort(entries.begin(), entries.end(),
                [](const sys_dirent_t* a, const sys_dirent_t* b)
                { return a->d_type < b->d_type || (a->d_type == b->d_type && strcmp(a->d_name, b->d_name) < 0); });

        for (const auto* entry : entries) {
            // Check ignore list
            std::string entry_pathname = path->dir_name() + entry->d_name;
            if (m_default_ignore && is_default_ignored(entry_pathname))
                continue;

            auto entry_path = std::make_shared<PathNode>(entry->d_name, path);

            if ((entry->d_type & DT_DIR) == DT_DIR || entry->d_type == DT_UNKNOWN) {
                // readdir says it's a dir or it doesn't know
                if (open_and_report(entry->d_name, *entry_path, path->fd))
                    enqueue(std::move(entry_path));
                continue;
            }
            m_cb(*entry_path, File);
        }
#ifdef XCI_LISTDIR_GETDENTS
        close(path->fd);
#else
        closedir(dirp);
#endif
    }

    /// \param pathname     path to open, may be relative
    /// \param node         PathNode associated with the path, for reporting
    /// \param at_fd        if path is relative, this can be open parent FD or AT_FDCWD
    /// \return     true if opened as a directory and callback returned true (i.e. descend)
    bool open_and_report(const char* pathname, PathNode& node, int at_fd) {
        // try to open as a directory, if it fails with ENOTDIR, it is a file
        constexpr int flags = O_DIRECTORY | O_NOFOLLOW | O_NOCTTY | O_CLOEXEC;
        int fd = openat(at_fd, pathname, flags, O_RDONLY);
        if (fd == -1) {
            if (errno == ENOTDIR) {
                // it's a file - report it
                m_cb(node, File);
                return false;
            }
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

    Callback m_cb;

    std::shared_ptr<PathNode> m_job;
    std::vector<std::thread> m_workers;
    std::mutex m_mutex;  // sync access to job from workers and CV
    std::condition_variable m_cv;

    // busy counter has three purposes:
    // * to keep workers alive while main thread submits work via walk()
    //   - the counter starts at 1 and main_worker() decrements it
    // * to keep workers alive when a job is posted via m_job
    //   - it's incremented when posting a job
    // * to keep workers alive while other workers are processing jobs
    //   - more jobs might be added by the processing
    //   - when worker finishes processing, it decrements the counter
    //     (this pairs with job posting)
    std::atomic_int m_busy {1};

    // total workers, including the main one
    int m_busy_max;

    bool m_default_ignore = true;
};

} // namespace xci::core

#endif // include guard
