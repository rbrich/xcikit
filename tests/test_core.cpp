// test_core.cpp created on 2018-03-30, part of XCI toolkit

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include <xci/core/format.h>
#include <xci/core/file.h>
#include <xci/core/string.h>
#include <xci/core/chrono.h>
#include <xci/core/memory.h>
#include <xci/core/sys.h>

#include <string>
#include <cstdio>
#include <sys/stat.h>

using namespace xci::core;
using namespace std::string_literals;

#define UTF8(l)  (const char*)u8 ## l


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
    std::string filename = get_self_path();
    INFO(filename);
    auto content = read_binary_file(filename);
    REQUIRE(bool(content));

    struct stat st = {};
    ::stat(filename.c_str(), &st);
    CHECK(size_t(st.st_size) == content->size());
    CHECK(content.use_count() == 1);
}


TEST_CASE( "path::dir_name", "[file]" )
{
    CHECK(path::dir_name("/dir/name/") == "/dir");
    CHECK(path::dir_name("/dir/name") == "/dir");
    CHECK(path::dir_name("name") == ".");
    CHECK(path::dir_name(".") == ".");
    CHECK(path::dir_name("..") == ".");
    CHECK(path::dir_name("/name") == "/");
#ifdef _WIN32
    CHECK(path::dir_name("C:\\xyz\\fsd") == "C:\\xyz");
    CHECK(path::dir_name("C:\\xyz\\") == "C:\\");
#endif
}


TEST_CASE( "path::base_name", "[file]" )
{
    CHECK(path::base_name("/dir/name/") == "name");
    CHECK(path::base_name("/dir/name") == "name");
    CHECK(path::base_name("/name") == "name");
    CHECK(path::base_name("name") == "name");
    CHECK(path::base_name(".") == ".");
}


TEST_CASE( "path::real_path, path::get_cwd", "[file]" )
{
    auto cwd = path::cwd();
    CHECK(path::real_path(cwd + "/././.") == cwd);
}


TEST_CASE( "utf8_length", "[string]" )
{
    std::string s = UTF8("河北梆子");
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
    CHECK(to_utf32(UTF8("Červeňoučký 🦞")) == U"Červeňoučký 🦞");
}


TEST_CASE( "to_utf8", "[string]" )
{
    CHECK(to_utf8(0x1F99E) == UTF8("🦞"));
}


TEST_CASE( "to_codepoint", "[string]" )
{
    CHECK(utf8_codepoint(UTF8("\n")) == 0xa);
    CHECK(utf8_codepoint(UTF8("#")) == '#');
    CHECK(utf8_codepoint(UTF8("ž")) == 0x017E);
    CHECK(utf8_codepoint(UTF8("€")) == 0x20AC);

    std::string s3 = UTF8("人");
    CHECK(s3.size() == 3);
    CHECK(utf8_length(s3) == 1);
    CHECK(utf8_codepoint(s3.data()) == 0x4EBA);

    std::string s4 = UTF8("🦞");
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
    CHECK(utf8_partial_end(UTF8("")) == 0);
    CHECK(utf8_partial_end(UTF8("hello")) == 0);

    std::string s = UTF8("fň");
    REQUIRE(s.size() == 3);  // 1 + 2
    CHECK(utf8_partial_end(s) == 0);
    CHECK(utf8_partial_end(s.substr(0, 2)) == 1);
    CHECK(utf8_partial_end(s.substr(0, 1)) == 0);

    s = UTF8("€");
    REQUIRE(s.size() == 3);
    CHECK(utf8_partial_end(s) == 0);
    CHECK(utf8_partial_end(s.substr(0, 2)) == 2);
    CHECK(utf8_partial_end(s.substr(0, 1)) == 1);

    s = UTF8("😈");  // F0 9F 98 88
    REQUIRE(s.size() == 4);
    CHECK(utf8_partial_end(s) == 0);
    CHECK(utf8_partial_end(s.substr(0, 3)) == 3);
    CHECK(utf8_partial_end(s.substr(0, 2)) == 2);
    CHECK(utf8_partial_end(s.substr(0, 1)) == 1);
}


TEST_CASE( "split", "[string]" )
{
    using l = std::vector<std::string_view>;
    CHECK(split("one\ntwo\nthree", '\n') == l{"one", "two", "three"});
    CHECK(split("\none\ntwo\n\nthree\n", '\n') == l{"", "one", "two", "", "three", ""});
    CHECK(split("one, two, three", ',', 1) == l{"one", " two, three"});
}


TEST_CASE( "rsplit", "[string]" )
{
    using l = std::vector<std::string_view>;
    CHECK(rsplit("one\ntwo\nthree", '\n') == l{"one", "two", "three"});
    CHECK(rsplit("\none\ntwo\n\nthree\n", '\n') == l{"", "one", "two", "", "three", ""});
    CHECK(rsplit("one, two, three", ',', 1) == l{"one, two", " three"});
}


TEST_CASE( "starts_with", "[string]" )
{
    CHECK(starts_with("/ab/cdef", "/ab"));
    CHECK(starts_with("/ab/cdef", "/ab/cdef"));
    CHECK(!starts_with("/ab/cdef", "/ab/cdef/"));
    CHECK(starts_with("", ""));
    CHECK(starts_with("abc", ""));
    CHECK(!starts_with("", "abc"));
}


TEST_CASE( "ends_with", "[string]" )
{
    CHECK(ends_with("/ab/cdef", "cdef"));
    CHECK(ends_with("/ab/cdef", "/cdef"));
    CHECK(ends_with("/ab/cdef", "/ab/cdef"));
    CHECK(ends_with("", ""));
    CHECK(ends_with("abc", ""));
    CHECK(!ends_with("", "abc"));
}


TEST_CASE( "remove_prefix", "[string]" )
{
    std::string s;
    s = "/ab/cdef/";
    CHECK(remove_prefix(s, "/ab"));
    CHECK(s == "/cdef/");

    s = "/ab/cdef/";
    CHECK(remove_prefix(s, s));
    CHECK(s.empty());

    s = "/ab/cdef/";
    CHECK(!remove_prefix(s, "cdef/"));
}


TEST_CASE( "remove_suffix", "[string]" )
{
    std::string s;
    s = "/ab/cdef/";
    CHECK(remove_suffix(s, "cdef/"));
    CHECK(s == "/ab/");

    s = "/ab/cdef/";
    CHECK(remove_suffix(s, s));
    CHECK(s.empty());

    s = "/ab/cdef/";
    CHECK(!remove_suffix(s, "/ab"));
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


TEST_CASE( "align_to", "[memory]" )
{
    CHECK(align_to(0, 4) == 0);
    CHECK(align_to(1, 4) == 4);
    CHECK(align_to(3, 4) == 4);
    CHECK(align_to(4, 4) == 4);
    CHECK(align_to(5, 4) == 8);
    CHECK(align_to(1000, 16) == 1008);
}
