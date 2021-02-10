// ff.cpp created on 2020-03-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Find File (ff) command line tool
/// A find-like tool using Hyperscan for regex matching.

#include <xci/core/FileTree.h>
#include <xci/core/ArgParser.h>
#include <xci/core/container/FlatSet.h>
#include <xci/core/memoization.h>
#include <xci/core/sys.h>
#include <xci/core/TermCtl.h>
#include <xci/core/NonCopyable.h>
#include <xci/compat/macros.h>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/ostream.h>
#include <hs/hs.h>

#include <cstring>
#include <utility>
#include <string_view>
#include <atomic>
#include <charconv>

#include <unistd.h>
#include <ctype.h>

using namespace xci::core;
using namespace xci::core::argparser;


static constexpr auto c_version = "0.6";


struct Theme {
    std::string normal;
    std::string dir;
    std::string file_dir;
    std::string file_name;
    std::string highlight;
    std::string grep_highlight;
    std::string grep_lineno;
};


struct Counters {
    std::atomic_uint64_t total_size {};
    std::atomic_uint64_t total_blocks {};
    std::atomic_uint seen_dirs {};
    std::atomic_uint seen_files {};
    std::atomic_uint matched_dirs {};
    std::atomic_uint matched_files {};
};


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

static bool is_default_ignored(std::string_view path)
{
    return std::any_of(std::begin(s_default_ignore_list), std::end(s_default_ignore_list),
            [&path](const char* ignore_path) { return path == ignore_path; });
}

static std::string default_ignore_list(const char* sep)
{
    return fmt::format("{}", fmt::join(std::begin(s_default_ignore_list), std::end(s_default_ignore_list), sep));
}


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
            // the symbols are accepted because they represent file types in `-l` output
            // 'r' (for regular) is accepted because why not ('f' is used by find)
            case 'x': case '*':   out_mask |= (S_IXUSR | S_IXGRP | S_IXOTH); break;
            case 'r':
            case 'f': case ' ':   out_mask |= S_IFREG; break;
            case 'd': case '/':   out_mask |= S_IFDIR; break;
            case 'l': case '@':   out_mask |= S_IFLNK; break;
            case 's': case '=':   out_mask |= S_IFSOCK; break;
            case 'p': case '|':   out_mask |= S_IFIFO; break;
            case 'c': case '-':   out_mask |= S_IFCHR; break;
            case 'b': case '+':   out_mask |= S_IFBLK; break;
            default:
                return false;
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


static int size_unit_to_shift(char unit)
{
    switch (toupper(unit)) {
        case 'B': return 0;
        case 'K': return 10;
        case 'M': return 20;
        case 'G': return 30;
        case 'T': return 40;
        case 'P': return 50;
        default:  return -1;
    }
}


static bool parse_size_filter(const char* arg, size_t& size_from, size_t& size_to)
{
    const char* end = arg + strlen(arg);
    auto res = std::from_chars(arg, end, size_from);
    arg = res.ptr;
    int shift;
    if (size_from != 0 && (shift = size_unit_to_shift(*arg)) != -1) {
        size_from <<= shift;
        ++arg;
    }
    if (arg == end)
        return true;
    if (end - arg < 2 || arg[0] != '.' || arg[1] != '.') {
        fmt::print(stderr,"ff: error parsing size at {}: expected '..'\n", arg);
        return false;
    }
    arg += 2;
    res = std::from_chars(arg, end, size_to);
    arg = res.ptr;
    if (size_to != 0 && (shift = size_unit_to_shift(*arg)) != -1) {
        size_to <<= shift;
        ++arg;
    }
    if (arg != end) {
        fmt::print(stderr,"ff: error parsing size at {}: unexpected characters\n", arg);
        return false;
    }
    if (size_to != 0 && size_to < size_from) {
        fmt::print(stderr,"ff: invalid range, min is greater than max: {} .. {}\n", size_from, size_to);
        return false;
    }
    return (arg == end);
}


static void highlight_path(std::string& out, FileTree::Type t, const FileTree::PathNode& path, const Theme& theme,
                           const int so=0, const int eo=0)
{
    out.reserve(path.size() + 30);  // reserve some space also for escape sequences
    if (t == FileTree::Directory) {
        out += theme.dir;
        out += path.parent_dir_path();
    } else {
        out += theme.file_dir;
        out += path.parent_dir_path();
        out += theme.file_name;
    }
    if (so != eo) {
        std::string_view name = path.name();
        out += name.substr(0, so);
        out += theme.highlight;
        out += name.substr(so, eo - so);
        if (t == FileTree::Directory)
            out += theme.dir;
        else
            out += theme.file_name;
        out += name.substr(eo);
    } else {
        out += path.name();
    }
    out += theme.normal;
}


struct ScanFileBuffers {
    // Two buffers, each is a moving window into the file
    // Buffer 1 always has lower offset than buffer 2
    struct Buffer {
        const char* data;
        size_t size;
        size_t offset;  // offset of the buffer from beginning of the stream
    };
    Buffer buffer[2];
};


struct GrepContext {
    const Theme& theme;
    size_t last_end = 0;  // offset to end of last match or newline
    int lineno = 1;

    enum State {
        CountLines,
        PrintMatch,
    };
    State state = CountLines;

    void print_line_number(std::string& out)
    {
        out += theme.grep_lineno;
        out += std::to_string(lineno);
        out += ':';
        out += theme.normal;
    }

    void highlight_line(std::string& out, const ScanFileBuffers& bufs,
                        const size_t from, const size_t to)
    {
        if (state == CountLines)
            print_line_number(out);
        state = PrintMatch;

        // print everything from last end (newline or last match) upto current match start
        const auto& buf0 = bufs.buffer[0];
        const auto& buf1 = bufs.buffer[1];
        if (last_end < buf0.offset) {
            // we've missed the start of line - it's not in the previous buffer
            out += theme.grep_lineno;
            out += "...";
            out += theme.normal;
            last_end = buf0.offset;
        }

        if (last_end < buf1.offset) {
            const auto end0 = last_end - buf0.offset;
            if (from < buf1.offset) {
                // special case - the match is split between buffers
                out.append(buf0.data + end0, buf0.size - end0 - (buf1.offset - from));
                out += theme.grep_highlight;
                const auto from0 = from - buf0.offset;
                out.append(buf0.data + from0, buf0.size - from0);
                out.append(buf1.data, to - buf1.offset);
                out += theme.normal;
                last_end = to;
                return;
            } else {
                out.append(buf0.data + end0, buf0.size - end0);
            }
            last_end = buf1.offset;
        }

        out.append(buf1.data + last_end - buf1.offset, from - last_end);

        // print the highlighted match
        out += theme.grep_highlight;
        out.append(buf1.data + from - buf1.offset, to - from);
        out += theme.normal;

        last_end = to;
    }

    void finish_line(std::string& out, const ScanFileBuffers& bufs, uint32_t end)
    {
        if (state == PrintMatch) {
            const auto& buf1 = bufs.buffer[1];
            out.append(buf1.data + last_end - buf1.offset, end - last_end);
            state = CountLines;
        }
        last_end = end;
        ++ lineno;
    }

    void finish_buffer0(std::string& out, const ScanFileBuffers& bufs)
    {
        if (state == PrintMatch) {
            // print rest of previous buffer
            const auto& buf0 = bufs.buffer[0];
            const auto& buf1 = bufs.buffer[1];
            if (last_end < buf1.offset) {
                const auto end0 = last_end - buf0.offset;
                out.append(buf0.data + end0, buf0.size - end0);
                last_end = buf1.offset;
            }
        }
    }

    void finish_buffer1(std::string& out, const ScanFileBuffers& bufs)
    {
        if (state == PrintMatch) {
            const auto& buf1 = bufs.buffer[1];
            if (last_end < buf1.offset + buf1.size) {
                const auto end1 = last_end - buf1.offset;
                out.append(buf1.data + end1, buf1.size - end1);
                last_end = buf1.offset + buf1.size;
            }
        }
    }
};


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
    fmt::print(std::cout, "{}{:c}{:04o}{} {:>{}}:{:{}} {}{:4}{}{}{} {}{:4}{}{}{}  {:%F %H:%M}  ",
            term.fg(file_mode_to_color(st.st_mode)), file_type, st.st_mode & 07777, term.normal(),
            user, w_user, group, w_group,
            term.fg(size_unit_to_color(size_unit)), size, term.dim(), size_unit, term.normal(),
            term.fg(size_unit_to_color(alloc_unit)), alloc, term.dim(), alloc_unit, term.normal(),
            fmt::localtime(st.st_mtime));
    std::cout << name;
    if (S_ISLNK(st.st_mode)) {
        std::string target;
        if (!path.readlink(target)) {
            fmt::print(stderr,"ff: readlink({}): {}\n", path.file_path(), errno_str());
            return;
        }
        fmt::print(std::cout, " -> {}", target);
    }
}


static void print_stats(const Counters& counters)
{
    TermCtl& term = TermCtl::stdout_instance();
    fmt::print(stderr, "----------------------------------------------\n"
                       " Searched {:8} directories {:8} files\n"
                       " Matched  {:8} directories {:8} files\n",
            counters.seen_dirs, counters.seen_files, counters.matched_dirs, counters.matched_files);
    if (counters.total_size || counters.total_blocks) {
        size_t size = counters.total_size;
        size_t alloc = counters.total_blocks * 512;
        char size_unit = round_size_to_unit(size);
        char alloc_unit = round_size_to_unit(alloc);
        fmt::print(stderr,
                " Size    {}{:8}{}{:c}{} total  {}{:8}{}{:c}{} allocated\n",
                term.fg(size_unit_to_color(size_unit)), size, term.dim(), size_unit, term.normal(),
                term.fg(size_unit_to_color(alloc_unit)), alloc, term.dim(), alloc_unit, term.normal());
    }
    fmt::print(stderr, "----------------------------------------------\n");
}


class HyperscanScratch: public NonCopyable {
public:
    ~HyperscanScratch() { hs_free_scratch(m_scratch); }

    bool reallocate_for(hs_database_t* db) {
        hs_error_t rc = hs_alloc_scratch(db, &m_scratch);
        if (rc != HS_SUCCESS) {
            fmt::print(stderr,"ff: hs_alloc_scratch: Unable to allocate scratch space.\n");
            return false;
        }
        return true;
    }

    bool clone(hs_scratch_t* prototype) {
        if (hs_clone_scratch(prototype, &m_scratch) != HS_SUCCESS) {
            fmt::print(stderr,"ff: hs_clone_scratch: Unable to allocate scratch space.\n");
            return false;
        }
        return true;
    }

    operator hs_scratch_t*() const { return m_scratch; }

private:
    hs_scratch_t* m_scratch = nullptr;
};


class HyperscanDatabase: public NonCopyable {
public:
    ~HyperscanDatabase() { hs_free_database(m_db); }

    operator bool() const { return m_db != nullptr; }
    operator const hs_database_t *() const { return m_db; }

    void add(const char* pattern, unsigned int flags, unsigned int id=0) {
        m_patterns.emplace_back(pattern);
        m_flags.push_back(flags);
        m_ids.push_back(id);
    }

    void add_literal(const char* literal, unsigned int flags, unsigned int id=0) {
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
        m_ids.push_back(id);
    }

    void add_extension(const char* ext, unsigned int flags, unsigned int id=0) {
        // Match file extension, i.e. '.' + ext at end of filename
        auto pattern = std::string("\\.") + ext + '$';
        m_patterns.push_back(pattern);
        m_flags.push_back(flags);
        m_ids.push_back(id);
    }

    bool allocate_scratch(HyperscanScratch& scratch) {
        if (!m_db)
            return true;
        return scratch.reallocate_for(m_db);
    }

    bool compile(unsigned int mode) {
        if (m_patterns.empty())
            return true;
        std::vector<const char*> expressions;
        expressions.reserve(m_patterns.size());
        for (const auto& pat : m_patterns)
            expressions.push_back(pat.c_str());
        hs_compile_error_t *re_compile_err;
        hs_error_t rc = hs_compile_multi(expressions.data(), m_flags.data(),
                m_ids.data(), expressions.size(),
                mode, nullptr, &m_db, &re_compile_err);
        if (rc != HS_SUCCESS) {
            fmt::print(stderr,"ff: hs_compile_multi: ({}) {}\n", rc, re_compile_err->message);
            hs_free_compile_error(re_compile_err);
            return false;
        }
        return true;
    }

    struct ScanResult {
        bool read_ok;
        hs_error_t hs_result = HS_INVALID;
    };

    // id == 10 is special pattern for matching newlines
    // The reader maintains two buffers, so the previous one can be saved
    // and used together with current one to complete lines that span through buffer boundary.
    // \return 1 to stop matching, 0 to continue
    using ScanFileCallback = std::function<int(const ScanFileBuffers& bufs,
                                               unsigned int id, uint64_t from, uint64_t to)>;

    ScanResult scan_file(const FileTree::PathNode& path, hs_scratch_t* scratch,
                         const ScanFileCallback& cb) {
        int fd = path.open();
        if (fd == -1) {
            if (errno != ELOOP)  // O_NOFOLLOW -> it's a symlink
                fmt::print(stderr,"ff: open({}): {}\n", path.file_path(), errno_str());
            return {false};
        }

        struct Context {
            const ScanFileCallback& cb;
            ScanFileBuffers bufs;
        };
        Context ctx { .cb = cb };

        auto handler = [] (unsigned int id, unsigned long long from,
                           unsigned long long to, unsigned int flags, void *ctx_p) {
            auto& ctx = *static_cast<struct Context*>(ctx_p);
            return ctx.cb(ctx.bufs, id, from, to);
        };

        constexpr size_t bufsize = 4096;
        char buffers[2][bufsize];
        unsigned int current_buffer = 1u;

        hs_stream_t* hs_stream;
        auto r = hs_open_stream(m_db, 0, &hs_stream);
        while (r == HS_SUCCESS) {
            // Read into the other buffer
            current_buffer ^= 1u;
            // Blocking read - not optimal
            auto size = ::read(fd, buffers[current_buffer], bufsize);
            if (size == -1) {
                fmt::print(stderr,"ff: read({}): {}\n", path.file_path(), errno_str());
                ::close(fd);
                hs_close_stream(hs_stream, scratch, nullptr, nullptr);
                return {false};
            }
            if (size == 0)
                break;

            // Pass the buffers to the callback
            auto& buf0 = ctx.bufs.buffer[0];
            auto& buf1 = ctx.bufs.buffer[1];
            buf0 = buf1;
            buf1.data = buffers[current_buffer];
            buf1.size = size;
            buf1.offset += buf0.size;

            r = hs_scan_stream(hs_stream, buffers[current_buffer], unsigned(size),
                    0, scratch, handler, &ctx);

            // notify: swapping buffers
            (void) ctx.cb(ctx.bufs, 11, 0, 0);
        }
        if (r == HS_SUCCESS) {
            r = hs_close_stream(hs_stream, scratch, handler, &ctx);
            // notify: end of stream
            (void) ctx.cb(ctx.bufs, 12, 0, 0);
        } else {
            // no longer interested in matches or errors, just close it
            hs_close_stream(hs_stream, scratch, nullptr, nullptr);
        }

        ::close(fd);
        return {true, r};
    }

private:
    hs_database_t* m_db = nullptr;
    std::vector<std::string> m_patterns;
    std::vector<unsigned int> m_flags;
    std::vector<unsigned int> m_ids;
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
    bool grep_mode = false;
    bool silent_grep = false;
    int jobs = 2 * cpu_count();
    size_t size_from = 0;
    size_t size_to = 0;
    mode_t type_mask = 0;
    const char* pattern = nullptr;
    const char* grep_pattern = nullptr;
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
            Option("-S, --search-in-special-dirs", "Allow descending into special directories: " + default_ignore_list(", "), search_in_special_dirs),
            Option("-X, --single-device", "Don't descend into directories with different device number", single_device),
            Option("-a, --all", "Don't skip any files (alias for -HDS)", [&]{ show_hidden = true; show_dirs = true; search_in_special_dirs = true; }),
            Option("-d, --max-depth N", "Descend at most N directory levels below input directories", max_depth),
            Option("-l, --long", "Print file attributes", long_form),
            Option("-L, --list-long", "Don't descend and print attributes, similar to `ls -l` (alias for -lDd1)", [&]{ long_form = true; show_dirs = true; max_depth = 1; }),
            Option("-t, --types TYPES", "Filter file types: f=regular, d=dir, l=link, s=sock, p=fifo, c=char, b=block, x=exec, e.g. -tdl for dir+link (implies -D)",
                    [&type_mask, &show_dirs](const char* arg){ show_dirs = true; return parse_types(arg, type_mask); }),
            Option("--size BETWEEN", "Filter files by size: [MIN]..[MAX], each site is optional, e.g. 1M..2M, 42K (eq. 42K..), ..1G",
                    [&size_from, &size_to](const char* arg){ return parse_size_filter(arg, size_from, size_to); }),
            Option("-g, --grep PATTERN", "Filter files by content, i.e. \"grep\"", grep_pattern),
            Option("-G, --grep-mode", "Switch to grep mode (positional arg PATTERN is searched in content instead of file names)", grep_mode),
            Option("-Q, --silent-grep", "Grep: filter files, don't show matched lines. Stops of first match, making it faster.", silent_grep),
            Option("-c, --color", "Force color output (default: auto)", [&term]{ term.set_is_tty(TermCtl::IsTty::Always); }),
            Option("-C, --no-color", "Disable color output (default: auto)", [&term]{ term.set_is_tty(TermCtl::IsTty::Never); }),
            Option("-M, --no-highlight", "Don't highlight matches (default: enabled for color output)", [&highlight_match]{ highlight_match = false; }),
            Option("-j, --jobs JOBS", fmt::format("Number of worker threads (default: 2*ncpu = {})", jobs), jobs).env("JOBS"),
            Option("-s, --stats", "Print statistics (number of searched objects)", show_stats),
            Option("-V, --version", "Show version", show_version),
            Option("-h, --help", "Show help", show_help),
            Option("[PATTERN]", "File name pattern (Perl-style regex)", pattern),
            Option("-- PATH ...", "Paths to search", paths),
    } (argv);

    if (show_version) {
        term.print("{t:bold}ff{t:normal} {}\n", c_version);
        term.print("using {t:bold}Hyperscan{t:normal} {}\n", hs_version());
        return 0;
    }

    if (grep_mode) {
        grep_pattern = pattern;
        pattern = nullptr;
    }

    // empty pattern -> show all files
    if (pattern && *pattern == '\0')
        pattern = nullptr;

    HyperscanDatabase re_db;
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

            // add pattern
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

    if (!re_db.compile(HS_MODE_BLOCK))
        return 1;

    HyperscanDatabase grep_db;
    if (grep_pattern) {
        int flags = HS_FLAG_UTF8 | HS_FLAG_UCP;
        if (ignore_case)
            flags |= HS_FLAG_CASELESS;
        if (highlight_match)
            flags |= HS_FLAG_SOM_LEFTMOST;
        else
            flags |= HS_FLAG_SINGLEMATCH;
        hs_compile_error_t *re_compile_err;
        // count newlines
        grep_db.add_literal("\n", flags, 10);
        if (fixed) {
            grep_db.add_literal(grep_pattern, flags);
        } else {
            // analyze pattern
            hs_expr_info_t* re_info = nullptr;
            hs_error_t rc = hs_expression_info(grep_pattern, flags, &re_info, &re_compile_err);
            if (rc != HS_SUCCESS) {
                fmt::print(stderr,"ff: hs_expression_info({}): ({}) {}\n", grep_pattern, rc, re_compile_err->message);
                hs_free_compile_error(re_compile_err);
                return 1;
            }
            if (re_info->min_width == 0) {
                // Pattern matches empty buffer
                fmt::print(stderr,"ff: grep pattern matches empty buffer: {}\n", grep_pattern);
                hs_free_compile_error(re_compile_err);
                free(re_info);
                return 1;
            }
            free(re_info);

            // add pattern
            grep_db.add(grep_pattern, flags);
        }
    }

    if (!grep_db.compile(HS_MODE_STREAM | HS_MODE_SOM_HORIZON_MEDIUM))
        return 1;

    // allocate scratch "prototype", to be cloned for each thread
    std::vector<HyperscanScratch> re_scratch(jobs);
    if (re_db || grep_db) {
        // prototype for main thread
        if (!re_db.allocate_scratch(re_scratch[0]) || !grep_db.allocate_scratch(re_scratch[0]))
            return 1;
        // clone for other threads
        for (int i = 1; i != jobs; ++i) {
            if (!re_scratch[i].clone(re_scratch[0]))
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
        .grep_highlight = term.bold().red().seq(),
        .grep_lineno = term.green().seq(),
    };

    FlatSet<dev_t> dev_ids;
    Counters counters;

    FileTree ft(jobs-1,
                [show_hidden, show_dirs, single_device, long_form, highlight_match,
                 type_mask, size_from, size_to, max_depth, search_in_special_dirs, silent_grep,
                 &re_db, &grep_db, &re_scratch, &theme, &dev_ids, &counters]
                (int tn, const FileTree::PathNode& path, FileTree::Type t)
    {
        switch (t) {
            case FileTree::Directory:
            case FileTree::File: {
                if (t == FileTree::Directory) {
                    counters.seen_dirs.fetch_add(1, std::memory_order_relaxed);
                } else {
                    counters.seen_files.fetch_add(1, std::memory_order_relaxed);
                }

                // skip hidden files (".", ".." not considered hidden - if passed as PATH arg)
                if (!show_hidden && path.is_hidden() && !path.is_dots_entry())
                    return false;

                bool descend = true;
                if (t == FileTree::Directory) {
                    if (max_depth >= 0 && path.depth() >= max_depth) {
                        descend = false;
                    }
                    // Check ignore list
                    if (!search_in_special_dirs && is_default_ignored(path.file_path())) {
                        descend = false;
                    }
                    if (!show_dirs || path.name_empty()) {
                        // path.component is empty when this is root report from walk_cwd()
                        counters.seen_dirs.fetch_sub(1, std::memory_order_relaxed);  // small correction - don't count implicitly searched CWD
                        return descend;
                    }
                    if (single_device) {
                        struct stat st;
                        if (!path.stat(st)) {
                            fmt::print(stderr,"ff: stat({}): {}\n", path.file_path(), errno_str());
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
                }

                std::string out;
                if (re_db) {
                    if (highlight_match) {
                        // match, with highlight
                        std::vector<std::pair<int, int>> matches;
                        auto r = hs_scan(re_db, path.name_data(), path.name_len(), 0, re_scratch[tn],
                                [] (unsigned int id, unsigned long long from,
                                    unsigned long long to, unsigned int flags, void *ctx)
                                {
                                    auto* m = static_cast<std::vector<std::pair<int, int>>*>(ctx);
                                    m->emplace_back(from, to);
                                    return 0;
                                }, &matches);
                        if (r != HS_SUCCESS) {
                            fmt::print(stderr,"ff: hs_scan({}): Unable to scan ({})\n", path.name(), r);
                            return descend;
                        }
                        if (matches.empty())
                            return descend;  // not matched
                        highlight_path(out, t, path, theme, matches[0].first, matches[0].second);
                    } else {
                        // match, no highlight
                        auto r = hs_scan(re_db, path.name_data(), path.name_len(), 0, re_scratch[tn],
                                [] (unsigned int id, unsigned long long from,
                                    unsigned long long to, unsigned int flags, void *ctx)
                                {
                                    // stop scanning on first match
                                    return 1;
                                }, nullptr);
                        // returns HS_SCAN_TERMINATED on match (because callback returns 1)
                        if (r == HS_SUCCESS)
                            return descend;  // not matched
                        if (r != HS_SCAN_TERMINATED) {
                            fmt::print(stderr,"ff: hs_scan({}): ({}) Unable to scan\n", path.name(), r);
                            return descend;
                        }
                        highlight_path(out, t, path, theme);
                    }
                } else {
                    // no matching, just print, without highlight
                    highlight_path(out, t, path, theme);
                }

                struct stat st;
                if (long_form || type_mask || size_from || size_to) {
                    // need stat
                    if (!path.stat(st)) {
                        fmt::print(stderr,"ff: stat({}): {}\n", path.file_path(), errno_str());
                        return descend;
                    }
                    counters.total_size.fetch_add(st.st_size, std::memory_order_relaxed);
                    counters.total_blocks.fetch_add(st.st_blocks, std::memory_order_relaxed);
                }

                if (type_mask) {
                    // match type
                    if (!(st.st_mode & type_mask & S_IFMT))
                        return descend;
                    // match rights
                    if ((type_mask & 07777) && !(st.st_mode & type_mask & 07777))
                        return descend;
                }

                if (size_from && size_t(st.st_size) < size_from)
                    return descend;
                if (size_to && size_t(st.st_size) > size_to)
                    return descend;

                if (t == FileTree::Directory) {
                    counters.matched_dirs.fetch_add(1, std::memory_order_relaxed);
                } else {
                    counters.matched_files.fetch_add(1, std::memory_order_relaxed);
                }

                std::string content;
                if (t == FileTree::File && grep_db) {
                    bool matched = false;
                    GrepContext ctx {.theme = theme};
                    auto [read_ok, hs_res] = grep_db.scan_file(path, re_scratch[tn],
                            [silent_grep, &matched, &content, &ctx]
                            (const ScanFileBuffers& bufs, unsigned id, uint32_t from, uint32_t to)
                            {
                                // id  0    -> match
                                // id 10    -> newline pattern, for counting lines
                                // id 11    -> finish buffer
                                // id 12    -> end of stream

                                if (silent_grep) {
                                    // stop if a match was found
                                    return id == 0 ? 1 : 0;
                                }

                                switch (id) {
                                    // match found
                                    case 0:
                                        ctx.highlight_line(content, bufs, from, to);
                                        matched = true;
                                        return 0;

                                    // newline found
                                    case 10:
                                        // special newline pattern, for counting lines
                                        ctx.finish_buffer0(content, bufs);
                                        ctx.finish_line(content, bufs, to);
                                        return 0;

                                    // buffers will be swapped
                                    case 11:
                                        // there are two buffers, new data are read to the other buffer
                                        ctx.finish_buffer0(content, bufs);
                                        return 0;

                                    // end of stream
                                    case 12:
                                        ctx.finish_buffer0(content, bufs);
                                        ctx.finish_buffer1(content, bufs);
                                        return 0;

                                    default:
                                        assert(!"pattern ID not handled");
                                        return 0;
                                }
                            });
                    if (!read_ok)
                        return false;
                    if ((silent_grep && hs_res == HS_SUCCESS) || (!silent_grep && !matched))
                        return false;  // not matched
                    if ((silent_grep && hs_res != HS_SCAN_TERMINATED) || (!silent_grep && hs_res != HS_SUCCESS)) {
                        fmt::print(stderr,"ff: {}: scan failed ({})\n", path.name(), hs_res);
                        return false;
                    }
                }
                flockfile(stdout);
                if (long_form)
                    print_path_with_attrs(out, path, st);
                else
                    std::cout << out;
                std::cout << '\n';
                if (!content.empty()) {
                    std::cout << content << '\n';
                }
                funlockfile(stdout);
                return descend;
            }
            case FileTree::OpenError:
                fmt::print(stderr,"ff: open({}): {}\n", path.file_path(), errno_str());
                return true;
            case FileTree::OpenDirError:
                fmt::print(stderr,"ff: opendir({}): {}\n", path.file_path(), errno_str());
                return true;
            case FileTree::ReadDirError:
                fmt::print(stderr,"ff: readdir({}): {}\n", path.file_path(), errno_str());
                return true;
        }
        UNREACHABLE;
    });

    if (paths.empty()) {
        ft.walk_cwd();
    } else {
        for (const auto& path : paths) {
            ft.walk(path);
        }
    }

    ft.main_worker();

    if (show_stats) {
        print_stats(counters);
    }

    return 0;
}
