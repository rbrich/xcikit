// demo_logger.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/log.h>
#include <xci/core/sys.h>

#include <fmt/ostream.h>  // for ArbitraryObject
#include <iostream>
#include <thread>

using namespace xci::core;
using namespace xci::core::log;

class ArbitraryObject {};
std::ostream& operator<<(std::ostream& stream, const ArbitraryObject&) {
    return stream << "I am arbitrary!";
}

template <> struct fmt::formatter<ArbitraryObject> : ostream_formatter {};


void thread_run(const std::string& thread_name)
{
    info("Log from {}", thread_name);
}


int main()
{
    ArbitraryObject obj;

    debug("{} {}!", "Hello", "World");
    info("float: {} int: {}!", 1.23f, 42);
    warning("arbitrary object: {}", obj);
    error("beware");

    TRACE("trace message");

    // Custom handler
    Logger::default_instance().set_handler([](Logger::Level lvl, std::string_view msg) {
        std::cerr << "[custom handler] " << int(lvl) << ": " << msg << std::endl;
    });

    debug("{} {}!", "Hello", "World");
    info("float: {} int: {}!", 1.23f, 42);
    warning("arbitrary object: {}", obj);
    error("beware");

    // Reinstall default handler
    Logger::default_instance().set_handler(Logger::default_handler);
    info("back to normal");

    errno = ENOENT;
    error("errno: ({m:d}) {m}");

    info("[sys] HOME = {}", home_directory_path());
    info("[sys] TEMP = {}", fs::temp_directory_path());
    info("[sys] self = {}", self_executable_path());
    info("[sys] cpu time = {}s", get_cpu_time());

    // Log from threads
    std::thread a(thread_run, "thread1");
    std::thread b(thread_run, "thread2");
    a.join();
    b.join();

    return EXIT_SUCCESS;
}
