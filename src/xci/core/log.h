// Logger.h created on 2018-03-01, part of XCI toolkit

#ifndef XCI_CORE_LOG_H
#define XCI_CORE_LOG_H

#include <xci/core/format.h>
#include <xci/config.h>

namespace xci::core {


class Logger
{
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error,
    };

    // Initialize default logger, Call this before anything that logs
    // to make sure logger is created before it (and destroyed after).
    // If not called, default logger will be created lazily
    // (at the time of first use).
    static void init(Level level = Level::Debug) { (void) default_instance(level); }

    static Logger& default_instance(Logger::Level initial_level = Level::Debug);

    explicit Logger(Level level);
    ~Logger();

    // Set minimal level of messages to be logged.
    // Messages below this level are dropped.
    void set_level(Level level) { m_level = level; }

    // Customizable log handler
    // A function with same signature as `default_handler` can be used
    // instead of default handler. The function parameters are preformatted
    // messages and log level. The handler has to add timestamp by itself.
    static void default_handler(Level lvl, const std::string& msg);
    using Handler = decltype(&default_handler);
    void set_handler(Handler handler) { m_handler = handler; }

    void log(Level lvl, const std::string& msg);

private:
    Level m_level;
    Handler m_handler = default_handler;
};


template<typename... Args>
inline void log_error(const char *fmt, Args&&... args) {
    Logger::default_instance().log(
            Logger::Level::Error,
            xci::core::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_warning(const char *fmt, Args&&... args) {
    Logger::default_instance().log(
            Logger::Level::Warning,
            xci::core::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_info(const char *fmt, Args&&... args) {
    Logger::default_instance().log(
            Logger::Level::Info,
            xci::core::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_debug(const char *fmt, Args&&... args) {
    Logger::default_instance().log(
            Logger::Level::Debug,
            xci::core::format(fmt, std::forward<Args>(args)...));
}

namespace log {
    using xci::core::log_error;
    using xci::core::log_warning;
    using xci::core::log_info;
    using xci::core::log_debug;
}


#ifdef XCI_DEBUG_TRACE
#define TRACE(fmt, ...)  log_debug("{}:{} ({}) " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define TRACE(fmt, ...)
#endif


} // namespace xci::core

#endif // XCI_CORE_LOG_H
