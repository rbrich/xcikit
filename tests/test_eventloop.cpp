// test_eventloop.cpp created on 2019-03-25, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <xci/core/event.h>
#include <xci/core/chrono.h>

#include <thread>
#include <unistd.h>

using namespace xci::core;
using std::this_thread::sleep_for;


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
    ::write(pipe_rw[1], data, 1);

    loop.run();

    CHECK(ev_count == 1);
    ::close(pipe_rw[0]);
    ::close(pipe_rw[1]);
}


TEST_CASE( "Timer events", "[core][event][TimerWatch]" )
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


TEST_CASE( "FS events", "[core][event][FSWatch]" )
{
    EventLoop loop;

    FSWatch::Event expected_events[] = {
            FSWatch::Event::Modify,  // one
            FSWatch::Event::Modify,  // two
            FSWatch::Event::Modify,  // three
            FSWatch::Event::Delete,  // unlink
    };
    size_t ev_ptr = 0;
    size_t ev_size = sizeof(expected_events) / sizeof(expected_events[0]);

    std::string tmpname = "/tmp/xci_test_filewatch.XXXXXX";
    close(mkstemp(&tmpname[0]));

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
        // open
        std::ofstream f(tmpname);
        sleep_for(50ms);

        // modify
        f << "1\n";
        sleep_for(50ms);

        // modify, close
        f << "2\n";
        f.close();
        sleep_for(50ms);

        // reopen, modify, close
        f.open(tmpname, std::ios::app);
        f << "3\n";
        f.close();
        sleep_for(50ms);

        // delete
        ::unlink(tmpname.c_str());
        sleep_for(50ms);

        quit_cond.fire();
    });

    loop.run();
    t.join();

    CHECK(ev_ptr == ev_size);  // got all expected events
}
