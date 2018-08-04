// test_util.cpp created on 2018-03-30, part of XCI toolkit

#include "catch.hpp"

#include <xci/util/format.h>
#include <xci/util/log.h>
#include <xci/util/FileWatch.h>
#include <xci/util/string.h>

#include <fstream>
#include <cstdio>
#include <unistd.h>

using namespace xci::util;

TEST_CASE( "Format placeholders", "[format]" )
{
    CHECK(format("") == "");
    CHECK(format("hello there") == "hello there");
    CHECK(format("{unknown} placeholders {!!!}") == "{unknown} placeholders {!!!}");

    CHECK(format("number {} str {}", 123, "hello") == "number 123 str hello");

    CHECK(format("surplus placeholder {}{}", "left as is") == "surplus placeholder left as is{}");

    CHECK(format("hex {:x} dec {}", 255, 255) == "hex ff dec 255");
    CHECK(format("hex {:02X} dec {:03}", 15, 15) == "hex 0F dec 015");

    float f = 1.2345678f;
    CHECK(format("float {} {:.2} {:.3f} {:.3f}", f, f, f, 1.2) ==
                 "float 1.23457 1.2 1.235 1.200");

    errno = EACCES;
    CHECK(format("error: {m}") == "error: Permission denied");
}


TEST_CASE( "File watch", "[FileWatch]" )
{
    FileWatch& fw = FileWatch::default_instance();

    std::string tmpname = "/tmp/xci_test_filewatch.XXXXXX";
    close(mkstemp(&tmpname[0]));
    std::ofstream f(tmpname);

    FileWatch::Event expected_events[] = {
        FileWatch::Event::Modify,  // one
        FileWatch::Event::Modify,  // two
        FileWatch::Event::Modify,  // three
        FileWatch::Event::Delete,  // unlink
    };
    size_t ev_ptr = 0;
    size_t ev_size = sizeof(expected_events) / sizeof(expected_events[0]);
    int wd = fw.add_watch(tmpname,
            [&expected_events, &ev_ptr, ev_size] (FileWatch::Event ev)
    {
        CHECK(ev_ptr < ev_size);
        CHECK(expected_events[ev_ptr] == ev);
        ev_ptr++;
    });

    // modify
    f << "one" << std::endl;
    usleep(50000);

    // modify, close
    f << "two" << std::endl;
    f.close();
    usleep(50000);

    // reopen, modify, close
    f.open(tmpname, std::ios::app);
    f << "three" << std::endl;
    f.close();
    usleep(50000);

    // delete
    ::unlink(tmpname.c_str());
    usleep(50000);

    // although the inotify watch is removed automatically after delete,
    // this should still be called to cleanup the callback info
    fw.remove_watch(wd);

    CHECK(ev_ptr == ev_size);  // got all expected events
}


TEST_CASE( "utf8_length", "[string]" )
{
    std::string s = "河北梆子";
    CHECK(s.size() == 4 * 3);
    CHECK(utf8_length(s) == 4);

    // count characters backwards
    auto pos = s.crbegin();
    int size = 0;
    while (pos != s.crend()) {
        pos = utf8_prev(pos);
        ++size;
    }
    CHECK(size == 4);
}
