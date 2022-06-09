// test_eventloop.cpp created on 2019-03-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/core/event.h>
#include <xci/core/dispatch.h>
#include <xci/core/log.h>
#include <xci/compat/unistd.h>

#include <thread>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>

using namespace xci::core;
using std::this_thread::sleep_for;
using namespace std::chrono_literals;
namespace fs = std::filesystem;


#ifndef _WIN32
TEST_CASE( "IO events", "[core][event][IOWatch]" )
{
    EventLoop loop;

    int pipe_rw[2];
    REQUIRE(::pipe(pipe_rw) == 0);

    int ev_count = 0;
    IOWatch io(loop, pipe_rw[0], IOWatch::Read, [&](int fd, IOWatch::Event ev){
        CHECK(fd == pipe_rw[0]);
        CHECK(ev == IOWatch::Event::Read);
        ++ ev_count;
        loop.terminate();
    });

    char data[] = {1, 0};
    (void) ::write(pipe_rw[1], data, 1u);

    loop.run();

    CHECK(ev_count == 1);
    ::close(pipe_rw[0]);
    ::close(pipe_rw[1]);
}
#endif


TEST_CASE( "Timer events", "[.][core][event][TimerWatch]" )
{
    EventLoop loop;

    int ev_count = 0;
    TimerWatch timer(loop, std::chrono::milliseconds{30}, [&](){
        ev_count ++;
        if (ev_count == 3) {
            loop.terminate();
        }
    });

    auto start_t = std::chrono::steady_clock::now();
    loop.run();
    auto end_t = std::chrono::steady_clock::now();

    CHECK(ev_count == 3);
    CHECK(end_t - start_t >= std::chrono::milliseconds{90});
    CHECK(end_t - start_t < std::chrono::milliseconds{100});

}


TEST_CASE( "FS events", "[.][core][event][FSWatch]" )
{
    EventLoop loop;

    FSWatch::Event expected_events[] = {
            FSWatch::Event::Create,  // first open
#ifndef _WIN32
            FSWatch::Event::Modify,  // one
#endif
            FSWatch::Event::Modify,  // two
            FSWatch::Event::Modify,  // three
            FSWatch::Event::Delete,  // unlink
    };
    size_t ev_ptr = 0;
    size_t ev_size = sizeof(expected_events) / sizeof(expected_events[0]);

    auto tmpname = fs::temp_directory_path() / "xci_test_fswatch";

    FSWatch fs_watch(loop);
    fs_watch.add(tmpname,
                 [&expected_events, &ev_ptr, ev_size]
                 (FSWatch::Event ev)
                 {
                     if (ev == FSWatch::Event::Attrib)
                         return;  // ignore attrib changes
                     INFO(ev_ptr);
                     CHECK(ev_ptr < ev_size);
                     CHECK(expected_events[ev_ptr] == ev);
                     ev_ptr++;
                 });

    EventWatch quit_cond(loop, [&loop](){ loop.terminate(); });

    std::thread t([&quit_cond, &tmpname](){
        sleep_for(50ms);

        // open
        std::ofstream f(tmpname);
        sleep_for(50ms);

        // modify
        f.write("1\n", 2);
        f.flush();
        sleep_for(50ms);

        // modify, close
        f.write("2\n", 2);
        f.close();
        sleep_for(50ms);

        // reopen, modify, close
        f.open(tmpname, std::ios::app);
        f.write("3\n", 2);
        f.close();
        sleep_for(50ms);

        // delete
        fs::remove(tmpname);
        sleep_for(50ms);

        quit_cond.fire();
    });

    loop.run();
    t.join();

    CHECK(ev_ptr == ev_size);  // got all expected events
}


TEST_CASE( "File watch", "[.][FSDispatch]" )
{
    Logger::init(Logger::Level::Error);
    FSDispatch fw;

    auto tmpname = fs::temp_directory_path() / "xci_test_fsdispatch";
    std::ofstream f(tmpname);

    FSDispatch::Event expected_events[] = {
#ifndef _WIN32
            FSDispatch::Event::Modify,  // one
#endif
            FSDispatch::Event::Modify,  // two
            FSDispatch::Event::Modify,  // three
            FSDispatch::Event::Delete,  // unlink
    };
    size_t ev_ptr = 0;
    size_t ev_size = std::size(expected_events);
    fw.add_watch(tmpname,
            [&expected_events, &ev_ptr, ev_size] (FSDispatch::Event ev)
            {
              CHECK(ev_ptr < ev_size);
              CHECK(expected_events[ev_ptr] == ev);
              ev_ptr++;
            });

    // modify
    f << "one" << std::endl;
    f.flush();
    sleep_for(100ms);

    // modify, close
    f << "two" << std::endl;
    f.close();
    sleep_for(100ms);

    // reopen, modify, close
    f.open(tmpname, std::ios::app);
    f << "three" << std::endl;
    f.close();
    sleep_for(100ms);

    // delete
    fs::remove(tmpname);
    sleep_for(100ms);

    // although the inotify watch is removed automatically after delete,
    // this should still be called to cleanup the callback info
    fw.remove_watch(tmpname);

    CHECK(ev_ptr == ev_size);  // got all expected events
}
