// demo_thread.cpp created on 2018-03-05, part of XCI toolkit

#include <xci/text/FontLibrary.h>
#include <xci/util/log.h>

#include <iostream>
#include <thread>

using namespace xci::text;
using namespace xci::util::log;

void thread_run(const std::string& thread_name)
{
    log_info("{}: {:x}",
             thread_name,
             (size_t) FontLibrary::get_default_instance()->raw_handle());
}

int main()
{
    // each thread has its own static instance of FontLibrary
    log_info("main:    {:x}",
             (size_t) FontLibrary::get_default_instance()->raw_handle());

    std::thread a(thread_run, "thread1"), b(thread_run, "thread2");

    a.join();
    b.join();

    return EXIT_SUCCESS;
}
