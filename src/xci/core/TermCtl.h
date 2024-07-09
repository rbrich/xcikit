// TermCtl.h created on 2018-07-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_TERM_H
#define XCI_CORE_TERM_H

#include <xci/compat/macros.h>
#include <fmt/format.h>
#include <vector>
#include <string>
#include <string_view>
#include <ostream>
#include <array>
#include <span>
#include <chrono>
#include <functional>

namespace xci::core {

using namespace fmt::literals;

// Sends control codes and escape sequences to controlling terminal
// or does nothing if the output stream is not connected to TTY.

// FIXME: switch cur_term, free resources (del_curterm)

class TermCtl {
public:
    enum class IsTty {
        Auto,
        Always,
        Never,
    };

    // Static instance for standard input / output / error
    static TermCtl& stdin_instance(IsTty is_tty = IsTty::Auto);
    static TermCtl& stdout_instance(IsTty is_tty = IsTty::Auto);
    static TermCtl& stderr_instance(IsTty is_tty = IsTty::Auto);

    // Constructor for custom streams
    explicit TermCtl(int fd, IsTty is_tty = IsTty::Auto);
    ~TermCtl();

    // Change mode (initial mode is set in constructor)
    void set_is_tty(IsTty is_tty);

    // Is the output stream connected to TTY?
    // This respects chosen Mode:
    // - Auto: true if connected to TTY
    // - Always: true
    // - Never: false
    [[nodiscard]] bool is_tty() const { return m_tty_ok; }

    // Detect terminal size, return {0,0} if not detected
    struct Size {
        uint16_t rows;
        uint16_t cols;
        friend std::ostream& operator<<(std::ostream& os, Size s) { return os << '(' << s.rows << ", " << s.cols << ')'; }
    };
    Size size() const;

    // Following methods are appending the capability codes
    // to a copy of TermCtl instance, which can then be send to stream

    enum class Color: uint8_t {
        Black = 0, Red, Green, Yellow, Blue, Magenta, Cyan, White,
        Invalid = 8, Default = 9,
        BrightBlack = 10, BrightRed, BrightGreen, BrightYellow,
        BrightBlue, BrightMagenta, BrightCyan, BrightWhite,
        _Last = BrightWhite
    };
    static constexpr const char* c_color_names[] = {
        "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
        "invalid", "default",
        "*black", "*red", "*green", "*yellow",
        "*blue", "*magenta", "*cyan", "*white"
    };
    static_assert(std::size(c_color_names) == size_t(Color::_Last) + 1);

    enum class Mode: uint8_t {
        Normal,  // reset all attributes
        Bold, Dim, Italic, Underline, Overline, CrossOut, Frame,
        Blink, Reverse, Hidden,
        NormalIntensity, NoItalic, NoUnderline, NoOverline, NoCrossOut, NoFrame,
        NoBlink, NoReverse, NoHidden,
        _Last = NoHidden
    };
    static constexpr const char* c_mode_names[] = {
        "normal",
        "bold", "dim", "italic", "underline", "overline", "cross_out", "frame",
        "blink", "reverse", "hidden",
        "normal_intensity", "no_italic", "no_underline", "no_overline", "no_cross_out", "no_frame",
        "no_blink", "no_reverse", "no_hidden"
    };
    static_assert(std::size(c_mode_names) == size_t(Mode::_Last) + 1);

    // foreground
    TermCtl& fg(Color color);
    TermCtl& black() { return fg(Color::Black); }
    TermCtl& red() { return fg(Color::Red); }
    TermCtl& green() { return fg(Color::Green); }
    TermCtl& yellow() { return fg(Color::Yellow); }
    TermCtl& blue() { return fg(Color::Blue); }
    TermCtl& magenta() { return fg(Color::Magenta); }
    TermCtl& cyan() { return fg(Color::Cyan); }
    TermCtl& white() { return fg(Color::White); }
    TermCtl& bright_black() { return fg(Color::BrightBlack); }
    TermCtl& bright_red() { return fg(Color::BrightRed); }
    TermCtl& bright_green() { return fg(Color::BrightGreen); }
    TermCtl& bright_yellow() { return fg(Color::BrightYellow); }
    TermCtl& bright_blue() { return fg(Color::BrightBlue); }
    TermCtl& bright_magenta() { return fg(Color::BrightMagenta); }
    TermCtl& bright_cyan() { return fg(Color::BrightCyan); }
    TermCtl& bright_white() { return fg(Color::BrightWhite); }
    TermCtl& default_fg() { return fg(Color::Default); }

    // background
    TermCtl& bg(Color color);
    TermCtl& on_black() { return bg(Color::Black); }
    TermCtl& on_red() { return bg(Color::Red); }
    TermCtl& on_green() { return bg(Color::Green); }
    TermCtl& on_yellow() { return bg(Color::Yellow); }
    TermCtl& on_blue() { return bg(Color::Blue); }
    TermCtl& on_magenta() { return bg(Color::Magenta); }
    TermCtl& on_cyan() { return bg(Color::Cyan); }
    TermCtl& on_white() { return bg(Color::White); }
    TermCtl& default_bg() { return bg(Color::Default); }

    // mode
    TermCtl& mode(Mode mode);
    TermCtl& bold();  // bold or increased intensity
    TermCtl& dim();  // faint or decreased intensity
    TermCtl& italic();
    TermCtl& underline();
    TermCtl& overline();
    TermCtl& cross_out();
    TermCtl& frame();
    TermCtl& blink();
    TermCtl& reverse();
    TermCtl& hidden();  // conceal
    TermCtl& normal_intensity();
    TermCtl& no_italic();
    TermCtl& no_underline();
    TermCtl& no_overline();
    TermCtl& no_cross_out();
    TermCtl& no_frame();
    TermCtl& no_blink();
    TermCtl& no_reverse();
    TermCtl& no_hidden(); // reveal
    TermCtl& normal();  // reset all attributes

    // cursor movement
    TermCtl& move_up();
    TermCtl& move_up(unsigned n_lines);
    TermCtl& move_down();
    TermCtl& move_down(unsigned n_lines);
    TermCtl& move_left();
    TermCtl& move_left(unsigned n_cols);
    TermCtl& move_right();
    TermCtl& move_right(unsigned n_cols);
    TermCtl& move_to_column(unsigned column);  // column is 0-based
    TermCtl& move_to_beginning();  // CR ('\r')
    TermCtl& save_cursor() { return _save_cursor(); }
    TermCtl& restore_cursor() { return _restore_cursor(); }
    TermCtl& request_cursor_position();

    // tabulation (tab stops)
    TermCtl& tab_clear();      // TBC 0 (CSI 0 g)
    TermCtl& tab_clear_all();  // TBC 3 (CSI 3 g)
    TermCtl& tab_set();        // HTS   (ESC H or \x88)
    TermCtl& tab_set_every(unsigned n_cols);    // composite operation
    TermCtl& tab_set_all(std::span<unsigned> n_cols) {
        return _tab_set_all(std::span<const unsigned>{n_cols.data(), n_cols.size()});
    }
    TermCtl& tab_set_all(std::initializer_list<unsigned> n_cols) {
        return _tab_set_all(std::span{n_cols.begin(), n_cols.end()});
    }
    TermCtl& tab_set_all(std::vector<unsigned> n_cols) {
        return _tab_set_all(std::span{n_cols.data(), n_cols.size()});
    }

    /// Returns cursor position (row, col), 0-based
    /// On failure, returns (-1, -1)
    /// \param tin          TermCtl which is connected to the response channel
    std::pair<int, int> get_cursor_position(TermCtl& tin = stdin_instance());

    // clear screen content
    TermCtl& clear_screen_down();
    TermCtl& clear_line_to_end();

    TermCtl& soft_reset();

    // Cached seq
    std::string seq() { return std::move(m_seq); }
    void write() { write_raw(seq()); }
    void write_nl() { m_seq.append(1, '\n'); write(seq()); }
    friend std::ostream& operator<<(std::ostream& os, TermCtl& t) { return os << t.seq(); }

    /// Format string, adding colors via special placeholders:
    /// <fg:COLOR> where COLOR is default | red | *red ... ("*" = bright)
    /// <bg:COLOR> where COLOR is the same as for fg
    /// <t:MODE> where MODE is bold | underline | normal ...
    template<typename... T>
    std::string format(fmt::format_string<T...> fmt, T&&... args) {
        return fmt::vformat(_format({fmt.get().begin(), fmt.get().end()}),
                            fmt::make_format_args(args...));
    }

    /// Print string with special color/mode placeholders, see `format` above.
    template<typename... T>
    void print(fmt::format_string<T...> fmt, T&&... args) {
        write(format(fmt::runtime(fmt), args...));
    }

    void write(std::string_view buf);
    void write_raw(std::string_view buf);  // doesn't check newline

    class StreamBuf : public std::streambuf {
    public:
        explicit StreamBuf(TermCtl& t) : m_tout(t) {}

    protected:
        std::streamsize xsputn(const char_type* s, std::streamsize n) override {
            sync();
            m_tout.write(std::string_view{s, (size_t)n});
            return n;
        }

        int_type overflow(int_type ch) override {
            if (traits_type::eq_int_type(ch, traits_type::eof()))
                return 0;
            if (m_buf_pos == m_buf.size())
                sync();
            m_buf[m_buf_pos] = traits_type::to_char_type(ch);
            ++m_buf_pos;
            return 1;
        }

        int sync() override {
            if (m_buf_pos != 0) {
                m_tout.write(std::string_view{m_buf.data(), m_buf_pos});
                m_buf_pos = 0;
            }
            return 0;
        }

    private:
        TermCtl& m_tout;
        std::array<char, 500> m_buf;
        unsigned m_buf_pos = 0;
    };

    class Stream : public std::ostream {
    public:
        Stream(TermCtl& t) : std::ostream(&m_sbuf), m_sbuf(t) {}
    private:
        StreamBuf m_sbuf;
    };

    Stream stream() { return Stream{*this}; }

    using WriteCallback = std::function<void(std::string_view data)>;
    void set_write_callback(WriteCallback cb) { m_write_cb = std::move(cb); }

    /// This function makes sure the cursor is at line beginning (col 0).
    /// When the last output didn't end with a newline,
    /// it prints a "missing newline" marker (⏎) and adds the newline.
    /// \param tin          TermCtl which is connected to the response channel
    void sanitize_newline(TermCtl& tin = stdin_instance());

    /// Compute number of columns required to print the string `s`.
    /// Control sequences and invisible characters are stripped,
    /// double-width UTF-8 characters are counted as two columns.
    /// Newlines (\n) are counted as 1 column to allow multiline handling in EditLine.
    static unsigned int stripped_width(std::string_view s);

    /// Temporarily switch the terminal to raw mode
    /// (no echo, no buffering, no special processing, no signal processing)
    /// \param isig     ISIG flag. Set to true to enable signal processing (Ctrl-C etc.)
    void with_raw_mode(const std::function<void()>& cb, bool isig = false);

    /// Read input from stdin
    /// \param timeout  Return after timeout if no input comes (default = infinite)
    /// \returns        The input data, empty string on error, timeout or EOF
    std::string input(std::chrono::microseconds timeout = {});

    /// Combination of `with_raw_mode` and `input`
    std::string raw_input(bool isig = false);

    /// Query terminal
    /// Send request, read response from `tin`.
    /// \param request      a control sequence to send as a request
    /// \param tin          TermCtl which is connected to the response channel
    /// \returns            raw response from the terminal
    std::string query(std::string_view request, TermCtl& tin = stdin_instance());

    enum class Key : uint8_t {
        Unknown = 0,

        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        Escape,
        Enter,
        Backspace,
        Tab,
        Insert, Delete,
        Home, End,
        PageUp, PageDown,
        Left, Right, Up, Down,

        UnicodeChar,
    };

    struct Modifier {
        uint8_t flags = 0;
        enum {
            None = 0,
            Shift = 1,
            Alt = 2,
            Ctrl = 4,
            Meta = 8,
        };
        explicit operator bool() const { return flags != 0; }
        friend std::ostream& operator<<(std::ostream& os, Modifier v);

        void set_shift() { flags |= Shift; }
        void set_alt() { flags |= Alt; }
        void set_ctrl() { flags |= Ctrl; }
        void set_meta() { flags |= Meta; }

        // ignore Shift, translate Meta to Alt, leaving only three combinations: Ctrl, Alt, Ctrl|Alt
        Modifier normalized() const;
        uint8_t normalized_flags() const { return normalized().flags; }

        bool is_shift() const { return flags == Shift; }
        bool is_alt() const { return flags == Alt; }
        bool is_ctrl() const { return flags == Ctrl; }
        bool is_meta() const { return flags == Meta; }
        bool is_ctrl_alt() const { return flags == (Ctrl | Alt); }
    };

    struct DecodedInput {
        uint16_t input_len = 0;  // length of input sequence (chars consumed)
        Key key = Key::Unknown;
        Modifier mod;
        char32_t unicode = 0;
    };

    /// Try to decode input key or char from byte sequence
    /// \param input_buffer     Raw bytes as read from TTY (see raw_input above)
    /// \returns DecodedInput:
    /// * input_len == 0    -- incomplete input, read more chars into the buffer
    /// * input_len > 0     -- this number of bytes was used
    /// * key               -- Unknown if input_len == 0 or corrupted UTF-8,
    ///                        otherwise either a special key or UnicodeChar (see `unicode` for actual char)
    /// * unicode           -- Unicode character decoded from the input (only when key=UnicodeChar)
    ///                        or zero in case of special cases (nothing decoded or a non-unicode key)
    DecodedInput decode_input(std::string_view input_buffer);

    struct ControlSequence {
        std::vector<int> par;  // parameters, default value is -1 (empty parameter)
        char fun = 0;  // function
        uint16_t input_len = 0;  // length of input sequence (chars consumed)
    };
    ControlSequence decode_seq(std::string_view input_buffer);

private:
    // Aliases needed to avoid macro collision
    TermCtl& _save_cursor();
    TermCtl& _restore_cursor();
    TermCtl& _tab_set_all(std::span<const unsigned> n_cols);
    TermCtl& _append_seq(const char* seq) { if (seq) m_seq += seq; return *this; }  // needed for TermInfo, which returns NULL for unknown seqs
    TermCtl& _append_seq(std::string_view seq) { m_seq += seq; return *this; }
    std::string _format(std::string_view fmt);

    static constexpr Mode _parse_mode(std::string_view name) {
        Mode r = Mode::Normal;
        for (const char* n : c_mode_names) {
            if (name == n)
                return r;
            r = static_cast<Mode>(uint8_t(r) + 1);
        }
        return r;
    }

    static constexpr Color _parse_color(std::string_view name) {
        Color r = Color::Black;
        for (const char* n : c_color_names) {
            if (name == n)
                return r;
            r = static_cast<Color>(uint8_t(r) + 1);
        }
        return r;
    }

    std::string m_seq;  // cached capability sequences
    WriteCallback m_write_cb;
    int m_fd;   // FD (on Windows mapped to handle)
    bool m_tty_ok : 1 = false;  // tty initialized, will reset the term when destroyed
    bool m_at_newline : 1 = true;

#ifdef _WIN32
    unsigned long m_orig_mode = 0;  // original console mode of the handle
#endif
};

} // namespace xci::core


// support `term.bold()` etc. directly in format args
template <>
struct [[maybe_unused]] fmt::formatter<xci::core::TermCtl> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();  // NOLINT
        if (it != ctx.end() && *it != '}')
            throw fmt::format_error("invalid format for TermCtl");
        return it;
    }

    template <typename FormatContext>
    auto format(xci::core::TermCtl& term, FormatContext& ctx) const {
        auto seq = term.seq();
        return std::copy(seq.cbegin(), seq.cend(), ctx.out());
    }
};


#endif //XCI_CORE_TERM_H
