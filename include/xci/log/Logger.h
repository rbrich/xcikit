// Logger.h created on 2018-03-01, part of XCI toolkit

#ifndef XCI_LOG_LOGGER_H
#define XCI_LOG_LOGGER_H

#include <xci/util/format.h>

namespace xci {
namespace log {


// Provides configuration options, passes messages to handlers
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

}} // namespace xci::log

#endif // XCI_LOG_LOGGER_H
