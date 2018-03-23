// Logger.h created on 2018-03-01, part of XCI toolkit

#ifndef XCI_UTIL_LOG_H
#define XCI_UTIL_LOG_H

#include <xci/util/format.h>

namespace xci {
namespace util {


// TODO: Provides configuration options, passes messages to handlers
class Logger
{
public:
    static Logger& get_default_instance();

    enum class Level {
        Error,
        Warning,
        Info,
        Debug,
    };

    void log(Level lvl, std::string && msg);
};


template<typename... Args>
inline void log_error(const char *fmt, Args&&... args) {
    Logger::get_default_instance().log(
            Logger::Level::Error,
            xci::util::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_warning(const char *fmt, Args&&... args) {
    Logger::get_default_instance().log(
            Logger::Level::Warning,
            xci::util::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_info(const char *fmt, Args&&... args) {
    Logger::get_default_instance().log(
            Logger::Level::Info,
            xci::util::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_debug(const char *fmt, Args&&... args) {
    Logger::get_default_instance().log(
            Logger::Level::Debug,
            xci::util::format(fmt, std::forward<Args>(args)...));
}

namespace log {
    using xci::util::log_error;
    using xci::util::log_warning;
    using xci::util::log_info;
    using xci::util::log_debug;
}

}} // namespace xci::log

#endif // XCI_UTIL_LOG_H
