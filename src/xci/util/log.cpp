// Logger.h created on 2018-03-01, part of XCI toolkit

#include <xci/util/log.h>
#include <xci/util/Term.h>
#include <xci/util/sys.h>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <cassert>
#include <unistd.h>
#include "log.h"


namespace xci {
namespace util {


static const char* level_format[] = {
        "{:20} {cyan}{}{normal}  {bold}ERROR{normal}  {bold}{red}{}{normal}\n",
        "{:20} {cyan}{}{normal}  {bold}WARN {normal}  {bold}{yellow}{}{normal}\n",
        "{:20} {cyan}{}{normal}  {bold}INFO {normal}  {bold}{white}{}{normal}\n",
        "{:20} {cyan}{}{normal}  {bold}DEBUG{normal}  {white}{}{normal}\n",
};


Logger& Logger::default_instance()
{
    static Logger logger;
    return logger;
}


Logger::Logger()
{
    Term& t = Term::stderr_instance();
    auto msg = t.format("{underline}   Date      Time     TID   Level  Message   {normal}\n");
    ::write(STDERR_FILENO, msg.data(), msg.size());
}


Logger::~Logger()
{
    Term& t = Term::stderr_instance();
    auto msg = t.format("{overline}                 End of Log                   {normal}\n");
    ::write(STDERR_FILENO, msg.data(), msg.size());
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
    auto formatted_msg = t.format(level_format[lvl_num], ts_buf, get_thread_id(), msg);
    ::write(STDERR_FILENO, formatted_msg.data(), formatted_msg.size());
}


}} // namespace xci::log
