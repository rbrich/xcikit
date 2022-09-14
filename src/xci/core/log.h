// log.h created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_LOG_H
#define XCI_CORE_LOG_H

#include <xci/config.h>

#include <fmt/format.h>

#include <string_view>
#include <filesystem>

namespace xci::core {


class Logger
{
public:
    enum class Level {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        None,  // disable logging
    };

    // Initialize default logger, Call this before anything that logs
    // to make sure logger is created before it (and destroyed after).
    // If not called, default logger will be created lazily
    // (at the time of first use).
    static void init(Level level = Level::Trace) { (void) default_instance(level); }

    static Logger& default_instance(Logger::Level initial_level = Level::Trace);

    explicit Logger(Level level);

    // Set minimal level of messages to be logged.
    // Messages below this level are dropped.
    void set_level(Level level) { m_level = level; }

    // Customizable log handler
    // The function parameters are preformatted message and the log level.
    // The handler has to add a timestamp by itself.
    static void default_handler(Level lvl, std::string_view msg);
    using Handler = decltype(&default_handler);
    void set_handler(Handler handler) { m_handler = handler; }

    void log(Level lvl, std::string_view msg);

private:
    Level m_level;
    Handler m_handler = default_handler;
};


namespace log {

// Dummy type for custom formatting of last error message:
// - {m} or {m:s}   strerror(errno)
// - {m:d}          errno
// - {m:l}          GetLastError on Windows, same as {m} elsewhere
struct LastErrorPlaceholder {
    static std::string message(bool use_last_error, bool error_code);
};

#define XCI_LAST_ERROR_ARG fmt::arg("m", LastErrorPlaceholder{})

template <typename... T>
inline std::string format(fmt::format_string<T..., decltype(XCI_LAST_ERROR_ARG)> fmt, T&&... args) {
    return fmt::vformat(fmt, fmt::make_format_args(args..., XCI_LAST_ERROR_ARG));
}

template <typename... T>
inline void message(Logger::Level lvl, fmt::format_string<T..., decltype(XCI_LAST_ERROR_ARG)> fmt, T&&... args) {
    const auto& vargs = fmt::make_format_args(args..., XCI_LAST_ERROR_ARG);
    Logger::default_instance().log(lvl, fmt::vformat(fmt, vargs));
}

template <typename... T>
inline void trace(fmt::format_string<T..., decltype(XCI_LAST_ERROR_ARG)> fmt, T&&... args) {
    const auto& vargs = fmt::make_format_args(args..., XCI_LAST_ERROR_ARG);
    Logger::default_instance().log(Logger::Level::Trace, fmt::vformat(fmt, vargs));
}

template <typename... T>
inline void debug(fmt::format_string<T..., decltype(XCI_LAST_ERROR_ARG)> fmt, T&&... args) {
    const auto& vargs = fmt::make_format_args(args..., XCI_LAST_ERROR_ARG);
    Logger::default_instance().log(Logger::Level::Debug, fmt::vformat(fmt, vargs));
}

template <typename... T>
inline void info(fmt::format_string<T..., decltype(XCI_LAST_ERROR_ARG)> fmt, T&&... args) {
    const auto& vargs = fmt::make_format_args(args..., XCI_LAST_ERROR_ARG);
    Logger::default_instance().log(Logger::Level::Info, fmt::vformat(fmt, vargs));
}

template <typename... T>
inline void warning(fmt::format_string<T..., decltype(XCI_LAST_ERROR_ARG)> fmt, T&&... args) {
    const auto& vargs = fmt::make_format_args(args..., XCI_LAST_ERROR_ARG);
    Logger::default_instance().log(Logger::Level::Warning, fmt::vformat(fmt, vargs));
}

template <typename... T>
inline void error(fmt::format_string<T..., decltype(XCI_LAST_ERROR_ARG)> fmt, T&&... args) {
    const auto& vargs = fmt::make_format_args(std::forward<T>(args)..., XCI_LAST_ERROR_ARG);
    Logger::default_instance().log(Logger::Level::Error, fmt::vformat(fmt, vargs));
}

} // namespace log
} // namespace xci::core


#ifdef XCI_DEBUG_TRACE
#define TRACE(fmt, ...)  xci::core::log::trace("{}:{} ({}) " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define TRACE(fmt, ...)  ((void)0)
#endif


template <>
struct [[maybe_unused]] fmt::formatter<xci::core::log::LastErrorPlaceholder> {
    bool last_error = false;    // l -> true
    bool error_code = false;    // s -> false, d -> true

    // Parses format specifications of the form ['l']['s' | 'd'].
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin();  // NOLINT
        while (it != ctx.end() && *it != '}') {
            switch (*it) {
                case 'l':
                    last_error = true;
                    break;
                case 's':
                    error_code = false;
                    break;
                case 'd':
                    error_code = true;
                    break;
                default:
                    throw fmt::format_error("invalid format for last error, expected: [l][s|d]");
            }
            ++it;
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const xci::core::log::LastErrorPlaceholder& p, FormatContext& ctx) const -> decltype(ctx.out()) {
        auto msg = xci::core::log::LastErrorPlaceholder::message(last_error, error_code);
        return std::copy(msg.begin(), msg.end(), ctx.out());
    }
};


template <>
struct fmt::formatter<std::filesystem::path> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();  // NOLINT
        if (it != ctx.end() && *it != '}')
            throw fmt::format_error("invalid format for std::filesystem::path");
        return it;
    }

    template <typename FormatContext>
    auto format(const std::filesystem::path& p, FormatContext& ctx) const {
        const auto& msg = p.string();
        return std::copy(msg.begin(), msg.end(), ctx.out());
    }
};


#endif // XCI_CORE_LOG_H
