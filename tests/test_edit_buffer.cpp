// test_edit_buffer.cpp created on 2021-02-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/core/EditBuffer.h>

using namespace xci::core;

#define UTF8(l)  (const char*)u8 ## l


TEST_CASE( "ascii", "[EditBuffer]" )
{
    EditBuffer eb("just ascii");
    CHECK(eb.cursor() == eb.content_view().size());
    CHECK(eb.content_upto_cursor() == "just ascii");
    CHECK(eb.content_from_cursor() == "");

    CHECK(eb.delete_left());
    CHECK(eb.content() == "just asci");

    CHECK(eb.move_left());
    CHECK(eb.move_left());
    CHECK(eb.delete_right());
    CHECK(eb.content() == "just asi");
    CHECK(eb.content_upto_cursor() == "just as");
    CHECK(eb.content_from_cursor() == "i");

    CHECK(eb.move_to_home());
    CHECK_FALSE(eb.move_to_home());
    CHECK_FALSE(eb.move_left());
    CHECK(eb.move_right());
    CHECK(eb.content_from_cursor() == "ust asi");
    CHECK(eb.delete_right());
    CHECK(eb.content() == "jst asi");
    CHECK(eb.cursor() == 1);
}


TEST_CASE( "utf-8", "[EditBuffer]" )
{
    EditBuffer eb(UTF8("河北梆子"));
    CHECK(eb.cursor() == eb.content_view().size());
    CHECK(eb.content_upto_cursor() == UTF8("河北梆子"));
    CHECK(eb.content_from_cursor() == "");

    CHECK(eb.delete_left());
    CHECK(eb.content() == UTF8("河北梆"));

    CHECK(eb.move_left());
    CHECK(eb.move_left());
    CHECK(eb.delete_right());
    CHECK(eb.content() == UTF8("河梆"));
    CHECK(eb.content_upto_cursor() == UTF8("河"));
    CHECK(eb.content_from_cursor() == UTF8("梆"));

    eb.insert(UTF8("①"));
    CHECK(eb.content() == UTF8("河①梆"));

    CHECK(eb.move_to_home());
    CHECK_FALSE(eb.move_to_home());
    CHECK_FALSE(eb.move_left());
    CHECK(eb.move_right());
    CHECK(eb.content_from_cursor() == UTF8("①梆"));
    CHECK(eb.delete_left());
    CHECK_FALSE(eb.delete_left());
    CHECK(eb.content() == UTF8("①梆"));
    CHECK(eb.cursor() == 0);
    CHECK(eb.move_right());
    CHECK(eb.cursor() == strlen(UTF8("①")));
}
