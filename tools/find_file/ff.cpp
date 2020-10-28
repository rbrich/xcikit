// ff.cpp created on 2020-03-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Find File (ff) command line tool
/// A find-like tool using Hyperscan for regex matching.

#include <xci/core/log.h>  // TRACE for FileTree - must be included first
#include <xci/core/FileTree.h>
#include <xci/core/ArgParser.h>
#include <xci/core/container/FlatSet.h>
#include <xci/core/memoization.h>
#include <xci/core/sys.h>
#include <xci/core/string.h>
#include <xci/core/TermCtl.h>
#include <xci/core/NonCopyable.h>
#include <xci/compat/macros.h>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <hs/hs.h>

#include <cstring>
#include <utility>
#include <string_view>
#include <atomic>

#include <unistd.h>
#include <ctype.h>

using namespace xci::core;
using namespace xci::core::argparser;


static constexpr auto c_version = "0.4";


struct Theme {
    std::string normal;
    std::string dir;
    std::string file_dir;
    std::string file_name;
    std::string highlight;
};


static char file_type_to_char(mode_t mode)
{
    switch (mode & S_IFMT) {
        case S_IFREG:   return (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? '*' : ' ';
        case S_IFDIR:   return '/';
        case S_IFLNK:   return '@';
        case S_IFSOCK:  return '=';
        case S_IFIFO:   return '|';
        case S_IFCHR:   return '-';
        case S_IFBLK:   return '+';
        default:        return '?';
    }
}


static TermCtl::Color file_mode_to_color(mode_t mode)
{
    using C = xci::core::TermCtl::Color;
    switch (mode & S_IFMT) {
        case S_IFREG:   return (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? C::BrightGreen : C::White;
        case S_IFDIR:   return C::BrightCyan;
        case S_IFLNK:   return C::Cyan;
        case S_IFSOCK:  return C::Green;
        case S_IFIFO:   return C::BrightBlue;
        case S_IFCHR:   return C::Magenta;
        case S_IFBLK:   return C::BrightMagenta;
        default:        return C::White;
    }
}


static bool parse_types(const char* arg, mode_t& out_mask)
{
    for (const char* c = arg; *c; ++c) {
        switch (tolower(*c)) {
            case 'r':   out_mask |= S_IFREG; return true;
            case 'd':   out_mask |= S_IFDIR; return true;
            case 'l':   out_mask |= S_IFLNK; return true;
            case 's':   out_mask |= S_IFSOCK; return true;
            case 'f':   out_mask |= S_IFIFO; return true;
            case 'c':   out_mask |= S_IFCHR; return true;
            case 'b':   out_mask |= S_IFBLK; return true;
            default: return false;
        }
    }
    return true;
}


static char next_size_unit(char unit)
{
    switch (unit) {
        case 'B': return 'K';
        case 'K': return 'M';
        case 'M': return 'G';
        case 'G': return 'T';
        case 'T': return 'P';
        default:  return '?';
    }
}


static char round_size_to_unit(size_t& size)
{
    char unit = 'B';
    while (size >= 10'000) {
        size /= 1024;
        unit = next_size_unit(unit);
    }
    return unit;
}


static TermCtl::Color size_unit_to_color(char unit)
{
    using C = xci::core::TermCtl::Color;
    switch (unit) {
        case 'B': return C::Green;
        case 'K': return C::Cyan;
        case 'M': return C::Yellow;
        case 'G': return C::Magenta;
        case 'T': return C::Red;
        case 'P': return C::BrightRed;
        default:  return C::White;
    }
}


static std::string highlight_path(FileTree::Type t, const FileTree::PathNode& path, const Theme& theme,
                                  const int so=0, const int eo=0)
{
    std::string out;
    if (t == FileTree::Directory) {
        out += theme.dir;
        out += path.parent_dir_name();
    } else {
        out += theme.file_dir;
        out += path.parent_dir_name();
        out += theme.file_name;
    }
    if (so != eo) {
        const auto* name = path.component.c_str();
        out += std::string_view(name, so);
        out += theme.highlight;
        out += std::string_view(name + so, eo - so);
        if (t == FileTree::Directory)
            out += theme.dir;
        else
            out += theme.file_name;
        out += std::string_view(name + eo);
    } else {
        out += path.component;
    }
    out += theme.normal;
    return out;
}


static void print_path_with_attrs(const std::string& name, const FileTree::PathNode& path,
                                  struct stat& st)
{
    // Adaptive column width for user, group
    static std::atomic<size_t> w_user = 0;
    static std::atomic<size_t> w_group = 0;
    thread_local auto memoized_uid_to_user_name = memoize(uid_to_user_name);
    thread_local auto memoized_gid_to_group_name = memoize(gid_to_group_name);
    const auto user = memoized_uid_to_user_name(st.st_uid);
    const auto group = memoized_uid_to_user_name(st.st_gid);
    const size_t w_user_new = std::max(w_user.load(std::memory_order_acquire), user.length());
    w_user.store(w_user_new, std::memory_order_release);
    const size_t w_group_new = std::max(w_group.load(std::memory_order_acquire), group.length());
    w_group.store(w_group_new, std::memory_order_release);

    size_t size = st.st_size;
    size_t alloc = st.st_blocks * 512;  // size in allocated blocks
    char size_unit = round_size_to_unit(size);
    char alloc_unit = round_size_to_unit(alloc);
    char file_type = file_type_to_char(st.st_mode);
    TermCtl& term = TermCtl::stdout_instance();
    std::string out = fmt::format("{}{:c}{:04o}{} {:>{}}:{:{}} {}{:4}{}{}{} {}{:4}{}{}{}  {:%F %H:%M}  {}",
            term.fg(file_mode_to_color(st.st_mode)), file_type, st.st_mode & 07777, term.normal(),
            user, w_user, group, w_group,
            term.fg(size_unit_to_color(size_unit)), size, term.dim(), size_unit, term.normal(),
            term.fg(size_unit_to_color(alloc_unit)), alloc, term.dim(), alloc_unit, term.normal(),
            fmt::localtime(st.st_mtime),
            name);
    if (S_ISLNK(st.st_mode)) {
        char buf[PATH_MAX];
        ssize_t res;
        if (path.parent && path.parent->fd != -1)
            res = readlinkat(path.parent->fd, path.component.c_str(), buf, sizeof(buf));
        else
            res = readlink(path.file_name().c_str(), buf, sizeof(buf));
        if (res < 0) {
            fmt::print(stderr,"ff: readlink({}): {}\n", path.file_name(), errno_str());
            return;
        }
        out += fmt::format(" -> {}", std::string_view(buf, res));
    }
    puts(out.c_str());
}


class HyperscanDatabase: public NonCopyable {
public:
    ~HyperscanDatabase() { hs_free_database(m_db); }

    operator bool() const { return m_db != nullptr; }
    operator const hs_database_t *() const { return m_db; }

    void add(const char* pattern, unsigned int flags) {
        m_patterns.emplace_back(pattern);
        m_flags.push_back(flags);
    }

    void add_literal(const char* literal, unsigned int flags) {
        // Escape non-alphanumeric characters in literal to hex
        // See: https://github.com/intel/hyperscan/issues/191
        std::string pattern;
        pattern.reserve(4 * strlen(literal));
        auto it = std::back_inserter(pattern);
        for (const char* ch = literal; *ch; ++ch) {
            if (isalnum(*ch))
                *it++ = *ch;
            else
                it = fmt::format_to(it, "\\x{:02x}", *ch);
        }
        m_patterns.push_back(std::move(pattern));
        m_flags.push_back(flags);
    }

    void add_extension(const char* ext, unsigned int flags) {
        // Match file extension, i.e. '.' + ext at end of filename
        auto pattern = std::string("\\.") + ext + '$';
        m_patterns.push_back(pattern);
        m_flags.push_back(flags);
    }

    bool compile() {
        if (m_patterns.empty())
            return true;
        std::vector<const char*> expressions;
        expressions.reserve(m_patterns.size());
        for (const auto& pat : m_patterns)
            expressions.push_back(pat.c_str());
        hs_compile_error_t *re_compile_err;
        hs_error_t rc = hs_compile_multi(expressions.data(), m_flags.data(),
                nullptr, expressions.size(),
                HS_MODE_BLOCK, nullptr, &m_db, &re_compile_err);
        if (rc != HS_SUCCESS) {
            fmt::print(stderr,"ff: hs_compile_multi: ({}) {}\n", rc, re_compile_err->message);
            hs_free_compile_error(re_compile_err);
            return false;
        }
        return true;
    }

private:
    hs_database_t* m_db = nullptr;
    std::vector<std::string> m_patterns;
    std::vector<unsigned int> m_flags;
};


class HyperscanScratch: public NonCopyable {
public:
    HyperscanScratch(hs_scratch_t* prototype) {
        if (hs_clone_scratch(prototype, &m_scratch) != HS_SUCCESS) {
            fmt::print(stderr,"ff: hs_clone_scratch: Unable to allocate scratch space.\n");
            exit(1);
        }
    }
    ~HyperscanScratch() { hs_free_scratch(m_scratch); }

    operator hs_scratch_t*() { return m_scratch; }

private:
    hs_scratch_t* m_scratch = nullptr;
};


int main(int argc, const char* argv[])
{
    bool fixed = false;
    bool ignore_case = false;
    bool show_hidden = false;
    bool show_dirs = false;
    bool search_in_special_dirs = false;
    bool single_device = false;
    bool long_form = false;
    int max_depth = -1;
    bool show_version = false;
    bool show_stats = false;
    int jobs = 8;
    mode_t type_mask = 0;
    const char* pattern = nullptr;
    std::vector<fs::path> paths;
    std::vector<const char*> extensions;

    TermCtl& term = TermCtl::stdout_instance();

    // enable HS_FLAG_SOM_LEFTMOST only if we have color output
    bool highlight_match = term.is_tty();

    ArgParser {
            Option("-F, --fixed", "Match literal string instead of (default) regex", fixed),
            Option("-i, --ignore-case", "Enable case insensitive matching", ignore_case),
            Option("-e, --ext EXT ...", "Match only files with extension EXT (shortcut for pattern '\\.EXT$')", extensions),
            Option("-H, --search-hidden", "Don't skip hidden files", show_hidden),
            Option("-D, --search-dirnames", "Don't skip directory entries", show_dirs),
            Option("-S, --search-in-special-dirs", "Allow descending into special directories: " + FileTree::default_ignore_list(", "), search_in_special_dirs),
            Option("-X, --single-device", "Don't descend into directories with different device number", single_device),
            Option("-a, --all", "Don't skip any files (alias for -HDS)", [&]{ show_hidden = true; show_dirs = true; search_in_special_dirs = true; }),
            Option("-d, --max-depth N", "Descend at most N directory levels below input directories", max_depth),
            Option("-l, --long", "Print file attributes", long_form),
            Option("-L, --list-long", "Don't descend and print attributes, similar to `ls -l` (alias for -lDd1)", [&]{ long_form = true; show_dirs = true; max_depth = 1; }),
            Option("-t, --types TYPES", "Filter file types: r=regular, d=dir, l=link, s=sock, f=fifo, c=char, b=block, e.g. -tdl for dir+link (implies -D)",
                    [&type_mask, &show_dirs](const char* arg){ show_dirs = true; return parse_types(arg, type_mask); }),
            Option("-c, --color", "Force color output (default: auto)", [&term]{ term.set_is_tty(TermCtl::IsTty::Always); }),
            Option("-C, --no-color", "Disable color output (default: auto)", [&term]{ term.set_is_tty(TermCtl::IsTty::Never); }),
            Option("-M, --no-highlight", "Don't highlight matches (default: enabled for color output)", [&highlight_match]{ highlight_match = false; }),
            Option("-j, --jobs JOBS", "Number of worker threads", jobs).env("JOBS"),
            Option("-s, --stats", "Print statistics (number of searched objects)", show_stats),
            Option("-V, --version", "Show version", show_version),
            Option("-h, --help", "Show help", show_help),
            Option("[PATTERN]", "File name pattern (Perl-style regex)", pattern),
            Option("-- PATH ...", "Paths to search", paths),
    } (argv);

    if (show_version) {
        term.print("{t:bold}ff{t:normal} {}\n", c_version);
        term.print("using {t:bold}Hyperscan{t:normal} {}", hs_version());
        puts("");
        return 0;
    }

    // empty pattern -> show all files
    if (pattern && *pattern == '\0')
        pattern = nullptr;

    HyperscanDatabase re_db;
    hs_scratch_t* re_scratch_prototype = nullptr;
    if (pattern) {
        int flags = HS_FLAG_DOTALL | HS_FLAG_UTF8 | HS_FLAG_UCP;
        if (ignore_case)
            flags |= HS_FLAG_CASELESS;
        hs_compile_error_t *re_compile_err;
        if (fixed) {
            if (highlight_match)
                flags |= HS_FLAG_SOM_LEFTMOST;
            else
                flags |= HS_FLAG_SINGLEMATCH;
            re_db.add_literal(pattern, flags);
        } else {
            // analyze pattern
            hs_expr_info_t* re_info = nullptr;
            hs_error_t rc = hs_expression_info(pattern, flags, &re_info, &re_compile_err);
            if (rc != HS_SUCCESS) {
                fmt::print(stderr,"ff: hs_expression_info({}): ({}) {}\n", pattern, rc, re_compile_err->message);
                hs_free_compile_error(re_compile_err);
                return 1;
            }
            if (re_info->min_width == 0) {
                // Pattern matches empty buffer
                highlight_match = false;
                flags |= HS_FLAG_ALLOWEMPTY;
            }
            free(re_info);

            // compile pattern
            if (highlight_match)
                flags |= HS_FLAG_SOM_LEFTMOST;
            else
                flags |= HS_FLAG_SINGLEMATCH;
            re_db.add(pattern, flags);
        }
    }

    if (!extensions.empty()) {
        int flags = HS_FLAG_DOTALL | HS_FLAG_UTF8 | HS_FLAG_UCP;
        if (ignore_case)
            flags |= HS_FLAG_CASELESS;
        if (highlight_match)
            flags |= HS_FLAG_SOM_LEFTMOST;
        else
            flags |= HS_FLAG_SINGLEMATCH;
        for (const char* ext : extensions) {
            re_db.add_extension(ext, flags);
        }
    }

    if (!re_db.compile())
        return 1;

    if (re_db) {
        // allocate scratch "prototype", to be cloned for each thread
        hs_error_t rc = hs_alloc_scratch(re_db, &re_scratch_prototype);
        if (rc != HS_SUCCESS) {
            fmt::print(stderr,"ff: hs_alloc_scratch: Unable to allocate scratch space.\n");
            return 1;
        }
    }

    // "cyanide"
    Theme theme {
        .normal = term.normal().seq(),
        .dir = term.bold().cyan().seq(),
        .file_dir = term.cyan().seq(),
        .file_name = term.normal().seq(),
        .highlight = term.bold().yellow().seq(),
    };

    FlatSet<dev_t> dev_ids;
    struct {
        std::atomic_uint n_dirs {};
        std::atomic_uint n_files {};
    } counters;

    FileTree ft(jobs-1,
                [show_hidden, show_dirs, single_device, long_form, highlight_match, type_mask, max_depth,
                 &re_db, re_scratch_prototype, &theme, &dev_ids, &counters]
                (const FileTree::PathNode& path, FileTree::Type t)
    {
        switch (t) {
            case FileTree::Directory:
            case FileTree::File: {
                if (!show_hidden && path.component[0] == '.' && (path.component.size() > 2 && path.component[1] != '.'))
                    return false;

                bool descend = true;
                if (t == FileTree::Directory) {
                    counters.n_dirs.fetch_add(1, std::memory_order_relaxed);
                    if (max_depth >= 0 && path.depth >= max_depth) {
                        descend = false;
                    }
                    if (!show_dirs || path.component.empty()) {
                        // path.component is empty when this is root report from walk_cwd()
                        return descend;
                    }
                    if (single_device) {
                        struct stat st;
                        if (!path.stat(st)) {
                            fmt::print(stderr,"ff: stat({}): {}\n", path.dir_name(), errno_str());
                            return descend;
                        }
                        if (path.is_input()) {
                            // This branch is only visited from main thread and it's the only place which writes
                            // A race can occur between this write and the read below, but only when searching
                            // multiple directories. FIXME: avoid the race or synchronize dev_ids
                            dev_ids.emplace(st.st_dev);
                        } else if (!dev_ids.contains(st.st_dev)) {
                            return false;  // skip (different device ID)
                        }
                    }
                } else {
                    counters.n_files.fetch_add(1, std::memory_order_relaxed);
                }

                std::string out;
                if (re_db) {
                    thread_local HyperscanScratch re_scratch(re_scratch_prototype);

                    if (highlight_match) {
                        // match, with highlight
                        std::vector<std::pair<int, int>> matches;
                        auto r = hs_scan(re_db, path.component.c_str(), path.component.size(), 0, re_scratch,
                                [] (unsigned int id, unsigned long long from,
                                    unsigned long long to, unsigned int flags, void *ctx)
                                {
                                    auto* m = static_cast<std::vector<std::pair<int, int>>*>(ctx);
                                    m->emplace_back(from, to);
                                    return 0;
                                }, &matches);
                        if (r != HS_SUCCESS) {
                            fmt::print(stderr,"ff: hs_scan({}): Unable to scan ({})\n", path.component, r);
                            return descend;
                        }
                        if (matches.empty())
                            return descend;  // not matched
                        out = highlight_path(t, path, theme, matches[0].first, matches[0].second);
                    } else {
                        // match, no highlight
                        auto r = hs_scan(re_db, path.component.c_str(), path.component.size(), 0, re_scratch,
                                [] (unsigned int id, unsigned long long from,
                                    unsigned long long to, unsigned int flags, void *ctx)
                                { return 1; }, nullptr);
                        // returns HS_SCAN_TERMINATED on match (because callback returns 1)
                        if (r == HS_SUCCESS)
                            return descend;  // not matched
                        if (r != HS_SCAN_TERMINATED) {
                            fmt::print(stderr,"ff: hs_scan({}): ({}) Unable to scan\n", path.component, r);
                            return descend;
                        }
                        out = highlight_path(t, path, theme);
                    }
                } else {
                    // no matching, just print, without highlight
                    out = highlight_path(t, path, theme);
                }

                struct stat st;
                if (type_mask || long_form) {
                    // need stat
                    if (!path.stat(st)) {
                        fmt::print(stderr,"ff: stat({}): {}\n", path.file_name(), errno_str());
                        return descend;
                    }
                }

                if (type_mask && !(st.st_mode & type_mask))
                    return descend;

                if (long_form)
                    print_path_with_attrs(out, path, st);
                else
                    puts(out.c_str());
                return descend;
            }
            case FileTree::OpenError:
                fmt::print(stderr,"ff: open({}): {}\n", path.file_name(), errno_str());
                return true;
            case FileTree::OpenDirError:
                fmt::print(stderr,"ff: opendir({}): {}\n", path.file_name(), errno_str());
                return true;
            case FileTree::ReadDirError:
                fmt::print(stderr,"ff: readdir({}): {}\n", path.file_name(), errno_str());
                return true;
        }
        UNREACHABLE;
    });

    ft.set_default_ignore(!search_in_special_dirs);

    if (paths.empty()) {
        ft.walk_cwd();
    } else {
        for (const auto& path : paths) {
            ft.walk(path);
        }
    }

    ft.main_worker();

    if (show_stats)
        fmt::print(stderr, "--> searched {} directories, {} files\n", counters.n_dirs, counters.n_files);

    hs_free_scratch(re_scratch_prototype);
    return 0;
}
