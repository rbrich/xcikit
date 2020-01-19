// test_dispatch.cpp created on 2020-01-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <xci/core/dispatch.h>
#include <xci/core/log.h>
#include <xci/core/chrono.h>
#include <xci/compat/unistd.h>

#include <fstream>
#include <string>
#include <cstdio>

using namespace xci::core;
using std::this_thread::sleep_for;
using namespace std::string_literals;


TEST_CASE( "File watch", "[FSDispatch]" )
{
    Logger::init(Logger::Level::Error);
    FSDispatch fw;

    std::string tmpname = "/tmp/xci_test_filewatch.XXXXXX";
    close(mkstemp(&tmpname[0]));
    std::ofstream f(tmpname);

    FSDispatch::Event expected_events[] = {
        FSDispatch::Event::Modify,  // one
        FSDispatch::Event::Modify,  // two
        FSDispatch::Event::Modify,  // three
        FSDispatch::Event::Delete,  // unlink
    };
    size_t ev_ptr = 0;
    size_t ev_size = sizeof(expected_events) / sizeof(expected_events[0]);
    fw.add_watch(tmpname,
            [&expected_events, &ev_ptr, ev_size] (FSDispatch::Event ev)
    {
        CHECK(ev_ptr < ev_size);
        CHECK(expected_events[ev_ptr] == ev);
        ev_ptr++;
    });

    // modify
    f << "one" << std::endl;
    sleep_for(50ms);

    // modify, close
    f << "two" << std::endl;
    f.close();
    sleep_for(50ms);

    // reopen, modify, close
    f.open(tmpname, std::ios::app);
    f << "three" << std::endl;
    f.close();
    sleep_for(50ms);

    // delete
    ::unlink(tmpname.c_str());
    sleep_for(50ms);

    // although the inotify watch is removed automatically after delete,
    // this should still be called to cleanup the callback info
    fw.remove_watch(tmpname);

    CHECK(ev_ptr == ev_size);  // got all expected events
}
