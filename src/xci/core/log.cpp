// log.cpp created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "log.h"
#include <xci/core/sys.h>
#include <xci/core/file.h>


namespace xci::core {


Logger& Logger::default_instance(Logger::Level initial_level)
{
    static Logger logger(initial_level);
    return logger;
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
    return error_str();
}


} // namespace xci::core
