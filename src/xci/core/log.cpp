// Logger.h created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "log.h"
#include <xci/core/TermCtl.h>
#include <xci/core/sys.h>
#include <xci/core/file.h>
#include <xci/compat/unistd.h>

#include <ctime>
#include <cassert>


namespace xci::core {


static const char* level_format[] = {
        "{:19} {cyan}{:5}{normal}  {bold}DEBUG{normal}  {white}{}{normal}\n",
        "{:19} {cyan}{:5}{normal}  {bold}INFO {normal}  {bold}{white}{}{normal}\n",
        "{:19} {cyan}{:5}{normal}  {bold}WARN {normal}  {bold}{yellow}{}{normal}\n",
        "{:19} {cyan}{:5}{normal}  {bold}ERROR{normal}  {bold}{red}{}{normal}\n",
};


Logger& Logger::default_instance(Logger::Level initial_level)
{
    static Logger logger(initial_level);
    return logger;
}


Logger::Logger(Level level) : m_level(level)
{
    if (m_level <= Level::Info) {
        TermCtl& t = TermCtl::stderr_instance();
        auto msg = t.format("{underline}   Date      Time    TID   Level  Message   {normal}\n");
        auto res = write(STDERR_FILENO, msg);
        assert(res);  // write to stderr should not fail
        (void) res;
    }
}


Logger::~Logger()
{
    if (m_level <= Level::Info) {
        TermCtl& t = TermCtl::stderr_instance();
        auto msg = t.format("{overline}                 End of Log                 {normal}\n");
        auto res = write(STDERR_FILENO, msg);
        assert(res);  // write to stderr should not fail
        (void) res;
    }
}


static inline std::string format_current_time()
{
    time_t now = std::time(nullptr);
    std::string ts_buf(20, '\0');
    size_t ts_res = std::strftime(&ts_buf[0], ts_buf.size(), "%F %T", std::localtime(&now));
    assert(ts_res > 0 && ts_res < ts_buf.size());
    ts_buf.resize(ts_res);
    return ts_buf;
}


void Logger::default_handler(Logger::Level lvl, const std::string& msg)
{
    TermCtl& t = TermCtl::stderr_instance();
    auto lvl_num = static_cast<int>(lvl);
    auto formatted_msg = t.format(level_format[lvl_num], format_current_time(), get_thread_id(), msg);
    auto res = write(STDERR_FILENO, formatted_msg);
    assert(res);  // write to stderr should not fail
    (void) res;
}


void Logger::log(Logger::Level lvl, const std::string& msg)
{
    if (lvl < m_level)
        return;

    m_handler(lvl, msg);
}


} // namespace xci::core
