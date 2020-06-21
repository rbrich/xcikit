// test_data.cpp created on 2018-03-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include <xci/data/Dumper.h>
#include <xci/data/BinaryWriter.h>
#include <xci/data/BinaryReader.h>

#include <sstream>
#include <string>

using namespace xci::data;


enum class Option {
    ThisOne,
    ThatOne,
    OtherOne,
};


struct Node
{
    std::string name;
    Option option = Option(-1);
    std::vector<Node> child;
    double f = 0.0;

    void check_equal(const Node& rhs) const {
        CHECK(name == rhs.name);
        CHECK(option == rhs.option);
        REQUIRE(child.size() == rhs.child.size());
        for (size_t i = 0; i < child.size(); ++i) {
            child[i].check_equal(rhs.child[i]);
        }
    }

    template <class Archive>
    void serialize(Archive& ar) {
        XCI_ARCHIVE(ar, name, option, child, f);
    }
};


TEST_CASE( "node tree", "[data]" )
{
    Node root{"root", Option::ThisOne, {
        Node{"child1", Option::ThatOne, {}, 1.1},
        Node{"child2", Option::OtherOne, {}, 2.2},
    }, 0.0};
    const char* node_text =
            "(0):\n"
            "    (0) name: \"root\"\n"
            "    (1) option: ThisOne\n"
            "    (2) child:\n"
            "        (0) name: \"child1\"\n"
            "        (1) option: ThatOne\n"
            "        (3) f: 1.1\n"
            "    (2) child:\n"
            "        (0) name: \"child2\"\n"
            "        (1) option: OtherOne\n"
            "        (3) f: 2.2\n"
            "    (3) f: 0\n";

    SECTION( "Dumper" ) {
        std::stringstream s("");
        Dumper wtext(s);
        wtext(root);
        CHECK(s.str() == node_text);
    }

    SECTION( "BinaryWriter / BinaryReader" ) {
        std::stringstream s("");
        {
            BinaryWriter wbin(s);
            wbin(root);
        }

        Node reconstructed_node;
        try {
            s.seekg(std::ios::beg);
            BinaryReader rbin(s);
            rbin(reconstructed_node);
            rbin.finish_and_check();
        } catch (const ArchiveError& e) {
            INFO(e.what());
            FAIL();
        }
        CHECK(s);

        root.check_equal(reconstructed_node);

        std::stringstream st("");
        Dumper wtext(st);
        wtext(reconstructed_node);
        CHECK(st.str() == node_text);
    }
}
