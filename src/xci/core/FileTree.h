// FileTree.h created on 2020-10-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_FILETREE_H
#define XCI_CORE_FILETREE_H

#include <xci/core/string.h>
#include <xci/core/listdir.h>
#include <xci/core/NonCopyable.h>
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
    class PathNode: public NonCopyable {
    public:
        /// Basic constructor. Copies path to internal storage.
        explicit PathNode(std::string path)
        {
            // remove trailing slashes
            while (path.size() > 1 && path.back() == '/') {
                path.pop_back();
            }

            // split to dir and name parts
            auto last_slash_pos = path.rfind('/');
            m_name_len = (last_slash_pos == std::string::npos) ? path.size() : path.size() - last_slash_pos - 1;
            m_dir_len = path.size() - m_name_len;

            // allocate storage
            const bool need_trailing_slash = m_name_len != 0;
            m_path_ptr.reset(new char[path.size() + int(need_trailing_slash)]);

            // write to storage
            std::memcpy(m_path_ptr.get(), path.data(), path.size());
            if (need_trailing_slash)
                m_path_ptr[path.size()] = '/';
        }

        /// Advanced constructor. Storage for path is allocated by caller.
        /// \param path         Char storage for the path, it must have (at least) dir_len + name_len chars (+ 1 for slash)
        ///                     If name_len is not 0, a single slash character needs to be appended at the end
        ///                     (This avoids string concatenation in `dir_name` method).
        /// \param dir_len      Length of directory part of the path, including trailing slash,
        ///                     e.g. "/", "/etc/", "./" or "" (blank directory is alternative for "./")
        /// \param name_len     Length of name part of the path, not including any slashes or nul terminator
        ///                     e.g. "foo", ".", "" (blank name is alternative for ".")
        PathNode(std::unique_ptr<char[]>&& path, unsigned int dir_len, unsigned int name_len,
                 const std::shared_ptr<PathNode>& parent)
            : m_parent(parent), m_path_ptr(std::move(path)), m_dir_len(dir_len), m_name_len(name_len), m_depth(parent->m_depth + 1) {}

        /// Get contained path as directory path with trailing slash
        std::string_view dir_path() const {
            if (m_name_len == 0)
                return {m_path_ptr.get(), m_dir_len};
            return {m_path_ptr.get(), m_dir_len + m_name_len + 1};  // 1 for trailing slash
        }

        /// Get contained path as file path (no trailing slash with exception of root "/")
        std::string_view file_path() const {
            return {m_path_ptr.get(), m_dir_len + m_name_len};
        }

        /// Get parent dir part of contained path
        std::string_view parent_dir_path() const {
            return {m_path_ptr.get(), m_dir_len};
        }

        std::string_view name() const {
            return {m_path_ptr.get() + m_dir_len, m_name_len};
        }

        /// To be used together with name_len().
        /// NOT null-terminated.
        const char* name_data() const {
            return m_path_ptr.get() + m_dir_len;
        }

        unsigned int name_len() const { return m_name_len; }
        bool name_empty() const { return m_name_len == 0; }
        unsigned int size() const { return m_dir_len + m_name_len; }

        bool has_parent() const { return bool(m_parent); }
        int parent_fd() const { return m_parent->m_fd; }

        int depth() const { return m_depth; }
        int fd() const { return m_fd; }
        void set_fd(int fd) { m_fd = fd; }

        bool stat(struct stat& st) const {
            int rc;
            if (m_fd == -1) {
                if (!m_parent || m_parent->m_fd == -1) {
                    rc = ::lstat(std::string(file_path()).c_str(), &st);
                } else {
                    rc = ::fstatat(m_parent->m_fd, std::string(name()).c_str(), &st, AT_SYMLINK_NOFOLLOW);
                }
            } else {
                rc = ::fstat(m_fd, &st);
            }
            return rc == 0;
        }

        /// Is this a node from input, i.e. `walk()`?
        bool is_input() const { return m_depth == 0; }

        /// Name starts with '.'
        bool is_hidden() const { return m_name_len != 0 && m_path_ptr[m_dir_len] == '.'; }

        /// Name is "." or ".."
        bool is_dots_entry() const {
            return is_hidden() && (m_name_len == 1 || (m_name_len == 2 && m_path_ptr[m_dir_len + 1] == '.'));
        }

    private:
        std::shared_ptr<PathNode> m_parent;
        std::unique_ptr<char[]> m_path_ptr;
        unsigned int m_dir_len = 0;
        unsigned int m_name_len = 0;
        int m_fd = -1;
        int m_depth = 0;  // depth from input
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
    static bool is_default_ignored(std::string_view path);
    static std::string default_ignore_list(const char* sep);

    void walk_cwd() {
        // open "." but show entries without "./" prefix in reporting
        walk("", ".");
    }

    void walk(const fs::path& pathname) {
        walk(pathname, pathname.c_str());
    }

    void walk(const fs::path& pathname, const char* open_path) {
        // Create base node from relative or absolute path, e.g.:
        // - relative: "foo/bar", "foo", ".", ".."
        // - absolute: "/foo/bar", "/foo", "/"
        // Trailing slash is normalized in PathNode constructor.
        auto node = std::make_shared<PathNode>(pathname.lexically_normal().string());
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
        if (!list_dir_sys(path->fd(), arena, entries)) {
            m_cb(*path, OpenDirError);
            close(path->fd());
            return;
        }
#else
        DIR* dirp;
        if (!list_dir_posix(path->fd(), dirp, arena, entries)) {
            m_cb(*path, OpenDirError);
            return;
        }
#endif

        std::sort(entries.begin(), entries.end(),
                [](const sys_dirent_t* a, const sys_dirent_t* b)
                { return a->d_type < b->d_type || (a->d_type == b->d_type && strcmp(a->d_name, b->d_name) < 0); });

        for (const auto* entry : entries) {
            // entry_pathname = path->dir_name() + entry->d_name + '/';
            auto parent_dir = path->dir_path();
            auto name_len = strlen(entry->d_name);
            auto entry_pathname = std::unique_ptr<char[]>(new char[parent_dir.size() + name_len + 1]);
            std::memcpy(entry_pathname.get(), parent_dir.data(), parent_dir.size());
            std::memcpy(entry_pathname.get() + parent_dir.size(), entry->d_name, name_len);
            entry_pathname[parent_dir.size() + name_len] = '/';

            // Check ignore list
            if (m_default_ignore && is_default_ignored({entry_pathname.get(), parent_dir.size() + name_len}))
                continue;

            auto entry_path = std::make_shared<PathNode>(std::move(entry_pathname), parent_dir.size(), name_len, path);

            if ((entry->d_type & DT_DIR) == DT_DIR || entry->d_type == DT_UNKNOWN) {
                // readdir says it's a dir or it doesn't know
                if (open_and_report(entry->d_name, *entry_path, path->fd()))
                    enqueue(std::move(entry_path));
                continue;
            }
            m_cb(*entry_path, File);
        }
#ifdef XCI_LISTDIR_GETDENTS
        close(path->fd());
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
        node.set_fd(fd);

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
