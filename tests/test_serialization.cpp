// test_serialization.cpp created on 2018-03-30, part of XCI toolkit

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <xci/core/serialization.h>

#include <fstream>

using namespace xci::core;


struct MyData : public sdata::Serializable {
    sdata::String m_str {this, 1, "str_data", "value"};
    sdata::Int64 m_int {this, 1, "number", 123};
    sdata::Float64 m_float {this, 1, "float", 3.14};
};


TEST_CASE( "rstrip", "[string]" )
{
    std::ofstream f("/tmp/xci-serialization.txt");
    sdata::TextualWriter w(f);

    MyData data;
    data.serialize(w);
}
