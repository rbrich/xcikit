// log_termctl.cpp created on 2021-03-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "log.h"
#include <xci/core/TermCtl.h>
#include <xci/core/sys.h>  // get_thread_id

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


Logger::Logger(Level level) : m_level(level)
{
    if (m_level <= Level::Info) {
        TermCtl& t = TermCtl::stderr_instance();
        t.print("{t:underline}   Date      Time    TID   Level  Message   {t:normal}\n");
    }
}


Logger::~Logger()
{
    if (m_level <= Level::Info) {
        TermCtl& t = TermCtl::stderr_instance();
        t.print("{t:overline}                 End of Log                 {t:normal}\n");
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
    t.print(level_format[lvl_num], format_current_time(), get_thread_id(), msg);
}


} // namespace xci::core
