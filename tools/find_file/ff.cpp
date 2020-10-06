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

#include <sys/types.h>

using namespace xci::core;
using namespace xci::core::argparser;


int main(int argc, const char* argv[])
{
    bool fixed = false;
    bool ignore_case = false;
    bool show_hidden = false;
    bool show_dirs = false;
    bool search_in_special_dirs = false;
    bool single_device = false;
    bool show_version = false;
    int jobs = 8;
    std::vector<const char*> files;
    const char* pattern = nullptr;

    TermCtl& term = TermCtl::stdout_instance();

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
            Option("-c, --color", "Force color output", [&]{ term.set_is_tty(TermCtl::IsTty::Always); }),
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

    hs_database_t *re_db = nullptr;
    if (pattern) {
        int flags = 0;
        if (ignore_case)
            flags |= HS_FLAG_CASELESS;
        // enable start offset only if we have color output
        if (term.is_tty())
            flags |= HS_FLAG_SOM_LEFTMOST;
        hs_compile_error_t *re_compile_err;
        if (fixed) {
#ifdef HAVE_HS_COMPILE_LIT
            if (hs_compile_lit(pattern, flags,
                    strlen(pattern), HS_MODE_BLOCK, nullptr,
                    &re_db, &re_compile_err) != HS_SUCCESS) {
                fmt::print(stderr,"ff: hs_compile_lit({}): {}\n", pattern, re_compile_err->message);
                hs_free_compile_error(re_compile_err);
                return 1;
            }
#endif
        } else {
            if (hs_compile(pattern, flags | HS_FLAG_DOTALL | HS_FLAG_UTF8 | HS_FLAG_UCP,
                    HS_MODE_BLOCK, nullptr,
                    &re_db, &re_compile_err) != HS_SUCCESS) {
                fmt::print(stderr,"ff: hs_compile({}): {}\n", pattern, re_compile_err->message);
                hs_free_compile_error(re_compile_err);
                return 1;
            }
        }
    }

    struct Theme {
        std::string normal;
        std::string dir;
        std::string file_dir;
        std::string file_name;
        std::string highlight;
    };

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
                [show_hidden, show_dirs, single_device, pattern, &re_db, &theme, &dev_ids]
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
                    const auto* name = path.component.c_str();
                    thread_local hs_scratch_t *re_scratch = nullptr;
                    if (re_scratch == nullptr && hs_alloc_scratch(re_db, &re_scratch) != HS_SUCCESS) {
                        fmt::print(stderr,"ff: hs_alloc_scratch: Unable to allocate scratch space.\n");
                        return true;
                    }
                    // FIXME: thread cleanup: hs_free_scratch(re_scratch);

                    std::vector<std::pair<int, int>> matches;
                    auto r = hs_scan(re_db, path.component.data(), path.component.size(), 0, re_scratch,
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

                    std::string out;
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
                    puts(out.c_str());
                } else {
                    std::string out;
                    if (t == FileTree::Directory) {
                        out += theme.dir;
                        out += path.dir_name();
                    } else {
                        out += theme.file_dir;
                        out += path.parent_dir_name();
                        out += theme.file_name;
                        out += path.component;
                    }
                    out += theme.normal;
                    puts(out.c_str());
                }
                return true;
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
