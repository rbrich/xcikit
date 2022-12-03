// log_emscripten.cpp created on 2021-03-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "log.h"
#include <emscripten.h>

namespace xci::core {


Logger::Logger(Level level) : m_level(level) {}
Logger::~Logger() = default;


void Logger::default_handler(Logger::Level lvl, std::string_view msg)
{
    int em_level = 0;
    switch (lvl) {
        case Logger::Level::Trace:
        case Logger::Level::Debug:
            em_level = EM_LOG_DEBUG;
            break;
        case Logger::Level::Info:
            em_level = EM_LOG_INFO;
            break;
        case Logger::Level::Warning:
            em_level = EM_LOG_WARN;
            break;
        case Logger::Level::Error:
            em_level = EM_LOG_ERROR;
            break;
    }
    emscripten_log(EM_LOG_CONSOLE | em_level, "%s", std::string(msg).c_str());
}


} // namespace xci::core
