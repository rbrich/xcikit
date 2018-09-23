// demo_logger.cpp created on 2018-03-02, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <xci/core/log.h>
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

    return EXIT_SUCCESS;
}
