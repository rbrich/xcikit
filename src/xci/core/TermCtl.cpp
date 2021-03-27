// TermCtl.cpp created on 2018-07-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// References:
// * terminal control sequences:
//   - https://en.wikipedia.org/wiki/ANSI_escape_code
//   - https://www.ecma-international.org/wp-content/uploads/ECMA-48_5th_edition_june_1991.pdf
//   - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
//   - https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
//   - terminfo(5), https://linux.die.net/man/5/terminfo
// * raw mode:
//   - https://en.wikipedia.org/wiki/POSIX_terminal_interface
//   - termios(4)

#include "TermCtl.h"
#include "log.h"
#include <xci/compat/unistd.h>
#include <xci/compat/macros.h>
#include <xci/core/string.h>
#include <xci/core/file.h>
#include <xci/config.h>

#ifdef _WIN32
    #include <windows.h>
    static_assert(sizeof(unsigned long) == sizeof(DWORD));
#else
    #include <termios.h>
    #include <sys/ioctl.h>
#endif

#ifdef XCI_WITH_TERMINFO
    #include <term.h>
#endif

#include <fmt/format.h>

#include <array>
#include <cassert>

namespace xci::core {

#define ESC     "\033"
#define CSI     ESC "["
#define SS3     ESC "O"

// When building without TInfo, emit ANSI escape sequences directly
// Note that these need to be macros, to match compiler behaviour with <term.h>
#ifndef XCI_WITH_TERMINFO
#define cursor_up               CSI "A"
#define cursor_down             CSI "B"
#define cursor_right            CSI "C"
#define cursor_left             CSI "D"
#define enter_bold_mode         CSI "1m"
#define enter_dim_mode          CSI "2m"
#define enter_underline_mode    CSI "4m"
#define exit_attribute_mode     CSI "0m"
#define set_a_foreground        CSI "3{}m"
#define set_a_background        CSI "4{}m"
#define parm_up_cursor          CSI "{}A"   // move cursor up N lines
#define parm_down_cursor        CSI "{}B"   // move cursor down N lines
#define parm_right_cursor       CSI "{}C"   // move cursor right N spaces
#define parm_left_cursor        CSI "{}D"   // move cursor left N spaces
#define clr_eos                 CSI "J"     // clear screen from cursor down
#define clr_eol                 CSI "K"     // clear line from cursor to end
#define column_address          CSI "{}G"   // horizontal position, absolute
#define save_cursor             ESC "7"
#define restore_cursor          ESC "8"
#endif // XCI_WITH_TERMINFO

// not found in Terminfo DB:
static constexpr auto set_default_foreground = CSI "39m";
static constexpr auto set_default_background = CSI "49m";
static constexpr auto set_bright_foreground = CSI "9{}m";
static constexpr auto set_bright_background = CSI "10{}m";
static constexpr auto enter_overline_mode = CSI "53m";
static constexpr auto send_soft_reset = CSI "!p";


inline constexpr const char* xci_tparm(const char* seq) { return seq; }
template<typename ...Args>
inline std::string xci_tparm(const char* seq, Args... args) { return fmt::format(seq, args...); }

// Note that this cannot be implemented with variadic template,
// because the arguments must not be evaluated unless is_initialized() is true
#define XCI_TERM_APPEND(...) TermCtl(*this, is_tty() ? xci_tparm(__VA_ARGS__) : "")

#ifdef XCI_WITH_TERMINFO
    // delegate to TermInfo
    #define TERM_APPEND(...) TermCtl(*this, is_tty() ? tparm(__VA_ARGS__) : "")
    static unsigned _plus_one(unsigned arg) { return arg; };  // already corrected with Terminfo -> noop
#else
    // delegate to our implementation
    #define TERM_APPEND(...) XCI_TERM_APPEND(__VA_ARGS__)
    static unsigned _plus_one(unsigned arg) { return arg + 1; };
#endif


class TermInputSeq {
public:

    static TermCtl::DecodedInput lookup(std::string_view input_buffer) {
        return instance()._lookup(input_buffer);
    }

#ifdef XCI_WITH_TERMINFO
    static void populate_terminfo() {
        std::initializer_list<std::pair<const char*, TermCtl::Key>> seqs = {
                {key_enter, TermCtl::Key::Enter},
                {tab, TermCtl::Key::Tab},
                {key_backspace, TermCtl::Key::Backspace},
                {key_ic, TermCtl::Key::Insert},  // insert character
                {key_dc, TermCtl::Key::Delete},  // delete character
                {key_home, TermCtl::Key::Home},
                {key_end, TermCtl::Key::End},
                {key_ppage, TermCtl::Key::PageUp},  // prev page
                {key_npage, TermCtl::Key::PageDown},  // next page
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
        for (const auto& [seq, key] : seqs) {
            instance().add(seq, key);
        }
    }
#endif

private:
    static TermInputSeq& instance() {
        static TermInputSeq instance {
            {"\n", TermCtl::Key::Enter},
            {"\r", TermCtl::Key::Enter},
            {"\t", TermCtl::Key::Tab},
            {"\b", TermCtl::Key::Backspace},
            {"\x7f", TermCtl::Key::Backspace},
            {CSI "2~", TermCtl::Key::Insert},
            {CSI "3~", TermCtl::Key::Delete},
            {CSI "H", TermCtl::Key::Home},
            {CSI "F", TermCtl::Key::End},
            {CSI "5~", TermCtl::Key::PageUp},
            {CSI "6~", TermCtl::Key::PageDown},
            {CSI "A", TermCtl::Key::Up},
            {CSI "B", TermCtl::Key::Down},
            {CSI "C", TermCtl::Key::Right},
            {CSI "D", TermCtl::Key::Left},
            {SS3 "P", TermCtl::Key::F1},
            {SS3 "Q", TermCtl::Key::F2},
            {SS3 "R", TermCtl::Key::F3},
            {SS3 "S", TermCtl::Key::F4},
            {CSI "P", TermCtl::Key::F1},  // Ctrl+ sends CSI instead of SS3
            {CSI "Q", TermCtl::Key::F2},
            {CSI "R", TermCtl::Key::F3},
            {CSI "S", TermCtl::Key::F4},
            {CSI "15~", TermCtl::Key::F5},
            {CSI "17~", TermCtl::Key::F6},
            {CSI "18~", TermCtl::Key::F7},
            {CSI "19~", TermCtl::Key::F8},
            {CSI "20~", TermCtl::Key::F9},
            {CSI "21~", TermCtl::Key::F10},
            {CSI "23~", TermCtl::Key::F11},
            {CSI "24~", TermCtl::Key::F12},
        };
        return instance;
    };

    explicit TermInputSeq(std::initializer_list<std::pair<const char*, TermCtl::Key>> seqs) {
        for (const auto& [seq, key] : seqs) {
            add(seq, key);
        }
    }

    void add(const char* seq, TermCtl::Key key) {
        const auto c0 = seq[0];
        const auto c1 = seq[1];
        if (c1 == 0) {
            // single byte
            if (c0 == '\x7f') {
                m_map_7f = key;
                return;
            }
            assert(c0 >= 8 && c0 <= 13);
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
                    m_lookup_csi7e[arg - 1] = key;
                    return;
                }
                assert(seq[3] == 0);
                assert(c2 >= 'A' && c2 <= 'Z');
                m_lookup_csi_AtoZ[c2 - 'A'] = key;
                return;
            }
            if (c1 == 'O') {  // SS3
                assert(seq[3] == 0);
                assert(c2 >= 'A' && c2 <= 'Z');
                m_lookup_ss3_AtoZ[c2 - 'A'] = key;
                return;
            }
        }
        assert(!"No rule to save seq");
    }

    TermCtl::DecodedInput _lookup(std::string_view input_buffer) {
        enum { Start, Esc, Csi, Ss3 } state = Start;
        uint16_t len = 0;
        unsigned arg[10] {};
        unsigned arg_i = 0;
        for (const auto c : input_buffer) {
            ++ len;
            switch (state) {
                case Start:
                    if (c == '\x1b') {
                        state = Esc;
                        continue;
                    }
                    if (c == '\x7f')
                        return {len, m_map_7f};
                    if (c >= 8 && c <= 13) {
                        const auto key = m_lookup_8to13[c - 8];
                        if (key != TermCtl::Key::Unknown)
                            return {len, key};
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
                        arg[arg_i] = 10 * arg[arg_i] + (c - '0');
                        continue;
                    }
                    if (c == ';') {
                        ++arg_i;
                        continue;
                    }
                    if (c == '~' && arg[0] != 0 && arg[0] <= 24)
                        return {len, m_lookup_csi7e[arg[0] - 1], decode_mod(arg[1])};
                    if (c >= 'A' && c <= 'Z')
                        return {len, m_lookup_csi_AtoZ[c - 'A'], decode_mod(arg[1])};
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

    static auto decode_mod(unsigned arg) -> TermCtl::Modifier {
        if (arg == 0)
            return {};
        return TermCtl::Modifier{uint8_t(arg - 1)};
    }

    // 82 bytes in total
    TermCtl::Key m_map_7f {};
    std::array<TermCtl::Key, 6> m_lookup_8to13 {};
    std::array<TermCtl::Key, 26> m_lookup_ss3_AtoZ {};
    std::array<TermCtl::Key, 26> m_lookup_csi_AtoZ {};
    std::array<TermCtl::Key, 24> m_lookup_csi7e {};  // lookup CSI arg minus 1, e.g. "2~" is [1]
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
        assert(m_orig_mode != bad_mode);
        switch (m_fd) {
            case STDIN_FILENO:
                reset_console_mode(STD_INPUT_HANDLE, m_orig_mode);
                break;
            case STDOUT_FILENO:
                reset_console_mode(STD_OUTPUT_HANDLE, m_orig_mode);
                break;
            case STDERR_FILENO:
                reset_console_mode(STD_ERROR_HANDLE, m_orig_mode);
                break;
            default:
                return;
        }
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
        m_orig_mode = set_console_mode(std_handle, req_mode);
        if (m_orig_mode == bad_mode)
            return;
        m_state = State::InitOk;
    }
    if (is_tty == IsTty::Never && m_state == State::InitOk) {
        assert(m_orig_mode != bad_mode);
        reset_console_mode(std_handle, m_orig_mode);
        m_orig_mode = 0;
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

    #ifdef XCI_WITH_TERMINFO
    // Setup terminfo
    int err = 0;
    if (setupterm(nullptr, m_fd, &err) != 0)
        return;
    if (m_fd == STDIN_FILENO)
        TermInputSeq::populate_terminfo();
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


TermCtl TermCtl::fg(Color color) const
{
    if (color == Color::Default)
        return XCI_TERM_APPEND(set_default_foreground);
    else if (color < Color::BrightBlack)
        return TERM_APPEND(set_a_foreground, static_cast<int>(color));
    // bright colors
    return XCI_TERM_APPEND(set_bright_foreground, static_cast<int>(color) - static_cast<int>(Color::BrightBlack));
}

TermCtl TermCtl::bg(Color color) const
{
    if (color == Color::Default)
        return XCI_TERM_APPEND(set_default_background);
    else if (color < Color::BrightBlack)
        return TERM_APPEND(set_a_background, static_cast<int>(color));
    // bright colors
    return XCI_TERM_APPEND(set_bright_background, static_cast<int>(color) - static_cast<int>(Color::BrightBlack));
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
TermCtl TermCtl::overline() const { return XCI_TERM_APPEND(enter_overline_mode); }
TermCtl TermCtl::normal() const { return TERM_APPEND(exit_attribute_mode); }

TermCtl TermCtl::move_up() const { return TERM_APPEND(cursor_up); }
TermCtl TermCtl::move_up(unsigned n_lines) const { return TERM_APPEND(parm_up_cursor, n_lines); }
TermCtl TermCtl::move_down() const { return TERM_APPEND(cursor_down); }
TermCtl TermCtl::move_down(unsigned n_lines) const { return TERM_APPEND(parm_down_cursor, n_lines); }
TermCtl TermCtl::move_left() const { return TERM_APPEND(cursor_left); }
TermCtl TermCtl::move_left(unsigned n_cols) const { return TERM_APPEND(parm_left_cursor, n_cols); }
TermCtl TermCtl::move_right() const { return TERM_APPEND(cursor_right); }
TermCtl TermCtl::move_right(unsigned n_cols) const { return TERM_APPEND(parm_right_cursor, n_cols); }
TermCtl TermCtl::move_to_column(unsigned column) const { return TERM_APPEND(column_address, _plus_one(column)); }
TermCtl TermCtl::_save_cursor() const { return TERM_APPEND(save_cursor); }
TermCtl TermCtl::_restore_cursor() const { return TERM_APPEND(restore_cursor); }

TermCtl TermCtl::clear_screen_down() const { return TERM_APPEND(clr_eos); }
TermCtl TermCtl::clear_line_to_end() const { return TERM_APPEND(clr_eol); }

TermCtl TermCtl::soft_reset() const { return XCI_TERM_APPEND(send_soft_reset); }


std::ostream& operator<<(std::ostream& os, const TermCtl& term)
{
    os << term.seq();
    return os;
}


auto TermCtl::ColorPlaceholder::parse(std::string_view name) -> Color
{
    if (name == "default")   return Color::Default;
    if (name == "black")     return Color::Black;
    if (name == "red")       return Color::Red;
    if (name == "green")     return Color::Green;
    if (name == "yellow")    return Color::Yellow;
    if (name == "blue")      return Color::Blue;
    if (name == "magenta")   return Color::Magenta;
    if (name == "cyan")      return Color::Cyan;
    if (name == "white")     return Color::White;
    if (name == "*black")    return Color::BrightBlack;
    if (name == "*red")      return Color::BrightRed;
    if (name == "*green")    return Color::BrightGreen;
    if (name == "*yellow")   return Color::BrightYellow;
    if (name == "*blue")     return Color::BrightBlue;
    if (name == "*magenta")  return Color::BrightMagenta;
    if (name == "*cyan")     return Color::BrightCyan;
    if (name == "*white")    return Color::BrightWhite;
    throw fmt::format_error("invalid color name: " + std::string(name));
}


auto TermCtl::ModePlaceholder::parse(std::string_view name) -> Mode
{
    if (name == "bold")      return Mode::Bold;
    if (name == "dim")       return Mode::Dim;
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


void TermCtl::with_raw_mode(const std::function<void()>& cb, bool isig)
{
    if (m_fd != STDIN_FILENO)
        return;

#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE)
        return;
    DWORD orig_mode = 0;
    if (!GetConsoleMode(h, &orig_mode))
        return;
    if (!SetConsoleMode(h, ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_PROCESSED_INPUT))
        return;

    cb();

    SetConsoleMode(h, orig_mode);
#else
    struct termios origtc = {};
    if (tcgetattr(m_fd, &origtc) < 0) {
        assert(!"tcgetattr failed");
        return;
    }

    struct termios newtc = origtc;
    cfmakeraw(&newtc);
    if (isig)
        newtc.c_lflag |= ISIG;
    if (tcsetattr(m_fd, TCSANOW, &newtc) < 0)  {
        assert(!"tcsetattr failed");
        return;
    }

    cb();

    if (tcsetattr(m_fd, TCSANOW, &origtc) < 0) {
        assert(!"tcsetattr failed");
        return;
    }
#endif
}


std::string TermCtl::input()
{
    char buf[100] {};
    ssize_t res;
    do {
        res = ::read(m_fd, buf, sizeof buf);
    } while (res < 0 && (errno == EINTR || errno == EAGAIN));
    if (res < 0) {
        log::error("read: {m}");
        return {};
    }
    if (res == 0) {
        return {};  // eof
    }
    return {buf, size_t(res)};
}


std::string TermCtl::raw_input(bool isig)
{
    std::string res;
    with_raw_mode([this, &res] {
        res = input();
    }, isig);
    return res;
}


void TermCtl::write(std::string_view buf)
{
    if (m_write_cb)
        m_write_cb(buf);
    else
        core::write(m_fd, buf);
}


unsigned int TermCtl::stripped_width(std::string_view s)
{
    enum State {
        Visible,
        Esc,
        Csi,
        ConsumeOne,
    } state = Visible;
    unsigned int length = 0;
    for (auto it = s.cbegin(); it != s.cend(); it = utf8_next(it)) {
        auto c = utf8_codepoint(&*it);
        switch (state) {
            case Visible:
                if (c == '\033')
                    state = Esc;
                else
                    length += c32_width(c);
                break;

            case Esc:
                if (c == '[')
                    state = Csi;
                else if (strchr(" #%()*+-./", c) != nullptr)
                    // two-char ESC controls, e.g. ESC # 3
                    state = ConsumeOne;
                else
                    state = Visible;
                break;

            case Csi:
                if (isalpha(c))
                    state = Visible;
                break;

            case ConsumeOne:
                // consume one character and switch back to normal state
                state = Visible;
                break;
        }
    }
    return length;
}


std::ostream& operator<<(std::ostream& s, TermCtl::Modifier v)
{
    using Mod = TermCtl::Modifier;
    if (v.flags == 0) {
        s << "None";
        return s;
    }
    if (v.flags & Mod::Shift)  s << "Shift";
    if (v.flags & Mod::Ctrl)   s << "Ctrl";
    if (v.flags & Mod::Alt)    s << "Alt";
    if (v.flags & Mod::Meta)   s << "Meta";
    return s;
}


auto TermCtl::Modifier::normalized() const -> Modifier
{
    Modifier res;
    if (flags & Ctrl)
        res.set_ctrl();
    if (flags & (Alt | Meta))
        res.set_alt();
    return res;
}


auto TermCtl::decode_input(std::string_view input_buffer) -> DecodedInput
{
    if (input_buffer.empty())
        return {};

    // Lookup escape sequences
    {
        const auto decoded = TermInputSeq::lookup(input_buffer);
        if (decoded.input_len != 0)
            return decoded;
    }

    // Special handling of ESC
    Modifier mod;
    unsigned offset = 0;
    if (input_buffer[0] == '\x1b') {
        if (input_buffer.size() == 1)   // ESC
            return {1, Key::Escape};
        // ESC + <seq>
        auto decoded = TermInputSeq::lookup(input_buffer.substr(1));
        if (decoded.input_len != 0) {
            decoded.input_len += 1;
            decoded.mod.set_alt();
            return decoded;
        }
        // ESC + ESC
        if (input_buffer[1] == '\x1b') {
            mod.set_alt();
            return {2, Key::Escape, mod};
        }
        // ESC + <char>
        // remember Alt, continue to UTF-8 with offset
        mod.set_alt();
        offset = 1;
    }
    if (input_buffer[offset] >= 0 && input_buffer[offset] < 32) {
        // Ctrl + <char>
        mod.set_ctrl();
        return {uint16_t(offset + 1), Key::UnicodeChar, mod,
                char32_t(tolower('@' + input_buffer[offset]))};
    }

    // UTF-8
    const auto [len, unicode] = utf8_codepoint_and_length(input_buffer.substr(offset));
    if (len > 0)
        return {uint16_t(len + offset), Key::UnicodeChar, mod, unicode};
    if (len == 0)
        return {};  // incomplete UTF-8 char
    assert(len == -1);
    return {uint16_t(1 + offset)};  // consume the first byte of corrupted UTF-8
}


} // namespace xci::core
