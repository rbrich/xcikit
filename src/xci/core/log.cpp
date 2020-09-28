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
        "{:19} {fg:cyan}{:5}{t:normal}  {t:bold}TRACE{t:normal}  {fg:blue}{}{t:normal}\n",
        "{:19} {fg:cyan}{:5}{t:normal}  {t:bold}DEBUG{t:normal}  {fg:white}{}{t:normal}\n",
        "{:19} {fg:cyan}{:5}{t:normal}  {t:bold}INFO {t:normal}  {t:bold}{fg:white}{}{t:normal}\n",
        "{:19} {fg:cyan}{:5}{t:normal}  {t:bold}WARN {t:normal}  {t:bold}{fg:yellow}{}{t:normal}\n",
        "{:19} {fg:cyan}{:5}{t:normal}  {t:bold}ERROR{t:normal}  {t:bold}{fg:red}{}{t:normal}\n",
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
        auto msg = t.format("{t:underline}   Date      Time    TID   Level  Message   {t:normal}\n");
        auto res = write(STDERR_FILENO, msg);
        assert(res);  // write to stderr should not fail
        (void) res;
    }
}


Logger::~Logger()
{
    if (m_level <= Level::Info) {
        TermCtl& t = TermCtl::stderr_instance();
        auto msg = t.format("{t:overline}                 End of Log                 {t:normal}\n");
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


void Logger::default_handler(Logger::Level lvl, std::string_view msg)
{
    TermCtl& t = TermCtl::stderr_instance();
    auto lvl_num = static_cast<int>(lvl);
    auto formatted_msg = t.format(level_format[lvl_num], format_current_time(), get_thread_id(), msg);
    auto res = write(STDERR_FILENO, formatted_msg);
    assert(res);  // write to stderr should not fail
    (void) res;
}


void Logger::log(Logger::Level lvl, std::string_view msg)
{
    if (lvl < m_level)
        return;

    m_handler(lvl, msg);
}


std::string log::LastErrorPlaceholder::message(bool use_last_error, bool error_code)
{
    if (error_code) {
        if (use_last_error)
            return std::to_string(last_error());
        return std::to_string(errno);
    }
    if (use_last_error)
        return last_error_str();
    return errno_str();
}


} // namespace xci::core
