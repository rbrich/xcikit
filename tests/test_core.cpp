// test_core.cpp created on 2018-03-30, part of XCI toolkit

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <xci/core/format.h>
#include <xci/core/log.h>
#include <xci/core/file.h>
#include <xci/core/dispatch.h>
#include <xci/core/string.h>
#include <xci/core/chrono.h>

#include <fstream>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>

using namespace xci::core;
using std::this_thread::sleep_for;
using namespace std::string_literals;


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


TEST_CASE( "Format char type", "[format]" )
{
    // format uses std::ostream operator<<()
    // -> there is overload for char type:
    CHECK(format("{}", char('c')) == "c");
    CHECK(format("{}", (unsigned char)('c')) == "c");
    CHECK(format("{}", int8_t('c')) == "c");
    CHECK(format("{}", uint8_t('c')) == "c");
    // 'x' format spec just sends std::hex, it does not convert char to int implicitly:
    CHECK(format("{:02x}", uint8_t('c')) == "0c");
    // -> if we want char's numeric value, it has to be cast:
    CHECK(format("{}", int('c')) == "99");
    CHECK(format("{:02x}", int('c')) == "63");
}


TEST_CASE( "read_binary_file", "[file]" )
{
    const char* filename = XCI_SHARE_DIR "/shaders/rectangle.vert";
    auto content = read_binary_file(filename);
    REQUIRE(bool(content));

    struct stat st = {};
    ::stat(filename, &st);
    CHECK(size_t(st.st_size) == content->size());
    CHECK(content.use_count() == 1);
}


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


TEST_CASE( "to_utf32", "[string]" )
{
    CHECK(to_utf32(u8"Červeňoučký 🦞") == U"Červeňoučký 🦞");
}


TEST_CASE( "to_utf8", "[string]" )
{
    CHECK(to_utf8(0x1F99E) == u8"🦞");
}


TEST_CASE( "to_codepoint", "[string]" )
{
    CHECK(utf8_codepoint("\n") == 0xa);
    CHECK(utf8_codepoint("#") == '#');
    CHECK(utf8_codepoint("ž") == 0x017E);
    CHECK(utf8_codepoint("€") == 0x20AC);

    std::string s3 = "人";
    CHECK(s3.size() == 3);
    CHECK(utf8_length(s3) == 1);
    CHECK(utf8_codepoint(s3.data()) == 0x4EBA);

    std::string s4 = "🦞";
    CHECK(s4.size() == 4);
    CHECK(utf8_length(s4) == 1);
    CHECK(utf8_codepoint(s4.data()) == 0x1F99E);
}


TEST_CASE( "escape", "[string]" )
{
    CHECK(escape("abc\0"s) == "abc\\0");
    CHECK(escape("\1\2\3\4\5\6") == "\\1\\2\\3\\4\\5\\6");
    CHECK(escape("\x07\x08\x09\x0a\x0b\x0c") == "\\a\\b\\t\\n\\v\\f");
    CHECK(escape("\x0d\x0e\x0f\x10\x1a\x1b") == "\\r\\x0e\\x0f\\x10\\x1a\\x1b");
    CHECK(escape("\x80\xff") == "\\x80\\xff");
}


TEST_CASE( "unescape", "[string]" )
{
    CHECK(unescape("abc\\n") == "abc\n"s);
    CHECK(unescape("\\0\\1\\2\\3\\4\\5\\6") == "\0\1\2\3\4\5\6"s);
    CHECK(unescape("\\a\\b\\t\\n\\v\\f") == "\x07\x08\x09\x0a\x0b\x0c");
    CHECK(unescape("\\r\\x0e\\x0f\\x10\\x1a\\x1b") == "\x0d\x0e\x0f\x10\x1a\x1b");
    CHECK(unescape("\\x80\\xff") == "\x80\xff");
    // ill-formatted:
    CHECK(unescape("trailing backslash \\") == "trailing backslash ");
    CHECK(unescape("bad esc \\J\\X\\\\") == "bad esc JX\\");
}


TEST_CASE( "to_lower", "[string]" )
{
    CHECK(to_lower("HELLO!") == "hello!");
}


TEST_CASE( "utf8_partial_end", "[string]" )
{
    CHECK(utf8_partial_end("") == 0);
    CHECK(utf8_partial_end("hello") == 0);

    std::string s = "fň";
    REQUIRE(s.size() == 3);  // 1 + 2
    CHECK(utf8_partial_end(s) == 0);
    CHECK(utf8_partial_end(s.substr(0, 2)) == 1);
    CHECK(utf8_partial_end(s.substr(0, 1)) == 0);

    s = "€";
    REQUIRE(s.size() == 3);
    CHECK(utf8_partial_end(s) == 0);
    CHECK(utf8_partial_end(s.substr(0, 2)) == 2);
    CHECK(utf8_partial_end(s.substr(0, 1)) == 1);

    s = "😈";  // F0 9F 98 88
    REQUIRE(s.size() == 4);
    CHECK(utf8_partial_end(s) == 0);
    CHECK(utf8_partial_end(s.substr(0, 3)) == 3);
    CHECK(utf8_partial_end(s.substr(0, 2)) == 2);
    CHECK(utf8_partial_end(s.substr(0, 1)) == 1);
}


TEST_CASE( "split", "[string]" )
{
    {
        std::string s = "one\ntwo\nthree";
        auto res = split(s, '\n');
        REQUIRE(res.size() == 3);
        CHECK(res[0] == "one");
        CHECK(res[1] == "two");
        CHECK(res[2] == "three");
    }
    {
        // empty substrings are skipped
        std::string s = "\none\ntwo\n\nthree\n";
        auto res = split(s, '\n');
        REQUIRE(res.size() == 3);
        CHECK(res[0] == "one");
        CHECK(res[1] == "two");
        CHECK(res[2] == "three");
    }
}


TEST_CASE( "starts_with", "[string]" )
{
    CHECK(starts_with("/ab/cdef", "/ab") == true);
    CHECK(starts_with("/ab/cdef", "/ab/cdef") == true);
    CHECK(starts_with("/ab/cdef", "/ab/cdef/") == false);
    CHECK(starts_with("", "") == true);
    CHECK(starts_with("abc", "") == true);
    CHECK(starts_with("", "abc") == false);
}


TEST_CASE( "lstrip", "[string]" )
{
    std::string s;
    s = "/ab/cdef/"; lstrip(s, '/');
    CHECK(s == "ab/cdef/");
    s = "/ab/cdef/"; lstrip(s, ' ');
    CHECK(s == "/ab/cdef/");
    s = "/ab/cdef/"; lstrip(s, "/ba");
    CHECK(s == "cdef/");
}


TEST_CASE( "rstrip", "[string]" )
{
    std::string s;
    s = "/ab/cdef/"; rstrip(s, '/');
    CHECK(s == "/ab/cdef");
    s = "/ab/cdef/"; rstrip(s, ' ');
    CHECK(s == "/ab/cdef/");
    s = "/ab/cdef/"; rstrip(s, "/fedc");
    CHECK(s == "/ab");
}
