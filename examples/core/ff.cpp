// ff.cpp created on 2020-03-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Find File (ff) command line tool

#include <xci/core/ArgParser.h>
#include <xci/core/sys.h>
#include <xci/core/string.h>
#include <xci/core/log.h>
#include <xci/core/TermCtl.h>
#include <xci/compat/macros.h>

#include <fmt/core.h>

#include <iostream>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <cstring>
#include <utility>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>

using namespace xci::core;
using namespace xci::core::argparser;
using namespace std;


class FileTree {
public:
    struct PathNode {
        explicit PathNode(const std::string& component) : component(component) {}  // NOLINT
        PathNode(const char* component, const std::shared_ptr<PathNode>& parent) : component(component), parent(parent) {}  // NOLINT

        std::string to_string() const {
            if (!parent || parent->component.empty())
                return component;
            return parent->to_string() + '/' + component;
        }

        std::string dir_to_string() const {
            if (!parent || parent->component.empty())
                return {};
            return parent->to_string() + '/';
        }

        std::string component;
        int fd = -1;
        std::shared_ptr<PathNode> parent;
    };

    enum Type {
        File,
        Directory,
        OpenError,
        OpenDirError,
        ReadDirError,
    };

    /// for Directories, return true to descent, false to skip
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

    void walk(const std::string& pathname) {
        auto path = std::make_shared<PathNode>(rstrip(pathname, '/'));
        // try to open as directory, if it fails with ENOTDIR, it is a file
        int fd = open(pathname.empty() ? "." : pathname.c_str(), O_DIRECTORY | O_NOFOLLOW | O_NOCTTY, O_RDONLY);
        if (fd == -1) {
            if (errno == ENOTDIR) {
                // it's a file - report it
                m_cb(*path, File);
                return;
            }
            if (!m_cb(*path, Directory))
                return;
            m_cb(*path, OpenError);
            return;
        }
        path->fd = fd;

        if (!pathname.empty()) {
            if (!m_cb(*path, Directory)) {
                close(fd);
                return;
            }
        }
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

            auto entry_path = std::make_shared<PathNode>(dir_entry->d_name, path);

            if ((dir_entry->d_type & DT_DIR) == DT_DIR || dir_entry->d_type == DT_UNKNOWN) {
                // readdir says it's a dir or it doesn't know
                // try to open it as a dir, fallback to regular file on ENOTDIR
                int entry_fd = openat(path->fd, dir_entry->d_name, O_DIRECTORY | O_NOFOLLOW | O_NOCTTY, O_RDONLY);
                if (entry_fd == -1) {
                    if (errno == ENOTDIR) {
                        // it's a file - report it
                        m_cb(*entry_path, File);
                        continue;
                    }
                    if (!m_cb(*entry_path, Directory))
                        continue;
                    m_cb(*entry_path, OpenError);
                    continue;
                }
                entry_path->fd = entry_fd;

                if (!m_cb(*entry_path, Directory)) {
                    close(entry_fd);
                    continue;
                }
                enqueue(std::move(entry_path));
                continue;
            }
            m_cb(*entry_path, File);
        }
        closedir(dirp);
    }

    const unsigned m_max_threads;
    Callback m_cb;
    std::vector<std::shared_ptr<PathNode>> m_queue;
    std::vector<std::thread> m_workers;
    int m_busy = 0;  // number of threads in `read`
    std::mutex m_mutex;  // for both queue and workers
    std::condition_variable m_cv;
};


int main(int argc, const char* argv[])
{
    bool fixed = false;
    bool ignore_case = false;
    bool ungreedy = false;
    bool show_hidden = false;
    bool show_dirs = false;
    bool color = false;
    int jobs = 8;
    std::vector<const char*> files;
    const char* pattern = nullptr;
    ArgParser {
            Option("-F, --fixed", "Match literal string instead of (default) regex", fixed),
            Option("-i, --ignore-case", "Enable case insensitive matching", ignore_case),
            Option("-u, --ungreedy", "Use minimal (non-greedy) repetitions instead of the normal greedy ones", ungreedy),
            Option("-H, --show-hidden", "Don't skip hidden files", show_hidden),
            Option("-D, --show-dirs", "Don't skip directory entries", show_dirs),
            Option("-a, --all", "Don't skip any files, same as -H -D", [&]{ show_hidden = true; show_dirs = true; }),
            Option("-c, --color", "Force color output", color),
            Option("-j, --jobs JOBS", "Number of worker threads", jobs).env("JOBS"),
            Option("-h, --help", "Show help", show_help),
            Option("[PATTERN]", "File name pattern (Perl-style regex)", pattern),
            Option("-- FILE ...", "Files and/or directories to scan", files),
    } (argv);

#ifndef NDEBUG
    cout << "OK: hidden=" << boolalpha << show_hidden << endl;
    cout << "    jobs=" << jobs << endl;
    cout << "    pattern: " << (pattern ? pattern : "[not given]") << endl;
    cout << "    files:";
    for (const auto& f : files)
        cout << ' ' << f << ';';
    if (files.empty())
        cout << " [not given]";
    cout << endl;
#endif

    TermCtl& term = TermCtl::stdout_instance(color ? TermCtl::Mode::Always : TermCtl::Mode::Auto);

    regex_t re;
    if (pattern) {
        int flags = fixed ? (REG_LITERAL) : (REG_EXTENDED | REG_ENHANCED);
        if (ignore_case)
            flags |= REG_ICASE;
        if (!fixed && ungreedy)
            flags |= REG_UNGREEDY;
        auto r = regcomp(&re, pattern, flags);
        if (r != 0) {
            size_t size = regerror(r, &re, nullptr, 0);
            std::string buffer (size, '\0');
            regerror(r, &re, buffer.data(), buffer.size());
            fmt::print(stderr,"ff: regcomp({}): {}\n", pattern, buffer);
            exit(1);
        }
    }

    struct Theme {
        std::string normal;
        std::string dir;
        std::string dir_last;
        std::string file;
        std::string highlight;
    };

    Theme theme {
        .normal = term.normal().seq(),
        .dir = term.magenta().seq(),
        .dir_last = term.cyan().seq(),
        .file = term.normal().seq(),
        .highlight = term.bold().yellow().seq(),
    };

    FileTree ft(jobs-1, jobs, [show_hidden, show_dirs, pattern, &re, &theme](const FileTree::PathNode& path, FileTree::Type t) {
        if (!show_hidden && path.component[0] == '.')
            return false;
        switch (t) {
            case FileTree::Directory:
                if (!show_dirs)
                    return true;
                FALLTHROUGH;
            case FileTree::File:
                if (pattern) {
                    regmatch_t m;
                    auto* name = path.component.c_str();
                    auto r = regexec(&re, name, 1, &m, 0);
                    if (r == REG_NOMATCH)
                        return true;  // not matched
                    if (r != 0) {
                        size_t size = regerror(r, &re, nullptr, 0);
                        std::string buffer (size, '\0');
                        regerror(r, &re, buffer.data(), buffer.size());
                        fmt::print(stderr,"ff: regexec({}): {}\n", pattern, buffer);
                        return true;
                    }
                    std::string out = theme.dir;
                    out += path.dir_to_string();
                    if (t == FileTree::Directory)
                        out += theme.dir_last;
                    else
                        out += theme.file;
                    out += std::string_view(name, m.rm_so);
                    out += theme.highlight;
                    out += std::string_view(name + m.rm_so, m.rm_eo - m.rm_so);
                    if (t == FileTree::Directory)
                        out += theme.dir_last;
                    else
                        out += theme.file;
                    out += std::string_view(name + m.rm_eo);
                    out += theme.normal;
                    puts(out.c_str());
                } else {
                    std::string out = theme.dir;
                    out += path.dir_to_string();
                    if (t == FileTree::Directory)
                        out += theme.dir_last;
                    else
                        out += theme.file;
                    out += path.component;
                    out += theme.normal;
                    puts(out.c_str());
                }
                return true;
            case FileTree::OpenError:
                fmt::print(stderr,"ff: open({}): {}\n", path.to_string(), errno_str());
                return true;
            case FileTree::OpenDirError:
                fmt::print(stderr,"ff: opendir({}): {}\n", path.to_string(), errno_str());
                return true;
            case FileTree::ReadDirError:
                fmt::print(stderr,"ff: readdir({}): {}\n", path.to_string(), errno_str());
                return true;
        }
        UNREACHABLE;
    });
    if (files.empty())
        ft.walk("");
    for (const auto& f : files)
        ft.walk(f);

    ft.worker();

    if (pattern)
        regfree(&re);
    return 0;
}
