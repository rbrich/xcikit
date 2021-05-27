// test_coding.cpp created on 2020-06-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/data/coding/leb128.h>

#include <bit>

using namespace xci::data;
using namespace xci;


TEST_CASE( "LEB128", "[coding]" )
{
    std::byte buffer[10] {};

    SECTION( "encode & decode 7bit value" ) {
        uint32_t v_in = GENERATE(0lu, 1lu, 42lu, 0x0Flu, 0x7Flu);
        auto* iter = buffer;
        leb128_encode(iter, v_in);
        CHECK(iter - buffer == 1);  // encoded in 1B
        CHECK(unsigned(buffer[0]) < 0x80);    // high-order bit not set
        iter = buffer;
        auto v_out = leb128_decode<uint32_t>(iter);
        CHECK(v_in == v_out);
        CHECK(iter - buffer == 1);  // decoded 1B
    }

    SECTION( "encode & decode big value" ) {
        uint64_t v_in = GENERATE(0x80llu, 0xFFllu, 0xAAAllu, 0xABCDEF12llu, 0xFFFFFFFFllu,
                               std::numeric_limits<uint64_t>::max());
        unsigned v_bits = 64 - std::countl_zero(v_in);
        CAPTURE(v_in, v_bits);
        CHECK(v_bits >= 8);
        auto* iter = buffer;
        leb128_encode(iter, v_in);
        auto b_in = iter - buffer;
        CHECK(b_in == (v_bits + 6) / 7);
        CHECK(unsigned(buffer[0]) >= 0x80);    // high-order bit is set
        iter = buffer;
        auto v_out = leb128_decode<uint64_t>(iter);
        auto b_out = iter - buffer;
        CHECK(v_in == v_out);
        CHECK(b_out == b_in);
    }

    SECTION( "encode & decode with skip_bits" ) {
        uint64_t v_in = GENERATE(00llu, 1llu, 0x0Fllu, 0x7Fllu, 0x80llu, 0xFFllu, 0xAAAllu,
                0xABCDEF12llu, 0xFFFFFFFFllu, std::numeric_limits<uint64_t>::max());
        unsigned skip_bits = GENERATE(0, 1, 2, 4, 6);
        unsigned v_bits = 64 - std::countl_zero(v_in);
        std::byte check = skip_bits == 0 ? std::byte(0) :
                          skip_bits == 1 ? std::byte(GENERATE(0, 0x80)) :
                          /* else */       std::byte(GENERATE(0, 0x55, 0xFF) << (8-skip_bits));
        CAPTURE(v_in, skip_bits, check);
        buffer[0] = std::byte(check);
        auto* iter = buffer;
        leb128_encode(iter, v_in, skip_bits);
        auto b_in = iter - buffer;
        CHECK(b_in == std::max(1u, (v_bits + skip_bits + 6) / 7));
        CHECK((buffer[0] & std::byte(0xFF << (8-skip_bits))) == check);  // skipped bits untouched
        iter = buffer;
        auto v_out = leb128_decode<uint64_t>(iter, skip_bits);
        auto b_out = iter - buffer;
        CHECK(v_in == v_out);
        CHECK(b_out == b_in);
    }

    SECTION( "overflow" ) {
        for (int i = 0; i < 9; ++i)
            buffer[i] = std::byte(0xF0 + i);
        buffer[9] = std::byte(0x7F);
        auto* iter = buffer;
        CHECK(leb128_decode<uint32_t>(iter) == std::numeric_limits<uint32_t>::max());
        iter = buffer;
        CHECK(leb128_decode<uint64_t>(iter) == std::numeric_limits<uint64_t>::max());
        iter = buffer;
        CHECK(leb128_decode<uint32_t>(iter, 0) == std::numeric_limits<uint32_t>::max());
        iter = buffer;
        CHECK(leb128_decode<uint64_t>(iter, 0) == std::numeric_limits<uint64_t>::max());
    }
}
