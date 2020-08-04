// test_argparser.cpp created on 2020-02-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/core/ArgParser.h>
#include <array>

using namespace xci::core;
using namespace xci::core::argparser;


TEST_CASE( "Bool value conversion", "[ArgParser][value_from_cstr]" )
{
    bool v = false;
    SECTION("bool supported input") {
        CHECK(value_from_cstr("true", v)); CHECK(v);
        CHECK(value_from_cstr("false", v)); CHECK(!v);
        CHECK(value_from_cstr("yes", v)); CHECK(v);
        CHECK(value_from_cstr("no", v)); CHECK(!v);
        CHECK(value_from_cstr("1", v)); CHECK(v);
        CHECK(value_from_cstr("0", v)); CHECK(!v);
        CHECK(value_from_cstr("y", v)); CHECK(v);
        CHECK(value_from_cstr("n", v)); CHECK(!v);
        CHECK(value_from_cstr("T", v)); CHECK(v);
        CHECK(value_from_cstr("F", v)); CHECK(!v);
    }
    SECTION("bool unsupported input") {
        CHECK(!value_from_cstr("abc", v));
        CHECK(!value_from_cstr("yesyes", v));
        CHECK(!value_from_cstr("nn", v));
        CHECK(!value_from_cstr("X", v));
        CHECK(!value_from_cstr("ON", v));
        CHECK(!value_from_cstr("off", v));
    }
}


TEST_CASE( "Int value conversion", "[ArgParser][value_from_cstr]" )
{
    int v = 0;
    uint8_t u8 = 0;
    SECTION("int supported input") {
        CHECK(value_from_cstr("1", v)); CHECK(v == 1);
        CHECK(value_from_cstr("0", v)); CHECK(v == 0);
        CHECK(value_from_cstr("-1", v)); CHECK(v == -1);
        CHECK(value_from_cstr("123456", v)); CHECK(v == 123456);
        CHECK(value_from_cstr("0xff", v)); CHECK(v == 0xff);
    }
    SECTION("int unsupported input") {
        CHECK(!value_from_cstr("abc", v));
        CHECK(!value_from_cstr("1e3", v));
        CHECK(!value_from_cstr("11111111111111111111111111111111111", v));
    }
    SECTION("uint8 supported input") {
        CHECK(value_from_cstr("0", u8)); CHECK(u8 == 0);
        CHECK(value_from_cstr("255", u8)); CHECK(u8 == 255);
        CHECK(value_from_cstr("077", u8)); CHECK(u8 == 077);
        CHECK(value_from_cstr("0xff", u8)); CHECK(u8 == 0xff);
    }
    SECTION("uint8 out of range") {
        CHECK(!value_from_cstr("-1", u8));
        CHECK(!value_from_cstr("256", u8));
    }
}


TEST_CASE( "Other value conversion", "[ArgParser][value_from_cstr]" )
{
    SECTION("const char* -> original address returned unchanged") {
        const char* v;
        const char* sample = "test";
        CHECK(value_from_cstr(sample, v)); CHECK(v == sample);
    }
}


TEST_CASE( "Option description parsing", "[ArgParser][Option]" )
{
    bool flag;
    SECTION("basic") {
        Option o("-h, --help", "Show help", show_help);
        CHECK(o.has_short('h'));
        CHECK(o.has_long("help"));
        CHECK(!o.has_args());
        CHECK(!o.is_positional());
        CHECK(o.is_show_help());
        CHECK(!o.can_receive_arg());
    }
    SECTION("1 short") {
        Option o("-h", "1 short", flag);
        CHECK(o.has_short('h'));
        CHECK(!o.has_long("h"));
        CHECK(!o.has_args());
        CHECK(!o.is_positional());
    }
    SECTION("2 shorts") {
        Option o("-a -b", "2 shorts", flag);
        CHECK(o.has_short('a'));
        CHECK(o.has_short('b'));
        CHECK(!o.has_short('c'));
        CHECK(!o.has_args());
        CHECK(!o.is_positional());
    }
    SECTION("1 long") {
        Option o("--help", "1 long", flag);
        CHECK(o.has_long("help"));
        CHECK(!o.has_args());
        CHECK(!o.is_positional());
    }
    SECTION("2 longs") {
        Option o("--a --b", "2 longs", flag);
        CHECK(o.has_long("a"));
        CHECK(o.has_long("b"));
        CHECK(!o.has_args());
        CHECK(!o.is_positional());
    }
    SECTION("positional") {
        Option o("file", "positional", flag);
        CHECK(!o.is_short());
        CHECK(!o.is_long());
        CHECK(o.is_positional());
        CHECK(o.required_args() == 1);
    }
    SECTION("all positional") {
        Option o("input...", "all positional", flag);
        CHECK(!o.is_short());
        CHECK(!o.is_long());
        CHECK(o.is_positional());
        CHECK(o.required_args() == 1);
        CHECK(o.can_receive_all_args());
    }
    SECTION("remainder") {
        Option o("-- ...", "remaining arguments", flag);
        CHECK(!o.is_short());
        CHECK(!o.is_long());
        CHECK(!o.is_positional());
        CHECK(o.is_remainder());
        CHECK(o.can_receive_all_args());
    }
    SECTION("short with 1 arg") {
        Option o("-h ARG1", "", flag);
        CHECK(o.has_short('h'));
        CHECK(!o.is_long());
        CHECK(!o.is_positional());
        CHECK(!o.is_remainder());
        CHECK(o.has_args());
        CHECK(o.required_args() == 1);
    }
    SECTION("short with 2 args") {
        Option o("-h ARG1 ARG2", "", flag);
        CHECK(o.has_short('h'));
        CHECK(!o.is_long());
        CHECK(!o.is_positional());
        CHECK(o.has_args());
        CHECK(o.required_args() == 2);
    }
    SECTION("long with 1 arg") {
        Option o("--test ARG1", "", flag);
        CHECK(!o.is_short());
        CHECK(o.has_long("test"));
        CHECK(!o.is_positional());
        CHECK(o.has_args());
        CHECK(o.required_args() == 1);
    }
    SECTION("long with 2 args") {
        Option o("--test ARG1 ARG2", "", flag);
        CHECK(!o.is_short());
        CHECK(o.has_long("test"));
        CHECK(!o.is_positional());
        CHECK(o.has_args());
        CHECK(o.required_args() == 2);
    }
    SECTION("short and long with 2 args") {
        Option o("-t, --test ARG1 ARG2", "", flag);
        CHECK(o.has_short('t'));
        CHECK(o.has_long("test"));
        CHECK(!o.is_positional());
        CHECK(o.has_args());
        CHECK(o.required_args() == 2);
    }
}


TEST_CASE( "Invalid option descriptions", "[ArgParser][Option]" )
{
    CHECK_THROWS_AS(Option("---help", "Too many dashes", show_help), BadOptionDescription);
    CHECK_THROWS_AS(Option("-help", "Too long short option", show_help), BadOptionDescription);
    CHECK_THROWS_AS(Option("-", "Missing short name", show_help), BadOptionDescription);
    CHECK_THROWS_AS(Option("file.", "Not enough dots", show_help), BadOptionDescription);
    CHECK_THROWS_AS(Option("file..", "Not enough dots", show_help), BadOptionDescription);
    CHECK_THROWS_AS(Option("file....", "To many dots", show_help), BadOptionDescription);
    CHECK_THROWS_AS(Option("FILE -f", "Swapped nonsense", show_help), BadOptionDescription);
}



TEST_CASE( "Validation of the set of options", "[ArgParser][validate]" )
{
    int x;
    ArgParser ap {
        Option("--dummy", "", x),
        Option("-t, --test VALUE", "", x).env("TEST"),
        Option("-v, --verbose, -w, --whatever", "", x).env("VERBOSE"),
        Option("positional...", "", x).env("POSITIONAL"),
        Option("-- ...", "", x),
    };

    SECTION("repeat short name") {
        ap.add_option({"-t", "", x});
        CHECK_THROWS_AS(ap.validate(), BadOptionDescription);
    }

    SECTION("repeat short name (alias)") {
        ap.add_option({"-w", "", x});
        CHECK_THROWS_AS(ap.validate(), BadOptionDescription);
    }

    SECTION("repeat long name") {
        ap.add_option({"--test", "", x});
        CHECK_THROWS_AS(ap.validate(), BadOptionDescription);
    }

    SECTION("repeat long name (alias)") {
        ap.add_option({"--whatever", "", x});
        CHECK_THROWS_AS(ap.validate(), BadOptionDescription);
    }

    SECTION("repeat env (first)") {
        ap.add_option(std::move(Option{"-x", "", x}.env("TEST")));
        CHECK_THROWS_AS(ap.validate(), BadOptionDescription);
    }

    SECTION("repeat env (middle)") {
        ap.add_option(std::move(Option{"-x", "", x}.env("VERBOSE")));
        CHECK_THROWS_AS(ap.validate(), BadOptionDescription);
    }

    SECTION("repeat env (last)") {
        ap.add_option(std::move(Option{"-x", "", x}.env("POSITIONAL")));
        CHECK_THROWS_AS(ap.validate(), BadOptionDescription);
    }
}


#define ARGV(...)   std::array{__VA_ARGS__, (const char*)nullptr}.data()


TEST_CASE( "Parse args", "[ArgParser][parse_arg]" )
{
    bool verbose = false;
    bool warn = false;
    int optimize = 0;
    std::vector<std::string_view> files;

    ArgParser ap {
            Option("-v, --verbose", "Enable verbosity", verbose),
            Option("-w, --warn", "Warn me", warn),
            Option("-O, --optimize LEVEL", "Optimization level", optimize),
    };
    
    SECTION("bad input") {
        CHECK_THROWS_AS(ap.parse_arg(ARGV("-x")), BadArgument);
        CHECK_THROWS_AS(ap.parse_arg(ARGV("---v")), BadArgument);
        CHECK_THROWS_AS(ap.parse_arg(ARGV("--v")), BadArgument);
        CHECK_THROWS_AS(ap.parse_arg(ARGV("-verbose")), BadArgument);
        CHECK_THROWS_AS(ap.parse_arg(ARGV("-vx")), BadArgument);
        CHECK_THROWS_AS(ap.parse_arg(ARGV("file")), BadArgument);
    }

    SECTION("good input") {
        ap.add_option(Option("FILE...", "Input files", files));
        ap.parse_args(ARGV("-vwO3", "file1", "file2"));
        CHECK(verbose);
        CHECK(warn);
        CHECK(optimize == 3);
        CHECK(files == std::vector<std::string_view>{"file1", "file2"});

        INFO("same option given again");
        CHECK_THROWS_AS(ap.parse_arg(ARGV("--optimize")), BadArgument);
    }

    SECTION("single hyphen is parsed as positional argument") {
        ap.add_option(Option("FILE...", "Input files", files));
        ap.parse_args(ARGV("-vwO3", "-", "file2"));
        CHECK(files == std::vector<std::string_view>{"-", "file2"});
    }
}


TEST_CASE( "Option argument", "[ArgParser][parse_arg]" )
{
    char t;
    int v;
    ArgParser ap {
        Option("-t VALUE", "Test choices: a,b", [&t](const char* arg){
            t = arg[0];
            return (t == 'a' || t == 'b') && arg[1] == 0;
        }),
        Option("-v NUM", "Signed integer", v),
    };

    SECTION("choice: a") {
        ap.parse_arg(ARGV("-ta"));
        CHECK(t == 'a');
    }

    SECTION("choice: b") {
        ap.parse_args(ARGV("-t", "b"));
        CHECK(t == 'b');
    }

    SECTION("wrong choice: c") {
        CHECK_THROWS_AS(ap.parse_arg(ARGV("-tc")), BadArgument);
    }

    SECTION("int: 1") {
        ap.parse_args(ARGV("-v", "1"));
        CHECK(v == 1);
    }

    SECTION("int: -1 (ignore dash when parsing option argument)") {
        ap.parse_args(ARGV("-v", "-1"));
        CHECK(v == -1);
    }

    SECTION("int: missing value") {
        CHECK_THROWS_AS(ap.parse_args(ARGV("-v")), BadArgument);
    }
}


TEST_CASE( "Gather rest of the args", "[ArgParser]" )
{
    bool verbose = false;
    std::vector<std::string_view> rest;
    ArgParser ap {
        Option("-v, --verbose", "Enable verbosity", verbose),
        Option("-- ...", "Passthrough args", rest),
    };

    SECTION("passthrough all unconsumed positional args") {
        const char* args[] = {"aa", "bb", nullptr};
        ap.parse_args(args);
        CHECK(rest.size() == 2);
        CHECK(rest[0] == "aa");
        CHECK(rest[1] == "bb");
    }

    SECTION("passthrough the rest") {
        const char* args[] = {"-v", "aa", "bb", nullptr};
        ap.parse_args(args);
        CHECK(rest.size() == 2);
        CHECK(rest[0] == "aa");
        CHECK(rest[1] == "bb");
    }

    SECTION("passthrough args behave as normal positional args, flags are allowed in between") {
        const char* args[] = {"aa", "-v", "bb", nullptr};
        ap.parse_args(args);
        CHECK(rest.size() == 2);
        CHECK(rest[0] == "aa");
        CHECK(rest[1] == "bb");
    }

    SECTION("unrecognized flags are not passed through") {
        const char* args[] = {"-x", "-v", "bb", nullptr};
        CHECK_THROWS_AS(ap.parse_args(args), BadArgument);
    }

    SECTION("explicit passthrough (all)") {
        const char* args[] = {"--", "-v", "--verbose", "pos", nullptr};
        CHECK(ap.parse_args(args) == ArgParser::Stop);
        CHECK(rest.size() == 3);
        CHECK(rest[0] == "-v");
        CHECK(rest[1] == "--verbose");
        CHECK(rest[2] == "pos");
    }

    SECTION("explicit passthrough (with unconsumed positional)") {
        const char* args[] = {"f1", "f2", "--", "aa", "bb", nullptr};
        CHECK(ap.parse_args(args) == ArgParser::Stop);
        CHECK(rest.size() == 4);
        CHECK(rest[0] == "f1");
        CHECK(rest[1] == "f2");
        CHECK(rest[2] == "aa");
        CHECK(rest[3] == "bb");
    }

    std::vector<std::string_view> files;
    ap = ArgParser {
            Option("FILE...", "Input files", files),
            Option("-- REST ...", "Passthrough args", rest),
    };

    SECTION("explicit passthrough (with consumed positional)") {
        const char* args[] = {"f1", "f2", "--", "aa", "bb", nullptr};
        CHECK(ap.parse_args(args) == ArgParser::Stop);
        CHECK(files.size() == 2);
        CHECK(files[0] == "f1");
        CHECK(files[1] == "f2");
        CHECK(rest.size() == 2);
        CHECK(rest[0] == "aa");
        CHECK(rest[1] == "bb");
    }
}
