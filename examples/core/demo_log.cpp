// demo_logger.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/log.h>
#include <xci/core/sys.h>
using namespace xci::core;
using namespace xci::core::log;

#include <ostream>
using std::ostream;

#include <thread>


class ArbitraryObject {};
ostream& operator<<(ostream& stream, const ArbitraryObject&) {
    return stream << "I am arbitrary!";
}


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

    // Log from threads
    std::thread a(thread_run, "thread1");
    std::thread b(thread_run, "thread2");
    a.join();
    b.join();

    info("[sys] HOME = {}", get_home_dir());

    return EXIT_SUCCESS;
}
