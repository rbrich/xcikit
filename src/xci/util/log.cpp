// Logger.h created on 2018-03-01, part of XCI toolkit

#include <xci/util/log.h>

#include <iostream>
#include <ctime>
#include <iomanip>

namespace xci {
namespace util {


static const char* level_string[] = {
        "ERROR",
        "WARN ",
        "INFO ",
        "DEBUG",
};


Logger& Logger::get_default_instance()
{
    static Logger logger;
    return logger;
}


void Logger::log(Logger::Level lvl, std::string&& msg)
{
    auto now = std::time(nullptr);
    std::cerr
        << std::put_time(std::localtime(&now), "%F %T ")
        << level_string[(int)lvl] << "  " << msg << std::endl;
}


}} // namespace xci::log
