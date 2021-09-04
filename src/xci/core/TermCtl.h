// TermCtl.h created on 2018-07-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_TERM_H
#define XCI_CORE_TERM_H

#include <fmt/format.h>
#include <vector>
#include <string>
#include <ostream>
#include <array>
#include <chrono>
#include <functional>

namespace xci::core {

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
    [[nodiscard]] bool is_tty() const { return m_state != State::NoTTY; }

    // Detect terminal size, return {0,0} if not detected
    struct Size {
        unsigned short rows;
        unsigned short cols;
    };
    Size size() const;

    // Following methods are appending the capability codes
    // to a copy of TermCtl instance, which can then be send to stream

    enum class Color {
        Default = 9,
        Black = 0, Red, Green, Yellow, Blue, Magenta, Cyan, White,
        BrightBlack = 10, BrightRed, BrightGreen, BrightYellow,
        BrightBlue, BrightMagenta, BrightCyan, BrightWhite,
    };
    enum class Mode { Normal, Bold, Dim, Underline, Overline };

    // foreground
    TermCtl fg(Color color) const;
    TermCtl black() const { return fg(Color::Black); }
    TermCtl red() const { return fg(Color::Red); }
    TermCtl green() const { return fg(Color::Green); }
    TermCtl yellow() const { return fg(Color::Yellow); }
    TermCtl blue() const { return fg(Color::Blue); }
    TermCtl magenta() const { return fg(Color::Magenta); }
    TermCtl cyan() const { return fg(Color::Cyan); }
    TermCtl white() const { return fg(Color::White); }
    TermCtl default_fg() const { return fg(Color::Default); }

    // background
    TermCtl bg(Color color) const;
    TermCtl on_black() const { return bg(Color::Black); }
    TermCtl on_red() const { return bg(Color::Red); }
    TermCtl on_green() const { return bg(Color::Green); }
    TermCtl on_yellow() const { return bg(Color::Yellow); }
    TermCtl on_blue() const { return bg(Color::Blue); }
    TermCtl on_magenta() const { return bg(Color::Magenta); }
    TermCtl on_cyan() const { return bg(Color::Cyan); }
    TermCtl on_white() const { return bg(Color::White); }
    TermCtl default_bg() const { return bg(Color::Default); }

    // mode
    TermCtl mode(Mode mode) const;
    TermCtl bold() const;  // bold and/or increased intensity
    TermCtl dim() const;  // decreased intensity
    TermCtl underline() const;
    TermCtl overline() const;
    TermCtl normal() const;  // reset all attributes

    // cursor movement
    TermCtl move_up() const;
    TermCtl move_up(unsigned n_lines) const;
    TermCtl move_down() const;
    TermCtl move_down(unsigned n_lines) const;
    TermCtl move_left() const;
    TermCtl move_left(unsigned n_cols) const;
    TermCtl move_right() const;
    TermCtl move_right(unsigned n_cols) const;
    TermCtl move_to_column(unsigned column) const;  // column is 0-based
    TermCtl save_cursor() const { return _save_cursor(); }
    TermCtl restore_cursor() const { return _restore_cursor(); }
    TermCtl request_cursor_position() const;

    /// Returns cursor position (row, col), 0-based
    /// On failure, returns (-1, -1)
    /// \param tin          TermCtl which is connected to the response channel
    std::pair<int, int> get_cursor_position(TermCtl& tin = stdin_instance());

    // clear screen content
    TermCtl clear_screen_down() const;
    TermCtl clear_line_to_end() const;

    TermCtl soft_reset() const;

    // Output cached seq
    const std::string& seq() const { return m_seq; }
    friend std::ostream& operator<<(std::ostream& os, const TermCtl& term);

    // Formatting helpers
    struct Placeholder {
        const TermCtl& term_ctl;
    };
    struct ColorPlaceholder: Placeholder {
        using ValueType = Color;
        static Color parse(std::string_view name);
    };
    struct FgPlaceholder: ColorPlaceholder {
        std::string seq(Color color) const;
    };
    struct BgPlaceholder: ColorPlaceholder {
        std::string seq(Color color) const;
    };
    struct ModePlaceholder: Placeholder {
        using ValueType = Mode;
        static Mode parse(std::string_view name);
        std::string seq(Mode mode) const;
    };

    /// Format string, adding colors via special placeholders:
    /// {fg:COLOR} where COLOR is default | red | *red ... ("*" = bright)
    /// {bg:COLOR} where COLOR is the same as for fg
    /// {t:MODE} where MODE is bold | underline | normal ...
    template<typename ...Args>
    std::string format(const char *fmt, Args&&... args) {
        return fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...,
                        fmt::arg("fg", FgPlaceholder{*this}),
                        fmt::arg("bg", BgPlaceholder{*this}),
                        fmt::arg("t",  ModePlaceholder{*this}));
    }

    /// Print string with special color/mode placeholders, see `format` above.
    template<typename ...Args>
    void print(const char *fmt, Args&&... args) {
        write(format(fmt, std::forward<Args>(args)...));
    }

    void write(std::string_view buf);

    class StreamBuf : public std::streambuf {
    public:
        explicit StreamBuf(TermCtl& t) : m_tout(t) {}

    protected:
        std::streamsize xsputn(const char_type* s, std::streamsize n) override {
            sync();
            m_tout.write(std::string_view{s, (size_t)n});
            return n;
        };

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
    // Copy TermCtl and append seq to new instance
    TermCtl(const TermCtl& term, const std::string& seq)
        : m_seq(term.m_seq + seq)
        , m_state(term.m_state == State::NoTTY ? State::NoTTY : State::CopyOk) {}

    // Aliases needed to avoid macro collision
    TermCtl _save_cursor() const;
    TermCtl _restore_cursor() const;

    WriteCallback m_write_cb;

    std::string m_seq;  // cached capability sequences
    int m_fd;   // FD (on Windows mapped to handle)
    enum class State {
        NoTTY,      // initialization failed
        InitOk,     // main instance (it will reset the term when destroyed)
        CopyOk,     // a copy created by chained method
    };
    State m_state : 7 = State::NoTTY;

#ifdef __EMSCRIPTEN__
    bool m_at_newline : 1 = true;
#endif

#ifdef _WIN32
    unsigned long m_orig_mode = 0;  // original console mode of the handle
#endif
};

} // namespace xci::core


template<typename T>
concept TermCtlPlaceholder =
        std::is_same_v<T, xci::core::TermCtl::FgPlaceholder> ||
        std::is_same_v<T, xci::core::TermCtl::BgPlaceholder> ||
        std::is_same_v<T, xci::core::TermCtl::ModePlaceholder>;

template <TermCtlPlaceholder T>
struct [[maybe_unused]] fmt::formatter<T> {
    typename T::ValueType value;
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();  // NOLINT
        while (it != ctx.end() && *it != '}') {
            ++it;
        }
        value = T::parse({ctx.begin(), size_t(it - ctx.begin())});
        return it;
    }

    template <typename FormatContext>
    auto format(const T& p, FormatContext& ctx) {
        auto msg = p.seq(value);
        return std::copy(msg.begin(), msg.end(), ctx.out());
    }
};


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
    auto format(const xci::core::TermCtl& term, FormatContext& ctx) {
        return std::copy(term.seq().cbegin(), term.seq().cend(), ctx.out());
    }
};


#endif //XCI_CORE_TERM_H
