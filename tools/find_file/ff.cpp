// ff.cpp created on 2020-03-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Find File (ff) command line tool
/// A find-like tool using Hyperscan for regex matching.

#include <xci/core/ArgParser.h>
#include <xci/core/FileTree.h>
#include <xci/core/container/FlatSet.h>
#include <xci/core/sys.h>
#include <xci/core/string.h>
#include <xci/core/log.h>
#include <xci/core/TermCtl.h>
#include <xci/compat/macros.h>

#include <fmt/core.h>
#include <hs/hs.h>

#include <cstring>
#include <utility>
#include <string_view>

#include <unistd.h>
#include <time.h>

using namespace xci::core;
using namespace xci::core::argparser;


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
        case S_IFBLK:   return 'b';
        case S_IFCHR:   return 'c';
        case S_IFDIR:   return 'd';
        case S_IFIFO:   return 'p';
        case S_IFREG:   return ' ';
        case S_IFLNK:   return 'l';
        case S_IFSOCK:  return 's';
        default:        return '?';
    }
}


template <>
struct [[maybe_unused]] fmt::formatter<timespec> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();  // NOLINT
        if (it != ctx.end() && *it != '}')
            throw fmt::format_error("invalid format for timespec");
        return it;
    }

    template <typename FormatContext>
    auto format(const timespec& ts, FormatContext& ctx) {
        struct tm tm;
        localtime_r(&ts.tv_sec, &tm);
        char buf[100];
        size_t size = strftime(buf, sizeof(buf), "%F %H:%M", &tm);
        return std::copy(buf, buf + size, ctx.out());
    }
};


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


static void print_path_with_attrs(const std::string& name, const FileTree::PathNode& path)
{
    struct stat st;
    if (!path.stat(st)) {
        fmt::print(stderr,"ff: stat({}): {}\n", path.file_name(), errno_str());
        return;
    }
    // Adaptive column width
    auto user = uid_to_user_name(st.st_uid);
    auto group = gid_to_group_name(st.st_gid);
    size_t size = st.st_size;
    size_t unused = st.st_blocks * 512;  // allocated block size
    char size_unit = round_size_to_unit(size);
    char unused_unit = round_size_to_unit(unused);
    static size_t w_user = 0;
    static size_t w_group = 0;
    w_user = std::max(w_user, user.length());
    w_group = std::max(w_group, group.length());
    std::string out = fmt::format("{:c}{:04o} {:{}}:{:{}} {:4}{} {:4}{}  {}  {}",
            file_type_to_char(st.st_mode), st.st_mode & 07777,
            user, w_user, group, w_group,
            size, size_unit, unused, unused_unit,
            st.st_mtimespec,
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


class HyperscanScratch {
public:
    ~HyperscanScratch() { hs_free_scratch(m_scratch); }

    hs_scratch_t* get(const hs_database_t* re_db) {
        // this allocates new scratch space ONLY IF the previous one is NULL or no longer valid
        if (hs_alloc_scratch(re_db, &m_scratch) != HS_SUCCESS) {
            fmt::print(stderr,"ff: hs_alloc_scratch: Unable to allocate scratch space.\n");
            m_scratch = nullptr;
        }
        return m_scratch;
    }

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
    bool show_version = false;
    int jobs = 8;
    std::vector<const char*> files;
    const char* pattern = nullptr;

    TermCtl& term = TermCtl::stdout_instance();

    // enable HS_FLAG_SOM_LEFTMOST only if we have color output
    bool highlight_match = term.is_tty();

    ArgParser {
#ifdef HAVE_HS_COMPILE_LIT
            Option("-F, --fixed", "Match literal string instead of (default) regex", fixed),
#endif
            Option("-i, --ignore-case", "Enable case insensitive matching", ignore_case),
            Option("-H, --search-hidden", "Don't skip hidden files", show_hidden),
            Option("-D, --search-dirnames", "Don't skip directory entries", show_dirs),
            Option("-S, --search-in-special-dirs", "Allow descending into special directories: " + FileTree::default_ignore_list(", "), search_in_special_dirs),
            Option("-X, --single-device", "Don't descend into directories with different device number", single_device),
            Option("-a, --all", "Don't skip any files, same as -HDS", [&]{ show_hidden = true; show_dirs = true; search_in_special_dirs = true; }),
            Option("-l, --long", "Print file attributes", long_form),
            Option("-c, --color", "Force color output (default: auto)", [&term]{ term.set_is_tty(TermCtl::IsTty::Always); }),
            Option("-C, --no-color", "Disable color output (default: auto)", [&term]{ term.set_is_tty(TermCtl::IsTty::Never); }),
            Option("-M, --no-highlight", "Don't highlight matches (default: enabled for color output)", [&highlight_match]{ highlight_match = false; }),
            Option("-j, --jobs JOBS", "Number of worker threads", jobs).env("JOBS"),
            Option("-V, --version", "Show version", show_version),
            Option("-h, --help", "Show help", show_help),
            Option("[PATTERN]", "File name pattern (Perl-style regex)", pattern),
            Option("-- FILE ...", "Files and/or directories to scan", files),
    } (argv);

    if (show_version) {
        term.print("{t:bold}ff{t:normal} {}\n", "0.2");
        term.print("using {t:bold}Hyperscan{t:normal} {}", hs_version());
#ifndef HAVE_HS_COMPILE_LIT
        term.print(" (hs_compile_lit not available, {t:bold}{fg:green}--fixed{t:normal} option disabled)");
#endif
        puts("");
        return 0;
    }

    // empty pattern -> show all files
    if (pattern && *pattern == '\0')
        pattern = nullptr;

    hs_database_t *re_db = nullptr;
    if (pattern) {
        int flags = 0;
        if (ignore_case)
            flags |= HS_FLAG_CASELESS;
        hs_compile_error_t *re_compile_err;
        if (fixed) {
#ifdef HAVE_HS_COMPILE_LIT
            if (highlight_match)
                flags |= HS_FLAG_SOM_LEFTMOST;
            auto rc = hs_compile_lit(pattern, flags,
                    strlen(pattern), HS_MODE_BLOCK, nullptr,
                    &re_db, &re_compile_err);
            if (rc != HS_SUCCESS) {
                fmt::print(stderr,"ff: hs_compile_lit({}): ({}) {}\n", pattern, rc, re_compile_err->message);
                hs_free_compile_error(re_compile_err);
                return 1;
            }
#endif
        } else {
            flags |= HS_FLAG_DOTALL | HS_FLAG_UTF8 | HS_FLAG_UCP;

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
            rc = hs_compile(pattern, flags,
                    HS_MODE_BLOCK, nullptr,
                    &re_db, &re_compile_err);
            if (rc != HS_SUCCESS) {
                fmt::print(stderr,"ff: hs_compile({}): ({}) {}\n", pattern, rc, re_compile_err->message);
                hs_free_compile_error(re_compile_err);
                return 1;
            }
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

    FileTree ft(jobs-1, jobs,
                [show_hidden, show_dirs, single_device, pattern, &re_db, &theme, &dev_ids, &long_form, &highlight_match]
                (const FileTree::PathNode& path, FileTree::Type t)
    {
        if (!show_hidden && path.component[0] == '.')
            return false;
        switch (t) {
            case FileTree::Directory:
                if (!show_dirs)
                    return true;  // descent
                if (single_device) {
                    struct stat st;
                    if (!path.stat(st)) {
                        fmt::print(stderr,"ff: stat({}): {}\n", path.dir_name(), errno_str());
                        return true;
                    }
                    if (path.is_input()) {
                        dev_ids.emplace(st.st_dev);
                    } else if (!dev_ids.contains(st.st_dev)) {
                        return false;  // skip (different device ID)
                    }
                }
                FALLTHROUGH;
            case FileTree::File:
                if (pattern) {
                    std::string out;
                    const auto* name = path.component.c_str();
                    // FIXME: allow thread init/cleanup in FileTree and move this in there
                    thread_local HyperscanScratch re_scratch_alloc;
                    auto* re_scratch = re_scratch_alloc.get(re_db);
                    if (re_scratch == nullptr)
                        return true;

                    if (highlight_match) {
                        // match, with highlight
                        std::vector<std::pair<int, int>> matches;
                        auto r = hs_scan(re_db, name, path.component.size(), 0, re_scratch,
                                [] (unsigned int id, unsigned long long from,
                                    unsigned long long to, unsigned int flags, void *ctx)
                                {
                                    auto* m = static_cast<std::vector<std::pair<int, int>>*>(ctx);
                                    m->emplace_back(from, to);
                                    return 0;
                                }, &matches);
                        if (r != HS_SUCCESS) {
                            fmt::print(stderr,"ff: hs_scan({}): Unable to scan ({})\n", path.component, r);
                            return true;
                        }
                        if (matches.empty())
                            return true;  // not matched
                        const auto so = matches[0].first;
                        const auto eo = matches[0].second;

                        if (t == FileTree::Directory) {
                            out += theme.dir;
                            out += path.parent_dir_name();
                        } else {
                            out += theme.file_dir;
                            out += path.parent_dir_name();
                            out += theme.file_name;
                        }
                        out += std::string_view(name, so);
                        out += theme.highlight;
                        out += std::string_view(name + so, eo - so);
                        if (t == FileTree::Directory)
                            out += theme.dir;
                        else
                            out += theme.file_name;
                        out += std::string_view(name + eo);
                        out += theme.normal;
                        if (long_form)
                            print_path_with_attrs(out, path);
                        else
                            puts(out.c_str());
                        return true;
                    }

                    // match, no highlight
                    auto r = hs_scan(re_db, name, path.component.size(), 0, re_scratch,
                            [] (unsigned int id, unsigned long long from,
                                unsigned long long to, unsigned int flags, void *ctx)
                            { return 1; }, nullptr);
                    // returns HS_SCAN_TERMINATED on match (because callback returns 1)
                    if (r == HS_SUCCESS)
                        return true;  // not matched
                    if (r != HS_SCAN_TERMINATED) {
                        fmt::print(stderr,"ff: hs_scan({}): ({}) Unable to scan\n", path.component, r);
                        return true;
                    }
                    // print path below
                }

                // no matching, just print, without highlight
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
                    out += path.component;
                    out += theme.normal;
                    if (long_form)
                        print_path_with_attrs(out, path);
                    else
                        puts(out.c_str());
                    return true;
                }
            case FileTree::OpenError:
                fmt::print(stderr,"ff: open({}): {}\n", path.dir_name(), errno_str());
                return true;
            case FileTree::OpenDirError:
                fmt::print(stderr,"ff: opendir({}): {}\n", path.dir_name(), errno_str());
                return true;
            case FileTree::ReadDirError:
                fmt::print(stderr,"ff: readdir({}): {}\n", path.dir_name(), errno_str());
                return true;
        }
        UNREACHABLE;
    });

    ft.set_default_ignore(!search_in_special_dirs);

    if (files.empty()) {
        ft.walk_cwd();
    } else {
        for (const char* f : files) {
            ft.walk(f);
        }
    }

    ft.worker();

    hs_free_database(re_db);
    return 0;
}
