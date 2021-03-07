// TermCtl.h created on 2018-07-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_TERM_H
#define XCI_CORE_TERM_H

#include <fmt/format.h>
#include <string>
#include <ostream>

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

    template<typename ...Args>
    std::string format(const char *fmt, Args&&... args) {
        return fmt::format(fmt, std::forward<Args>(args)...,
                        fmt::arg("fg", FgPlaceholder{*this}),
                        fmt::arg("bg", BgPlaceholder{*this}),
                        fmt::arg("t",  ModePlaceholder{*this}));
    }

    template<typename ...Args>
    void print(const char *fmt, Args&&... args) {
        auto buf = format(fmt, std::forward<Args>(args)...);
        print(buf);
    }

    /// Compute length of `s` when stripped of terminal control sequences and invisible characters
    static unsigned int stripped_length(std::string_view s);

    /// Temporarily change terminal mode to RAW mode
    /// (no echo, no buffering, no special processing, only signal processing allowed by default)
    /// \param isig     ISIG flag. Set to false to also disable signal processing (Ctrl-C etc.)
    void with_raw_mode(const std::function<void()>& cb, bool isig = true);

    /// Read input from stdin
    /// \returns    The input data, empty string on error or EOF
    std::string input();

    /// Combination of `with_raw_mode` and `input`
    std::string raw_input(bool isig = true);

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

    enum class Modifier : uint8_t {
        None,
        Alt,
        Ctrl,  // "Ctrl" is a common name for the Control key
        CtrlAlt,  // Ctrl + Alt
    };

    struct DecodedInput {
        uint16_t input_len = 0;  // length of input sequence (chars consumed)
        Key key = Key::Unknown;
        Modifier mod = Modifier::None;
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

private:
    // Copy TermCtl and append seq to new instance
    TermCtl(const TermCtl& term, const std::string& seq)
        : m_state(term.m_state == State::NoTTY ? State::NoTTY : State::CopyOk)
        , m_seq(term.m_seq + seq) {}

    void print(const std::string& buf);

    // Aliases needed to avoid macro collision
    TermCtl _save_cursor() const;
    TermCtl _restore_cursor() const;

    enum class State {
        NoTTY,      // initialization failed
        InitOk,     // main instance (it will reset the term when destroyed)
        CopyOk,     // a copy created by chained method
    };
    State m_state = State::NoTTY;
    std::string m_seq;  // cached capability sequences
    int m_fd;

#ifdef _WIN32
    unsigned long m_orig_out_mode = 0;
#endif
};

} // namespace xci::core


template<typename T>
concept TermCtlPlaceholder =
        std::is_same_v<T, xci::core::TermCtl::FgPlaceholder> ||
        std::is_same_v<T, xci::core::TermCtl::BgPlaceholder> ||
        std::is_same_v<T, xci::core::TermCtl::ModePlaceholder>;

template <TermCtlPlaceholder T, typename Char, typename Enable>
struct [[maybe_unused]] fmt::formatter<T, Char, Enable> {
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
