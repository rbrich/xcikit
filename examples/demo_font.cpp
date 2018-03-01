// demo_font.cpp created on 2018-03-02, part of XCI toolkit

#include <xci/text/FontLibrary.h>

#include <iostream>
#include <thread>
#include <mutex>

using namespace xci::text;

std::mutex cout_mutex;

void thread_run(const std::string& thread_name)
{
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << thread_name << ": "
              << (size_t) FontLibrary::get_default_instance().get()
              << std::endl;
}

int main()
{
    // each thread has its own static instance of FontLibrary
    std::cout << "main: "
              << (size_t) FontLibrary::get_default_instance().get()
              << std::endl;

    std::thread a(thread_run, "thread1"), b(thread_run, "thread2");

    a.join();
    b.join();
}
