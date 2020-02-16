// demo_logger.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/log.h>
#include <xci/core/sys.h>
using namespace xci::core;

#include <ostream>
using std::ostream;

#include <thread>


class ArbitraryObject {};
ostream& operator<<(ostream& stream, const ArbitraryObject&) {
    return stream << "I am arbitrary!";
}


void thread_run(const std::string& thread_name)
{
    log_info("Log from {}", thread_name);
}


int main()
{
    ArbitraryObject obj;

    log_debug("{} {}!", "Hello", "World");
    log_info("float: {} int: {}!", 1.23f, 42);
    log_warning("arbitrary object: {}", obj);
    log_error("beware");

    TRACE("trace message");

    // Custom handler
    Logger::default_instance().set_handler([](Logger::Level lvl, const std::string& msg) {
        std::cerr << "[custom handler] " << int(lvl) << ": " << msg << std::endl;
    });

    log_debug("{} {}!", "Hello", "World");
    log_info("float: {} int: {}!", 1.23f, 42);
    log_warning("arbitrary object: {}", obj);
    log_error("beware");

    // Reinstall default handler
    Logger::default_instance().set_handler(Logger::default_handler);
    log_info("back to normal");
    
    // Log from threads
    std::thread a(thread_run, "thread1"), b(thread_run, "thread2");
    a.join();
    b.join();

    log_info("[sys] HOME = {}", get_home_dir());

    return EXIT_SUCCESS;
}
