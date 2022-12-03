// log_termctl.cpp created on 2021-03-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "log.h"
#include <xci/core/TermCtl.h>
#include <xci/core/sys.h>  // get_thread_id

#include <fmt/chrono.h>
#include <ctime>

namespace xci::core {


// 0..4 => log level (Trace..Error)
// 5, 6 => intro, outro
static constexpr size_t c_intro = 5;
static constexpr size_t c_outro = 6;
static const char* c_log_format[] = {
        "{:%F %T} {fg:cyan}{:5}{t:normal}  {t:bold}TRACE{t:normal}  {fg:blue}{}{t:normal}\n",
        "{:%F %T} {fg:cyan}{:5}{t:normal}  {t:bold}DEBUG{t:normal}  {fg:white}{}{t:normal}\n",
        "{:%F %T} {fg:cyan}{:5}{t:normal}  {t:bold}INFO {t:normal}  {t:bold}{fg:white}{}{t:normal}\n",
        "{:%F %T} {fg:cyan}{:5}{t:normal}  {t:bold}WARN {t:normal}  {t:bold}{fg:yellow}{}{t:normal}\n",
        "{:%F %T} {fg:cyan}{:5}{t:normal}  {t:bold}ERROR{t:normal}  {t:bold}{fg:red}{}{t:normal}\n",
        "{t:underline}   Date      Time    TID   Level  Message   {t:normal}\n",
        "{t:overline}                 End of Log                 {t:normal}\n",
};


Logger::Logger(Level level) : m_level(level)
{
    if (m_level <= Level::Info) {
        TermCtl& t = TermCtl::stderr_instance();
        t.print(c_log_format[c_intro]);
    }
}


Logger::~Logger()
{
    if (m_level <= Level::Info) {
        TermCtl& t = TermCtl::stderr_instance();
        t.print(c_log_format[c_outro]);
    }
}


void Logger::default_handler(Logger::Level lvl, std::string_view msg)
{
    TermCtl& t = TermCtl::stderr_instance();
    auto lvl_num = static_cast<int>(lvl);
    t.print(c_log_format[lvl_num], fmt::localtime(std::time(nullptr)), get_thread_id(), msg);
}


} // namespace xci::core
