// log.h created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_LOG_H
#define XCI_CORE_LOG_H

#include <xci/config.h>
#include <string_view>
#include <filesystem>
#include <fmt/format.h>

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
    };

    // Initialize default logger, Call this before anything that logs
    // to make sure logger is created before it (and destroyed after).
    // If not called, default logger will be created lazily
    // (at the time of first use).
    static void init(Level level = Level::Trace) { (void) default_instance(level); }

    static Logger& default_instance(Logger::Level initial_level = Level::Trace);

    explicit Logger(Level level);
    ~Logger();

    // Set minimal level of messages to be logged.
    // Messages below this level are dropped.
    void set_level(Level level) { m_level = level; }

    // Customizable log handler
    // A function with same signature as `default_handler` can be used
    // instead of default handler. The function parameters are preformatted
    // messages and log level. The handler has to add timestamp by itself.
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

template<typename ...Args>
inline std::string format(const char *fmt, Args&&... args)
{
    return fmt::format(fmt, std::forward<Args>(args)...,
                       fmt::arg("m", LastErrorPlaceholder{}));
}

template<typename... Args>
inline void message(Logger::Level lvl, const char *fmt, Args&&... args) {
    Logger::default_instance().log(lvl, format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void trace(const char *fmt, Args&&... args) {
    message(Logger::Level::Trace, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void debug(const char *fmt, Args&&... args) {
    message(Logger::Level::Debug, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void info(const char *fmt, Args&&... args) {
    message(Logger::Level::Info, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void warning(const char *fmt, Args&&... args) {
    message(Logger::Level::Warning, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void error(const char *fmt, Args&&... args) {
    message(Logger::Level::Error, fmt, std::forward<Args>(args)...);
}

}  // namespace log
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
    constexpr auto parse(format_parse_context& ctx) {
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
    auto format(const xci::core::log::LastErrorPlaceholder& p, FormatContext& ctx) {
        auto msg = xci::core::log::LastErrorPlaceholder::message(last_error, error_code);
        return std::copy(msg.begin(), msg.end(), ctx.out());
    }
};


template <>
struct [[maybe_unused]] fmt::formatter<std::filesystem::path> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();  // NOLINT
        if (it != ctx.end() && *it != '}')
            throw fmt::format_error("invalid format for std::filesystem::path");
        return it;
    }

    template <typename FormatContext>
    auto format(const std::filesystem::path& p, FormatContext& ctx) {
        const auto& msg = p.string();
        return std::copy(msg.begin(), msg.end(), ctx.out());
    }
};


#endif // XCI_CORE_LOG_H
