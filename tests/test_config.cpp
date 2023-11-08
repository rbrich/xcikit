// test_config.cpp created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <xci/config/ConfigParser.h>
#include <xci/config/Config.h>

#include <sstream>

using namespace xci::config;


class TestConfigParser : public ConfigParser {
public:
    std::string operator()(const std::string& str) {
        if (!parse_string(str))
            dump << "<parse error>";
        auto res = dump.str();
        dump.clear();
        return res;
    }

protected:
    void name(const std::string& name) override { dump << name << ' '; }
    void bool_value(bool value) override { dump << std::boolalpha << value; }

private:
    std::stringstream dump;
};


TEST_CASE( "Config syntax", "[ConfigParser]" )
{
    TestConfigParser p;

    CHECK(p("") == "");
    CHECK(p("bool_item false") == "bool_item false");
}
