// test_core.cpp created on 2018-03-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/core/file.h>
#include <xci/core/string.h>
#include <xci/core/memory.h>
#include <xci/core/sys.h>
#include <xci/core/TermCtl.h>

#ifndef _WIN32
#include <xci/core/FileTree.h>
#endif

#include <string>

using namespace xci::core;
using namespace std::string_literals;


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
    CHECK(to_utf32("Červeňoučký 🦞") == U"Červeňoučký 🦞");
}


TEST_CASE( "to_utf8", "[string]" )
{
    CHECK(to_utf8(0x00026) == "&");  // 0x00000000 - 0x0000007F
    CHECK(to_utf8(0x000C6) == "Æ");  // 0x00000080 - 0x000007FF
    CHECK(to_utf8(0x00B6F) == "୯");  // 0x00000800 - 0x0000FFFF
    CHECK(to_utf8(0x1F99E) == "🦞");  // 0x00010000 - 0x001FFFFF
    CHECK(to_utf8(U"ÆĳǌifѪ🦞") == "ÆĳǌifѪ🦞");
}


TEST_CASE( "utf8_codepoint", "[string]" )
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
    CHECK(escape("abc\0"s) == "abc\\x00");
    CHECK(escape("\1\2\3\4\5\6") == "\\x01\\x02\\x03\\x04\\x05\\x06");
    CHECK(escape("\x07\x08\x09\x0a\x0b\x0c") == "\\a\\b\\t\\n\\v\\f");
    CHECK(escape("\x0d\x0e\x0f\x10\x1a\x1b") == "\\r\\x0e\\x0f\\x10\\x1a\\x1b");
    CHECK(escape("\x80\xff") == "\\x80\\xff");
    // UTF-8
    CHECK(escape("černěný") == "\\xc4\\x8dern\\xc4\\x9bn\\xc3\\xbd");
    CHECK(escape_utf8("černěný") == "černěný");
}


TEST_CASE( "unescape", "[string]" )
{
    CHECK(unescape("abc\\n") == "abc\n"s);
    CHECK(unescape("\\0\\1\\2\\3\\4\\5\\6") == "\0\1\2\3\4\5\6"s);
    CHECK(unescape("\\a\\b\\t\\n\\v\\f") == "\a\b\t\n\v\f");
    CHECK(unescape("\\r\\x0e\\x0f\\x10\\x1a\\x1b") == "\r\x0e\x0f\x10\x1a\x1b");
    CHECK(unescape("\\x80\\xff") == "\x80\xff");
    CHECK(unescape_uni("\\u{ABCD} \\u{FF}") == "\xEA\xAF\x8D \xC3\xBF");  // Unicode char -> UTF-8
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
    using l = std::vector<std::string_view>;
    CHECK(split("one\ntwo\nthree", '\n') == l{"one", "two", "three"});
    CHECK(split("\none\ntwo\n\nthree\n", '\n') == l{"", "one", "two", "", "three", ""});
    CHECK(split("one, two, three", ',', 1) == l{"one", " two, three"});
    CHECK(split("one::two::three", "::") == l{"one", "two", "three"});
}


TEST_CASE( "split_ws", "[string]" )
{
    using l = std::vector<std::string_view>;
    CHECK(split_ws("one\ntwo\nthree") == l{"one", "two", "three"});
    CHECK(split_ws("\none\ntwo\n\nthree\n") == l{"one", "two", "three"});
    CHECK(split_ws("one  two\r\nthree\r\n") == l{"one", "two", "three"});
    CHECK(split_ws("one two three\n", 1) == l{"one", "two three\n"});
}


TEST_CASE( "rsplit", "[string]" )
{
    using l = std::vector<std::string_view>;
    CHECK(rsplit("one\ntwo\nthree", '\n') == l{"one", "two", "three"});
    CHECK(rsplit("\none\ntwo\n\nthree\n", '\n') == l{"", "one", "two", "", "three", ""});
    CHECK(rsplit("one, two, three", ',', 1) == l{"one, two", " three"});
    CHECK(rsplit("one::two::three", "::") == l{"one", "two", "three"});
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


TEST_CASE( "c32_width", "[TermCtl]" )
{
    CHECK(c32_width(utf8_codepoint(" ")) == 1);
    CHECK(c32_width(utf8_codepoint("❓")) == 2);
    CHECK(c32_width(utf8_codepoint("🐎")) == 2);
    CHECK(c32_width(utf8_codepoint("🔥")) == 2);
}


TEST_CASE( "stripped_width", "[TermCtl]" )
{
    CHECK(TermCtl::stripped_width("test") == 4);
    CHECK(TermCtl::stripped_width("❓") == 2);
    TermCtl t(1, TermCtl::IsTty::Always);
    CHECK(TermCtl::stripped_width(t.format("{fg:green}test{t:normal}")) == 4);
    CHECK(TermCtl::stripped_width("\x1b[32mtest\x1b(B\x1b[m") == 4);
    CHECK(TermCtl::stripped_width("\n") == 1);  // newline is 1 column (special handling in EditLine)
}
