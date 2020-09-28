// log.h created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_LOG_H
#define XCI_CORE_LOG_H

#include <xci/core/format.h>
#include <xci/config.h>
#include <string_view>

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

template<typename... Args>
inline void message(Logger::Level lvl, const char *fmt, Args&&... args) {
    Logger::default_instance().log(lvl, xci::core::format(fmt, std::forward<Args>(args)...));
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


#endif // XCI_CORE_LOG_H
