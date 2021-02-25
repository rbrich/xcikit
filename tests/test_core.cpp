// test_core.cpp created on 2018-03-30, part of XCI toolkit

#include <catch2/catch.hpp>

#include <xci/core/log.h>
#include <xci/core/file.h>
#include <xci/core/string.h>
#include <xci/core/chrono.h>
#include <xci/core/memory.h>
#include <xci/core/sys.h>

#ifndef _WIN32
#include <xci/core/FileTree.h>
#endif

#include <string>
#include <cstdio>
#include <sys/stat.h>

using namespace xci::core;
using namespace std::string_literals;
using xci::core::log::format;

#define UTF8(l)  (const char*)u8 ## l


TEST_CASE( "Format placeholders", "[log]" )
{
    CHECK(format("") == "");  // NOLINT
    CHECK(format("hello there") == "hello there");

    CHECK(format("number {} str {}", 123, "hello") == "number 123 str hello");

    CHECK(format("hex {:x} dec {}", 255, 255) == "hex ff dec 255");
    CHECK(format("hex {:02X} dec {:03}", 15, 15) == "hex 0F dec 015");

    errno = EACCES;
    CHECK(format("error: {m}") == "error: Permission denied");
}


TEST_CASE( "Format char type", "[log]" )
{
    // only 'char' is special, other char-like types are just numbers
    CHECK(format("{}", char('c')) == "c");
    CHECK(format("{:c}", int('c')) == "c");
    CHECK(format("{}", (unsigned char)('c')) == "99");
    CHECK(format("{}", int8_t('c')) == "99");
    CHECK(format("{}", uint8_t('c')) == "99");
    CHECK(format("{:02x}", char('c')) == "63");
}


TEST_CASE( "read_binary_file", "[file]" )
{
#ifndef __EMSCRIPTEN__
    auto filename = self_executable_path();
#else
    fs::path filename = "test_file";
#endif
    INFO(filename.string());
    auto content = read_binary_file(filename);
    REQUIRE(bool(content));

    CHECK(fs::file_size(filename) == content->size());
    CHECK(content.use_count() == 1);
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


TEST_CASE( "utf8_codepoint", "[string]" )
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


#ifndef _WIN32
TEST_CASE( "PathNode", "[FileTree]" )
{
    using PathNode = FileTree::PathNode;
    SECTION("dir_path") {
        CHECK(PathNode::make("")->dir_path() == "");
        CHECK(PathNode::make(".")->dir_path() == "./");
        CHECK(PathNode::make("/")->dir_path() == "/");
        CHECK(PathNode::make("foo")->dir_path() == "foo/");
        CHECK(PathNode::make("/foo/bar")->dir_path() == "/foo/bar/");
        CHECK(PathNode::make("/foo/bar/")->dir_path() == "/foo/bar/");
    };
    SECTION("parent_dir_name") {
        CHECK(PathNode::make("")->parent_dir_path() == "");
        CHECK(PathNode::make(".")->parent_dir_path() == "");
        CHECK(PathNode::make("/")->parent_dir_path() == "/");
        CHECK(PathNode::make("foo")->parent_dir_path() == "");
        CHECK(PathNode::make("./foo")->parent_dir_path() == "./");
        CHECK(PathNode::make("foo/bar")->parent_dir_path() == "foo/");
        CHECK(PathNode::make("/foo/bar")->parent_dir_path() == "/foo/");
    };
}
#endif // _WIN32
