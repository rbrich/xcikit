// Logger.h created on 2018-03-01, part of XCI toolkit

#ifndef XCI_UTIL_LOG_H
#define XCI_UTIL_LOG_H

#include <xci/util/format.h>
#include <xci/config.h>

namespace xci {
namespace util {


// TODO: Provide configuration options,
//       pass messages to handlers instead of cerr directly
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

    void log(Level lvl, const std::string& msg);

private:
    Level m_level;
};


template<typename... Args>
inline void log_error(const char *fmt, Args&&... args) {
    Logger::default_instance().log(
            Logger::Level::Error,
            xci::util::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_warning(const char *fmt, Args&&... args) {
    Logger::default_instance().log(
            Logger::Level::Warning,
            xci::util::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_info(const char *fmt, Args&&... args) {
    Logger::default_instance().log(
            Logger::Level::Info,
            xci::util::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_debug(const char *fmt, Args&&... args) {
    Logger::default_instance().log(
            Logger::Level::Debug,
            xci::util::format(fmt, std::forward<Args>(args)...));
}

namespace log {
    using xci::util::log_error;
    using xci::util::log_warning;
    using xci::util::log_info;
    using xci::util::log_debug;
}


#ifdef XCI_DEBUG_TRACE
#define TRACE(fmt, ...)  log_debug("{}:{} ({}) " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define TRACE(fmt, ...)
#endif


}} // namespace xci::log

#endif // XCI_UTIL_LOG_H
