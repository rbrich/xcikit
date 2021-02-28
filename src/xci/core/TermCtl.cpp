// TermCtl.cpp created on 2018-07-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// References:
// - https://en.wikipedia.org/wiki/POSIX_terminal_interface
// - https://en.wikipedia.org/wiki/ANSI_escape_code
// - https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

#include "TermCtl.h"
#include <xci/compat/unistd.h>
#include <xci/compat/macros.h>
#include <xci/core/string.h>
#include <xci/config.h>

#ifdef _WIN32
    #include <windows.h>
    static_assert(sizeof(unsigned long) == sizeof(DWORD));
#else
    #include <termios.h>
    #include <sys/ioctl.h>
#endif

//#define XCI_WITH_TINFO
#ifdef XCI_WITH_TINFO
    #include <term.h>
#endif

#include <fmt/format.h>
#include <cassert>
#include <array>

namespace xci::core {

#define ESC     "\033"
#define CSI     ESC "["
#define SS3     ESC "O"

// When building without TInfo, emit ANSI escape sequences directly
#ifndef XCI_WITH_TINFO
static constexpr auto cursor_up = CSI "A";
static constexpr auto cursor_down = CSI "B";
static constexpr auto cursor_right = CSI "C";
static constexpr auto cursor_left = CSI "D";
static constexpr auto enter_bold_mode = CSI "1m";
static constexpr auto enter_dim_mode = CSI "2m";
static constexpr auto enter_underline_mode = CSI "4m";
static constexpr auto exit_attribute_mode = CSI "0m";
static constexpr auto set_a_foreground = CSI "3{}m";
static constexpr auto set_a_background = CSI "4{}m";
static constexpr auto parm_up_cursor = CSI "{}A";  // move cursor up N lines
static constexpr auto parm_down_cursor = CSI "{}B";  // move cursor down N lines
static constexpr auto parm_right_cursor = CSI "{}C";  // move cursor right N spaces
static constexpr auto parm_left_cursor = CSI "{}D";  // move cursor left N spaces
static constexpr auto clr_eos = CSI "J";  // clear screen from cursor down
static constexpr auto clr_eol = CSI "K";  // clear line from cursor to end
static constexpr auto column_address = CSI "{}G";  // horizontal position, absolute

// input sequences:
static constexpr auto key_enter = "\r";
static constexpr auto tab = "\t";
static constexpr auto key_backspace = "\b";
static constexpr auto key_ic = CSI "2~";    // insert character
static constexpr auto key_dc = CSI "3~";    // delete character
static constexpr auto key_home = CSI "H";
static constexpr auto key_end = CSI "F";
static constexpr auto key_ppage = CSI "5~";   // prev page (PgUp)
static constexpr auto key_npage = CSI "6~";   // next page (PgDown)
static constexpr auto key_up = CSI "A";
static constexpr auto key_down = CSI "B";
static constexpr auto key_right = CSI "C";
static constexpr auto key_left = CSI "D";
static constexpr auto key_f1 = SS3 "P";
static constexpr auto key_f2 = SS3 "Q";
static constexpr auto key_f3 = SS3 "R";
static constexpr auto key_f4 = SS3 "S";
static constexpr auto key_f5 = CSI "15~";
static constexpr auto key_f6 = CSI "17~";
static constexpr auto key_f7 = CSI "18~";
static constexpr auto key_f8 = CSI "19~";
static constexpr auto key_f9 = CSI "20~";
static constexpr auto key_f10 = CSI "21~";
static constexpr auto key_f11 = CSI "23~";
static constexpr auto key_f12 = CSI "24~";

inline constexpr const char* tparm(const char* seq) { return seq; }
template<typename ...Args>
inline std::string tparm(const char* seq, Args... args) { return fmt::format(seq, args...); }
#endif // XCI_WITH_TINFO

// not found in Terminfo DB:
static constexpr auto set_bright_foreground = CSI "1;3{}m";
static constexpr auto set_bright_background = CSI "1;4{}m";
static constexpr auto enter_overline_mode = CSI "53m";
static constexpr auto send_soft_reset = CSI "!p";

class KeySeqLookup {
public:
    explicit KeySeqLookup(std::initializer_list<std::pair<const char*, TermCtl::Key>> seqs) {
        for (const auto& [seq, key] : seqs) {
            add(seq, key);
        }
    }

    std::pair<uint16_t, TermCtl::Key> lookup(std::string_view input_buffer) {
        enum { Start, Esc, Csi, Ss3 } state = Start;
        unsigned len = 0;
        unsigned arg = 0;
        for (const auto c : input_buffer) {
            ++ len;
            switch (state) {
                case Start:
                    if (c >= 8 && c <= 13)
                        return {len, m_lookup_8to13[c - 8]};
                    if (c == '\x1b') {
                        state = Esc;
                        continue;
                    }
                    break;  // unknown
                case Esc:
                    if (c == '[') {
                        state = Csi;
                        continue;
                    }
                    if (c == 'O') {
                        state = Ss3;
                        continue;
                    }
                    break;  // unknown
                case Csi:
                    if (isdigit(c)) {
                        arg = 10 * arg + (c - '0');
                        continue;
                    }
                    if (c == ';') {
                        arg *= 100;
                        continue;
                    }
                    if (c == '~' && arg != 0 && arg <= 24)
                        return {len, m_lookup_csi7e[arg - 1]};
                    if (c >= 'A' && c <= 'Z')
                        return {len, m_lookup_csi_AtoZ[c - 'A']};
                    return {len, TermCtl::Key::Unknown};
                case Ss3:
                    if (c >= 'A' && c <= 'Z')
                        return {len, m_lookup_ss3_AtoZ[c - 'A']};
                    return {len, TermCtl::Key::Unknown};
            }
            break;  // unknown
        }
        return {0, TermCtl::Key::Unknown};
    }

private:
    void add(const char* seq, TermCtl::Key key) {
        const auto c0 = seq[0];
        const auto c1 = seq[1];
        if (c1 == 0) {
            // single byte
            assert(c0 >= 8 && c0 <= 13);
            assert(m_lookup_8to13[c0 - 8] == TermCtl::Key::Unknown);
            m_lookup_8to13[c0 - 8] = key;
            return;
        }
        if (c0 == '\033') {
            // ESC-sequence
            const auto c2 = seq[2];
            if (c1 == '[') {  // CSI
                if (isdigit(c2)) { // 0x7e
                    char* str_end;
                    long arg = strtol(seq + 2, &str_end, 10);
                    assert(arg > 0);
                    assert(str_end != seq + 2);
                    assert(str_end[0] == '~');
                    assert(str_end[1] == 0);
                    assert( m_lookup_csi7e[arg - 1] == TermCtl::Key::Unknown);
                    m_lookup_csi7e[arg - 1] = key;
                    return;
                }
                assert(seq[3] == 0);
                assert(c2 >= 'A' && c2 <= 'Z');
                assert(m_lookup_csi_AtoZ[c2 - 'A'] == TermCtl::Key::Unknown);
                m_lookup_csi_AtoZ[c2 - 'A'] = key;
                return;
            }
            if (c1 == 'O') {  // SS3
                assert(seq[3] == 0);
                assert(c2 >= 'A' && c2 <= 'Z');
                assert(m_lookup_ss3_AtoZ[c2 - 'A'] == TermCtl::Key::Unknown);
                m_lookup_ss3_AtoZ[c2 - 'A'] = key;
                return;
            }
        }
        assert(!"No rule to save seq");
    }

    // 82 bytes in total
    std::array<TermCtl::Key, 6> m_lookup_8to13 {};
    std::array<TermCtl::Key, 26> m_lookup_ss3_AtoZ {};
    std::array<TermCtl::Key, 26> m_lookup_csi_AtoZ {};
    std::array<TermCtl::Key, 24> m_lookup_csi7e {};  // lookup CSI arg minus 1, e.g. "2~" is [1]
};

static KeySeqLookup key_seq_lookup {
        {key_enter, TermCtl::Key::Enter},
        {tab, TermCtl::Key::Tab},
        {key_backspace, TermCtl::Key::Backspace},
        {key_ic, TermCtl::Key::Insert},
        {key_dc, TermCtl::Key::Delete},
        {key_home, TermCtl::Key::Home},
        {key_end, TermCtl::Key::End},
        {key_ppage, TermCtl::Key::PageUp},
        {key_npage, TermCtl::Key::PageDown},
        {key_up, TermCtl::Key::Up},
        {key_down, TermCtl::Key::Down},
        {key_right, TermCtl::Key::Right},
        {key_left, TermCtl::Key::Left},
        {key_f1, TermCtl::Key::F1},
        {key_f2, TermCtl::Key::F2},
        {key_f3, TermCtl::Key::F3},
        {key_f4, TermCtl::Key::F4},
        {key_f5, TermCtl::Key::F5},
        {key_f6, TermCtl::Key::F6},
        {key_f7, TermCtl::Key::F7},
        {key_f8, TermCtl::Key::F8},
        {key_f9, TermCtl::Key::F9},
        {key_f10, TermCtl::Key::F10},
        {key_f11, TermCtl::Key::F11},
        {key_f12, TermCtl::Key::F12},
};


#ifdef _WIN32
// returns orig mode or bad_mode on failure
static constexpr unsigned long bad_mode = ~0ul;
static DWORD set_console_mode(DWORD hid, DWORD req_mode)
{
    HANDLE h = GetStdHandle(hid);
    if (h == INVALID_HANDLE_VALUE)
        return bad_mode;
    DWORD orig_mode = 0;
    if (!GetConsoleMode(h, &orig_mode))
        return bad_mode;
    if (!SetConsoleMode(h, orig_mode | req_mode))
        return bad_mode;
    return orig_mode;
}
static void reset_console_mode(DWORD hid, DWORD mode)
{
    HANDLE h = GetStdHandle(hid);
    if (h != INVALID_HANDLE_VALUE)
        SetConsoleMode(h, mode);
}
#endif


TermCtl& TermCtl::stdin_instance(IsTty is_tty)
{
    static TermCtl term(STDIN_FILENO, is_tty);
    return term;
}


TermCtl& TermCtl::stdout_instance(IsTty is_tty)
{
    static TermCtl term(STDOUT_FILENO, is_tty);
    return term;
}


TermCtl& TermCtl::stderr_instance(IsTty is_tty)
{
    static TermCtl term(STDERR_FILENO, is_tty);
    return term;
}


TermCtl::TermCtl(int fd, IsTty is_tty) : m_fd(fd)
{
    set_is_tty(is_tty);
}


TermCtl::~TermCtl() {
#ifdef _WIN32
    if (m_state == State::InitOk) {
        assert(m_orig_out_mode != bad_mode);
        unsigned long std_handle = (m_fd == STDOUT_FILENO) ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE;
        reset_console_mode(std_handle, m_orig_out_mode);
    }
#else
    (void) 0;
#endif
}


void TermCtl::set_is_tty(IsTty is_tty)
{
#ifdef _WIN32
    unsigned long std_handle;
    DWORD req_mode;
    switch (m_fd) {
        case STDIN_FILENO:
            std_handle = STD_INPUT_HANDLE;
            req_mode = ENABLE_VIRTUAL_TERMINAL_INPUT;
            break;
        case STDOUT_FILENO:
            std_handle = STD_OUTPUT_HANDLE;
            req_mode = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            break;
        case STDERR_FILENO:
            std_handle = STD_ERROR_HANDLE;
            req_mode = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            break;
        default:
            return;
    }

    if (is_tty != IsTty::Never && m_state == State::NoTTY) {
        m_orig_out_mode = set_console_mode(std_handle, req_mode);
        if (m_orig_out_mode == bad_mode)
            return;
        m_state = State::InitOk;
    }
    if (is_tty == IsTty::Never && m_state == State::InitOk) {
        assert(m_orig_out_mode != bad_mode);
        reset_console_mode(std_handle, m_orig_out_mode);
        m_orig_out_mode = 0;
        m_state = State::NoTTY;
    }
#else
    switch (is_tty) {
        case IsTty::Auto:
            // Do not even try if not TTY (e.g. when piping to other command)
            if (isatty(m_fd) != 1) {
                m_state = State::NoTTY;
                return;
            }
            break;  // go to setup below
        case IsTty::Always:
            break;  // go to setup below
        case IsTty::Never:
            m_state = State::NoTTY;
            return;
    }

    #ifdef XCI_WITH_TINFO
    // Setup terminfo
    int err = 0;
    if (setupterm(nullptr, fd, &err) != 0)
        return;
    #endif

    m_state = State::InitOk;
#endif
}


auto TermCtl::size() const -> Size
{
#ifndef _WIN32
    struct winsize ws;
    if (ioctl(m_fd, TIOCGWINSZ, &ws) == -1) {
        return {0, 0};
    }
    return {ws.ws_row, ws.ws_col};
#else
    return {0, 0};
#endif
}


// Note that this cannot be implemented with variadic template,
// because the arguments must not be evaluated unless is_initialized() is true
#define TERM_APPEND(...) TermCtl(*this, is_tty() ? tparm(__VA_ARGS__) : "")

TermCtl TermCtl::fg(Color color) const
{
    if (color < Color::BrightBlack)
        return TERM_APPEND(set_a_foreground, static_cast<int>(color));
    // bright colors
    return TERM_APPEND(set_bright_foreground, static_cast<int>(color) - static_cast<int>(Color::BrightBlack));
}

TermCtl TermCtl::bg(Color color) const
{
    if (color < Color::BrightBlack)
        return TERM_APPEND(set_a_background, static_cast<int>(color));
    // bright colors
    return TERM_APPEND(set_bright_background, static_cast<int>(color) - static_cast<int>(Color::BrightBlack));
}

TermCtl TermCtl::mode(Mode mode) const
{
    switch (mode) {
        case Mode::Normal: return normal();
        case Mode::Bold: return bold();
        case Mode::Dim: return dim();
        case Mode::Underline: return underline();
        case Mode::Overline: return overline();
    }
    UNREACHABLE;
}

TermCtl TermCtl::bold() const { return TERM_APPEND(enter_bold_mode); }
TermCtl TermCtl::dim() const { return TERM_APPEND(enter_dim_mode); }
TermCtl TermCtl::underline() const { return TERM_APPEND(enter_underline_mode); }
TermCtl TermCtl::overline() const { return TERM_APPEND(enter_overline_mode); }
TermCtl TermCtl::normal() const { return TERM_APPEND(exit_attribute_mode); }

TermCtl TermCtl::move_up() const { return TERM_APPEND(cursor_up); }
TermCtl TermCtl::move_up(unsigned n_lines) const { return TERM_APPEND(parm_up_cursor, n_lines); }
TermCtl TermCtl::move_down() const { return TERM_APPEND(cursor_down); }
TermCtl TermCtl::move_down(unsigned n_lines) const { return TERM_APPEND(parm_down_cursor, n_lines); }
TermCtl TermCtl::move_left() const { return TERM_APPEND(cursor_left); }
TermCtl TermCtl::move_left(unsigned n_cols) const { return TERM_APPEND(parm_left_cursor, n_cols); }
TermCtl TermCtl::move_right() const { return TERM_APPEND(cursor_right); }
TermCtl TermCtl::move_right(unsigned n_cols) const { return TERM_APPEND(parm_right_cursor, n_cols); }
TermCtl TermCtl::move_to_column(unsigned column) const { return TERM_APPEND(column_address, column); }

TermCtl TermCtl::clear_screen_down() const { return TERM_APPEND(clr_eos); }
TermCtl TermCtl::clear_line_to_end() const { return TERM_APPEND(clr_eol); }

TermCtl TermCtl::soft_reset() const { return TERM_APPEND(send_soft_reset); }


std::ostream& operator<<(std::ostream& os, const TermCtl& term)
{
    os << term.seq();
    return os;
}


auto TermCtl::ColorPlaceholder::parse(std::string_view name) -> Color
{
    if (name == "black")     return Color::Black;
    if (name == "red")       return Color::Red;
    if (name == "green")     return Color::Green;
    if (name == "yellow")    return Color::Yellow;
    if (name == "blue")      return Color::Blue;
    if (name == "magenta")   return Color::Magenta;
    if (name == "cyan")      return Color::Cyan;
    if (name == "white")     return Color::White;
    throw fmt::format_error("invalid color name: " + std::string(name));
}


auto TermCtl::ModePlaceholder::parse(std::string_view name) -> Mode
{
    if (name == "bold")      return Mode::Bold;
    if (name == "underline") return Mode::Underline;
    if (name == "overline")  return Mode::Overline;
    if (name == "normal")    return Mode::Normal;
    throw fmt::format_error("invalid mode name: " + std::string(name));
}


std::string TermCtl::FgPlaceholder::seq(Color color) const
{
    return term_ctl.fg(color).seq();
}


std::string TermCtl::BgPlaceholder::seq(Color color) const
{
    return term_ctl.bg(color).seq();
}


std::string TermCtl::ModePlaceholder::seq(Mode mode) const
{
    return term_ctl.mode(mode).seq();
}


void TermCtl::with_raw_mode(const std::function<void()>& cb)
{
    assert(m_fd == STDIN_FILENO);
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE)
        return;
    DWORD orig_mode = 0;
    if (!GetConsoleMode(h, &orig_mode))
        return;
    if (!SetConsoleMode(h, ENABLE_VIRTUAL_TERMINAL_INPUT |
            ENABLE_PROCESSED_INPUT ))
        return;

    cb();

    SetConsoleMode(h, orig_mode);
#else
    struct termios origtc = {};
    if (tcgetattr(0, &origtc) < 0) {
        assert(!"tcgetattr failed");
        return;
    }

    struct termios newtc = origtc;
    cfmakeraw(&newtc);
    newtc.c_lflag |= ISIG;
    if (tcsetattr(0, TCSANOW, &newtc) < 0)  {
        assert(!"tcsetattr failed");
        return;
    }

    cb();

    if (tcsetattr(0, TCSANOW, &origtc) < 0) {
        assert(!"tcsetattr failed");
        return;
    }
#endif
}


std::string TermCtl::input()
{
    char buf[20] {};
    while (::read(STDIN_FILENO, buf, sizeof buf) < 0
       && (errno == EINTR || errno == EAGAIN));
    return buf;
}


std::string TermCtl::raw_input()
{
    std::string res;
    with_raw_mode([this, &res] {
        res = input();
    });
    return res;
}


void TermCtl::print(const std::string& buf)
{
    ::write(m_fd, buf.data(), buf.size());
}


unsigned int TermCtl::stripped_length(std::string_view s)
{
    enum State {
        Visible,
        Esc,
        Csi,
    } state = Visible;
    unsigned int length = 0;
    for (const auto c : s) {
        switch (state) {
            case Visible:
                if (c == '\033')
                    state = Esc;
                else
                    ++length;
                break;

            case Esc:
                if (c == '[')
                    state = Csi;
                else
                    state = Visible;
                break;

            case Csi:
                if (isalpha(c))
                    state = Visible;
                break;
        }
    }
    return length;
}


auto TermCtl::decode_input(std::string_view input_buffer) -> DecodedInput
{
    if (input_buffer.empty())
        return {};

    // Lookup escape sequences
    {
        const auto [len, key] = key_seq_lookup.lookup(input_buffer);
        if (len != 0)
            return {len, key};
    }

    // Ascii codes
    bool alt = false;
    unsigned offset = 0;
    switch (input_buffer[0]) {
        case '\n':
        case '\r':
            return {1, Key::Enter};
        case '\b':
        case '\x7f':
            return {1, Key::Backspace};
        case '\x1b':
            if (input_buffer.size() == 1)   // ESC
                return {1, Key::Escape};
            else if (input_buffer[1] == '\x1b')  // ESC ESC
                return {2, Key::Escape, true};
            else { // ESC char
                alt = true;
                offset = 1;
            }
            break;
        default:
            break;
    }

    // UTF-8
    const auto [len, unicode] = utf8_codepoint_and_length(input_buffer.substr(offset));
    if (len > 0)
        return {uint16_t(len + offset), Key::UnicodeChar, alt, unicode};
    if (len == 0)
        return {};  // incomplete UTF-8 char
    assert(len == -1);
    return {uint16_t(1 + offset)};  // consume the first byte of corrupted UTF-8
}


} // namespace xci::core
