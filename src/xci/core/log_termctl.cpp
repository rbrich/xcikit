// log_termctl.cpp created on 2021-03-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "log.h"
#include <xci/core/TermCtl.h>
#include <xci/core/string.h>  // split
#include <xci/core/sys.h>  // get_thread_id

#include <fmt/chrono.h>
#include <ctime>

namespace xci::core {


// 0..4 => log level (Trace..Error)
// 5..9 => multi-line continuation for each log level
static constexpr size_t c_cont = 5;
static constexpr const char* c_log_format[] = {
        "{0:%F %T} <fg:cyan>{1:6x}<t:normal>  <t:bold>TRACE<t:normal>  <fg:blue>{2}<t:normal>\n",
        "{0:%F %T} <fg:cyan>{1:6x}<t:normal>  <t:bold>DEBUG<t:normal>  <fg:white>{2}<t:normal>\n",
        "{0:%F %T} <fg:cyan>{1:6x}<t:normal>  <t:bold>INFO <t:normal>  <t:bold><fg:white>{2}<t:normal>\n",
        "{0:%F %T} <fg:cyan>{1:6x}<t:normal>  <t:bold>WARN <t:normal>  <t:bold><fg:yellow>{2}<t:normal>\n",
        "{0:%F %T} <fg:cyan>{1:6x}<t:normal>  <t:bold>ERROR<t:normal>  <t:bold><fg:red>{2}<t:normal>\n",
        "                            <t:bold>...<t:normal>    <fg:blue>{2}<t:normal>\n",
        "                            <t:bold>...<t:normal>    <fg:white>{2}<t:normal>\n",
        "                            <t:bold>...<t:normal>    <t:bold><fg:white>{2}<t:normal>\n",
        "                            <t:bold>...<t:normal>    <t:bold><fg:yellow>{2}<t:normal>\n",
        "                            <t:bold>...<t:normal>    <t:bold><fg:red>{2}<t:normal>\n",
};
static constexpr const char* c_log_intro = "<t:underline>   Date      Time    TID    Level  Message   <t:normal>\n";


Logger::Logger(Level level) : m_level(level)
{
    if (m_level <= Level::Info) {
        TermCtl& t = TermCtl::stderr_instance();
        t.print(c_log_intro);
    }
}


void Logger::default_handler(Logger::Level lvl, std::string_view msg)
{
    TermCtl& t = TermCtl::stderr_instance();
    const auto lvl_num = static_cast<int>(lvl);
    const auto tm = fmt::localtime(std::time(nullptr));
    const auto tid = get_thread_id() & 0xFFFFFF;  // clip to 6 hex digits
    const auto lines = split(msg, '\n');
    size_t cont = 0;
    for (const auto line : lines) {
        t.print(fmt::runtime(c_log_format[lvl_num + cont]), tm, tid, line);
        cont = c_cont;
    }
}


} // namespace xci::core
