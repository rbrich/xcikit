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

    // Simplified smart pointer for PathNode.
    // Supports only move and calls decref when destroyed and content not moved away.
    // Doesn't check for move-into-self (that would be logic error, so it's fine if it crashes)
    template <typename T>
    class RcPtr {
    public:
        RcPtr() = default;
        explicit RcPtr(T* ptr) : ptr(ptr) {}
        RcPtr(RcPtr&& rhs) noexcept : ptr(rhs.ptr) { rhs.ptr = nullptr; }

        ~RcPtr() {
            if (ptr != nullptr)
                ptr->decref();
        }

        RcPtr& operator=(RcPtr&& rhs) noexcept {
            this->ptr = rhs.ptr;
            rhs.ptr = nullptr;
            return *this;
        }

        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }

        operator bool() const { return ptr != nullptr; }

    private:
        T* ptr = nullptr;
    };

    // Internal hierarchical storage for found files and directories.
    // Manual memory management:
    // - an instance can be created only on heap using make() function
    // - the instance starts with refcount = 1
    // - when a pointer is copied, call incref()
    // - when releasing a pointer, call decref()
    // Users of FileTree don't need to bother, they get only a reference,
    // which is not expected to live outside the callback body.
    class PathNode: NonCopyable, NonMovable {
        /// Internal constructor. Storage for path is allocated by caller.
        /// \param path         Char storage for the path, it must have (at least) dir_len + name_len chars (+ 1 for slash)
        ///                     If name_len is not 0, a single slash character needs to be appended at the end
        ///                     (This avoids string concatenation in `dir_name` method).
        /// \param dir_len      Length of directory part of the path, including trailing slash,
        ///                     e.g. "/", "/etc/", "./" or "" (blank directory is alternative for "./")
        /// \param name_len     Length of name part of the path, not including any slashes or nul terminator
        ///                     e.g. "foo", ".", "" (blank name is alternative for ".")
        PathNode(unsigned int dir_len, unsigned int name_len, PathNode& parent)
                : m_parent(&parent), m_dir_len(dir_len), m_name_len(name_len), m_depth(parent.depth() + 1)
        {
            parent.incref();
        }

        PathNode(unsigned int dir_len, unsigned int name_len) : m_dir_len(dir_len), m_name_len(name_len) {}

    public:
        /// Factory function. Copies path to internal storage of PathNode.
        static RcPtr<PathNode> make(std::string path)
        {
            // remove trailing slashes
            while (path.size() > 1 && path.back() == '/') {
                path.pop_back();
            }

            // split to dir and name parts
            auto last_slash_pos = path.rfind('/');
            const unsigned int name_len = (last_slash_pos == std::string::npos) ? path.size() : path.size() - last_slash_pos - 1;
            const unsigned int dir_len = path.size() - name_len;

            // allocate storage
            const bool need_trailing_slash = name_len != 0;
            char* buffer = new char[sizeof(PathNode) + path.size() + int(need_trailing_slash)];
            auto* path_node = new(buffer) PathNode(dir_len, name_len);

            // write path to storage
            std::memcpy(path_node->m_path_storage, path.data(), path.size());
            if (need_trailing_slash)
                path_node->m_path_storage[path.size()] = '/';
            return RcPtr{path_node};
        }

        static RcPtr<PathNode> make(PathNode& parent, std::string_view name)
        {
            auto parent_dir = parent.dir_path();

            // allocate storage for (parent_dir + name + '/')
            const unsigned int dir_len = parent_dir.size();
            const unsigned int name_len = name.size();
            char* buffer = new char[sizeof(PathNode) + dir_len + name_len + 1];
            auto* path_node = new(buffer) PathNode(dir_len, name_len, parent);

            // write path to storage
            std::memcpy(path_node->m_path_storage, parent_dir.data(), dir_len);
            std::memcpy(path_node->m_path_storage + dir_len, name.data(), name_len);
            path_node->m_path_storage[dir_len + name_len] = '/';
            return RcPtr{path_node};
        }

        void incref() {
            ++m_refcount;
        }

        void decref() {
            if (--m_refcount == 0) {
                this->~PathNode();
                char* buffer = reinterpret_cast<char*>(this);
                delete[] buffer;
            }
        }

        /// Get contained path as directory path with trailing slash
        std::string_view dir_path() const {
            if (m_name_len == 0)
                return {m_path_storage, m_dir_len};
            return {m_path_storage, m_dir_len + m_name_len + 1};  // 1 for trailing slash
        }

        /// Get contained path as file path (no trailing slash with exception of root "/")
        std::string_view file_path() const {
            return {m_path_storage, m_dir_len + m_name_len};
        }

        /// Get parent dir part of contained path
        std::string_view parent_dir_path() const {
            return {m_path_storage, m_dir_len};
        }

        std::string_view name() const {
            return {m_path_storage + m_dir_len, m_name_len};
        }

        /// To be used together with name_len().
        /// NOT null-terminated.
        const char* name_data() const {
            return m_path_storage + m_dir_len;
        }

        unsigned int name_len() const { return m_name_len; }
        bool name_empty() const { return m_name_len == 0; }
        unsigned int size() const { return m_dir_len + m_name_len; }

        bool has_parent() const { return bool(m_parent); }
        int parent_fd() const { return m_parent->fd(); }

        int depth() const { return m_depth; }
        int fd() const { return m_fd; }
        void set_fd(int fd) { m_fd = fd; }
        void close_fd() {
            if (m_fd != -1) {
                close(m_fd);
                m_fd = -1;
            }
        }

        bool stat(struct stat& st) const {
            int rc;
            if (m_fd == -1) {
                if (!m_parent || m_parent->fd() == -1) {
                    rc = ::lstat(std::string(file_path()).c_str(), &st);
                } else {
                    rc = ::fstatat(m_parent->fd(), std::string(name()).c_str(), &st, AT_SYMLINK_NOFOLLOW);
                }
            } else {
                rc = ::fstat(m_fd, &st);
            }
            return rc == 0;
        }

        bool readlink(std::string& target) const {
            target.resize(PATH_MAX);
            ssize_t res;
            if (has_parent() && parent_fd() != -1)
                res = ::readlinkat(parent_fd(), std::string(name()).c_str(), target.data(), target.size());
            else
                res = ::readlink(std::string(file_path()).c_str(), target.data(), target.size());
            if (res < 0)
                return false;
            target.resize(res);
            return true;
        }

        int open(int oflag = O_RDONLY | O_NOFOLLOW | O_CLOEXEC) const {
            if (has_parent() && parent_fd() != -1)
                return ::openat(parent_fd(), std::string(name()).c_str(), oflag);
            return ::open(std::string(file_path()).c_str(), oflag);
        }

        /// Is this a node from input, i.e. `walk()`?
        bool is_input() const { return m_depth == 0; }

        /// Name starts with '.'
        bool is_hidden() const { return m_name_len != 0 && m_path_storage[m_dir_len] == '.'; }

        /// Name is "." or ".."
        bool is_dots_entry() const {
            return is_hidden() && (m_name_len == 1 || (m_name_len == 2 && m_path_storage[m_dir_len + 1] == '.'));
        }

    private:
        RcPtr<PathNode> m_parent;
        std::atomic_int m_refcount {1};
        unsigned int m_dir_len = 0;
        unsigned int m_name_len = 0;
        int m_fd = -1;
        int m_depth = 0;  // depth from input
        char m_path_storage[0];
    };

    enum Type {
        File,
        Directory,
        OpenError,
        OpenDirError,
        ReadDirError,
    };

    /// for Directories, return true to descend, false to skip
    using NodeCallback = std::function<bool(int tn, const PathNode&, Type)>;

    /// \param num_threads      Number of threads FileTree should spawn.
    ///                         Can be zero, but don't forget to call worker() from main thread.
    explicit FileTree(int num_threads, NodeCallback&& cb)
        : m_cb(std::move(cb)), m_busy_max(num_threads + 1)
    {
        assert(m_cb);
        m_workers.reserve(num_threads);
        for (int i = 0; i != num_threads; ++i) {
            m_workers.emplace_back([this, i]{ worker(i + 1); });
        }
    }

    ~FileTree() {
        // join threads
        for (auto& t : m_workers)
            t.join();
    }

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
        auto node = PathNode::make(pathname.lexically_normal().string());
        // open original, uncleaned path (required to support root "/")
        if (!open_and_report(0, open_path, *node, AT_FDCWD))
            return;
        enqueue(0, std::move(node));
    }

    void main_worker() {
        m_busy.fetch_sub(1, std::memory_order_release);  // main worker is counted as busy from start (see ctor)
        worker(0);
    }

private:
    void enqueue(int tn, RcPtr<PathNode> path_node);

    void worker(int tn);

    void read(int tn, RcPtr<PathNode> path) {
        thread_local DirEntryArena arena;
        DirEntryArenaGuard arena_guard(arena);
        std::vector<sys_dirent_t*> entries;

#ifdef XCI_LISTDIR_GETDENTS
        if (!list_dir_sys(path->fd(), arena, entries)) {
            m_cb(tn, *path, OpenDirError);
            path->close_fd();
            return;
        }
#else
        DIR* dirp;
        if (!list_dir_posix(path->fd(), dirp, arena, entries)) {
            m_cb(tn, *path, OpenDirError);
            path->set_fd(-1);  // ownership moved to dirp, closed together with it
            return;
        }
#endif

        std::sort(entries.begin(), entries.end(),
                [](const sys_dirent_t* a, const sys_dirent_t* b)
                { return a->d_type < b->d_type || (a->d_type == b->d_type && strcmp(a->d_name, b->d_name) < 0); });

        for (const auto* entry : entries) {
            auto entry_node = PathNode::make(*path, {entry->d_name, strlen(entry->d_name)});

            if ((entry->d_type & DT_DIR) == DT_DIR || entry->d_type == DT_UNKNOWN) {
                // readdir says it's a dir or it doesn't know
                if (open_and_report(tn, entry->d_name, *entry_node, path->fd()))
                    enqueue(tn, std::move(entry_node));
                continue;
            }
            m_cb(tn, *entry_node, File);
        }

        // Close parent directory FD - it was already used in open_and_report() above
        // Note that the PathNode itself can stay alive, as parent in children nodes.
        // The FD needs to be closed here to save FDs - closing when destroying PathNode
        // would be too late, there would be too many open FDs at the same time.
#ifdef XCI_LISTDIR_GETDENTS
        path->close_fd();
#else
        path->set_fd(-1);  // ownership moved to dirp, closed together with it
        closedir(dirp);
#endif
    }

    /// \param pathname     path to open, may be relative
    /// \param node         PathNode associated with the path, for reporting
    /// \param at_fd        if path is relative, this can be open parent FD or AT_FDCWD
    /// \return     true if opened as a directory and callback returned true (i.e. descend)
    bool open_and_report(int tn, const char* pathname, PathNode& node, int at_fd) {
        // try to open as a directory, if it fails with ENOTDIR, it is a file
        constexpr int flags = O_DIRECTORY | O_NOFOLLOW | O_NOCTTY | O_CLOEXEC;
        int fd = openat(at_fd, pathname, flags, O_RDONLY);
        if (fd == -1) {
            if (errno == ENOTDIR) {
                // it's a file - report it
                m_cb(tn, node, File);
                return false;
            }
            m_cb(tn, node, OpenError);
            return false;
        }
        node.set_fd(fd);

        if (!m_cb(tn, node, Directory)) {
            node.close_fd();
            return false;
        }
        return true;
    }

    NodeCallback m_cb;

    RcPtr<PathNode> m_job;
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
};

} // namespace xci::core

#endif // include guard
