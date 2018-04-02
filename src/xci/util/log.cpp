// Logger.h created on 2018-03-01, part of XCI toolkit

#include <xci/util/log.h>

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <ctime>
#include <cassert>

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


void Logger::log(Logger::Level lvl, const std::string& msg)
{
    time_t now = std::time(nullptr);
    char ts_buf[20];
    size_t ts_res = std::strftime(ts_buf, sizeof(ts_buf), "%F %T", std::localtime(&now));
    assert(ts_res > 0 && ts_res < sizeof(ts_buf));

    fprintf(stderr, "%s %s  %s\n", ts_buf, level_string[(int)lvl], msg.c_str());
}


}} // namespace xci::log
