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

    // Static instance for standard output
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

    // Following methods are appending the capability codes
    // to a copy of TermCtl instance, which can then be send to stream

    enum class Color {
        Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
        BrightBlack, BrightRed, BrightGreen, BrightYellow,
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

    // clear screen content
    TermCtl clear_screen_down() const;

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

    // Temporarily change terminal mode to RAW mode
    // (no echo, no buffering, no special processing)
    // NOTE: Signal processing is enabled, so Ctrl-C still works
    void with_raw_mode(const std::function<void()>& cb);
    std::string raw_input();

private:
    // Copy TermCtl and append seq to new instance
    TermCtl(const TermCtl& term, const std::string& seq)
        : m_state(term.m_state == State::NoTTY ? State::NoTTY : State::CopyOk)
        , m_seq(term.m_seq + seq) {}

    void print(const std::string& buf);

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
