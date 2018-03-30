// test_util.cpp created on 2018-03-30, part of XCI toolkit

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <xci/util/format.h>

using namespace xci::util;

TEST_CASE( "Format placeholders", "[format]" )
{
    CHECK(format("") == "");
    CHECK(format("hello there") == "hello there");
    CHECK(format("{unknown} placeholders {!!!}") == "{unknown} placeholders {!!!}");

    CHECK(format("number {} str {}", 123, "hello") == "number 123 str hello");

    CHECK(format("surplus placeholder {}{}", "left as is") == "surplus placeholder left as is{}");

    errno = EACCES;
    CHECK(format("error: {m}") == "error: Permission denied");
}
