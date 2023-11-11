// test_config.cpp created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <xci/config/ConfigParser.h>
#include <xci/config/Config.h>
#include <xci/core/string.h>

#include <sstream>

using namespace xci::config;
using namespace xci::core;


class TestConfigParser : public ConfigParser {
public:
    std::string operator()(const std::string& str) {
        if (!parse_string(str))
            dump << "<parse error>";
        auto res = dump.str();
        dump.str("");
        return res;
    }

protected:
    std::string indent() const { return std::string(m_indent * 2, ' '); }
    void name(const std::string& name) override { dump << indent() << name << ' '; }
    void bool_value(bool value) override { dump << std::boolalpha << value << '\n'; }
    void int_value(int64_t value) override { dump << value << '\n'; }
    void float_value(double value) override { dump << value << '\n'; }
    void string_value(std::string value) override { dump << '"' << escape_utf8(value) << '"' << '\n'; }
    void begin_group() override {
        ++m_indent;
        dump << '{' << '\n';
    }
    void end_group() override {
        --m_indent;
        dump << indent() << '}' << '\n';
    }

private:
    std::stringstream dump;
    int m_indent = 0;
};


TEST_CASE( "Config syntax", "[ConfigParser]" )
{
    TestConfigParser p;

    CHECK(p("") == "");
    CHECK(p("bool_item false") == "bool_item false\n");
    CHECK(p("int_item 123") == "int_item 123\n");
    CHECK(p("int_item 4.56") == "int_item 4.56\n");
    CHECK(p("string_item \"abc\"") == "string_item \"abc\"\n");
    CHECK(p("int_item 1\nbool_item true") == "int_item 1\nbool_item true\n");
    CHECK(p("int_item 1; bool_item true") == "int_item 1\nbool_item true\n");
    CHECK(p("group {}") == "group {\n}\n");
    CHECK(p("group { value 1 }") == "group {\n  value 1\n}\n");
    CHECK(p("group { value 1; value 2 }") == "group {\n  value 1\n  value 2\n}\n");
    CHECK(p("group { value 1; subgroup { foo 42; bar \"baz\" } }") ==
        "group {\n"
        "  value 1\n"
        "  subgroup {\n"
        "    foo 42\n"
        "    bar \"baz\"\n"
        "  }\n"
        "}\n");
}


TEST_CASE( "Config", "[Config]" )
{
    Config c;
    CHECK(c.parse_string("bool_item false; int_item 1; float_item 2.3; string_item \"abc\\n\";"
                         "group { value 2; subgroup { foo 42; bar \"baz\" } }"));
    CHECK(c.size() == 5);
    CHECK(c.front().name() == "bool_item");
    CHECK(c.front().as_bool() == false);
    CHECK(c.back().name() == "group");
    CHECK(c.back().as_group().size() == 2);
    CHECK(c.back().as_group().front().name() == "value");

    // Shortcuts
    CHECK(c["int_item"] == 1);
    CHECK(c["int_item"] != "1");
    c["int_item"] = "42x";
    CHECK(c["int_item"] == "42x");
    CHECK(c["group"]["value"] == 2);
    CHECK(c["group"]["subgroup"]["bar"] == "baz");
    CHECK(c["group"]["value"]["subvalue"] != false);  // this is destructive, the original int value is replaced by group
    CHECK(c["group"]["value"].is_group());
    CHECK(c["group"]["value"].as_group().size() == 1);  // "subvalue" item was created
    CHECK(c["group"]["value"]["subvalue"].is_null());
}
