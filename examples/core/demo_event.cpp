// demo_event.cpp created on 2020-01-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/event.h>
#include <xci/core/log.h>
#include <xci/compat/unistd.h>
#include <thread>
#include <csignal>
#include <cstdlib>

using namespace xci::core;
using namespace std::chrono;

int main(int argc, char** argv)
{
    EventLoop loop;

    EventWatch event_watch(loop, [] {
        log_info("Event received.");
    });

    milliseconds elapsed {0};
    static constexpr milliseconds interval {500};
    TimerWatch timer_watch(loop, interval, [&elapsed] {
        elapsed += interval;
        log_info("Timer: {} ms", elapsed.count());
    });

    SignalWatch signal_watch(loop, {SIGTERM, SIGINT}, [&loop](int signum) {
        log_info("Signal received: {}", signum);
        loop.terminate();
    });

    std::thread t([&event_watch, &loop]{
        sleep(3);
        event_watch.fire();
        loop.terminate();
    });

    loop.run();
    t.join();
    return EXIT_SUCCESS;
}
