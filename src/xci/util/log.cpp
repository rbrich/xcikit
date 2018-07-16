// Logger.h created on 2018-03-01, part of XCI toolkit

#include <xci/util/log.h>
#include <xci/util/Term.h>

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


static const Term::Color level_color[] = {
        Term::Color::Red,
        Term::Color::Yellow,
        Term::Color::White,
        Term::Color::White,
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
    (void) ts_res;  // unused with NDEBUG
    assert(ts_res > 0 && ts_res < sizeof(ts_buf));

    auto lvl_num = static_cast<int>(lvl);

    Term& t = Term::stderr_instance();
    fputs(format("{} {}{}{}  {}{}{}{}\n",
                 ts_buf,
                 t.bold(), level_string[lvl_num], t.normal(),
                 (lvl < Logger::Level::Debug ? t.bold() : t.normal()),
                 t.fg(level_color[lvl_num]), msg, t.normal()
          ).c_str(),
          stderr);
}


}} // namespace xci::log
