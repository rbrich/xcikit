// test_data.cpp created on 2018-03-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/data/Dumper.h>
#include <xci/data/BinaryWriter.h>
#include <xci/data/BinaryReader.h>
#include <xci/core/container/ChunkedStack.h>

#include <sstream>
#include <string>

using namespace xci::data;
using namespace xci::core;


enum class Option : uint8_t {
    Zero,
    One,
    Two,
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


TEST_CASE( "Node tree: dump/save/load", "[data]" )
{
    Node root{"root", Option::Zero, {
        Node{"child1", Option::One, {}, 1.1},
        Node{"child2", Option::Two, {}, 2.2},
    }, 0.0};
    const char* node_text =
            "(0):\n"
            "    (0) name: \"root\"\n"
            "    (1) option: Zero\n"
            "    (2) child:\n"
            "        (0) name: \"child1\"\n"
            "        (1) option: One\n"
            "        (3) f: 1.1\n"
            "    (2) child:\n"
            "        (0) name: \"child2\"\n"
            "        (1) option: Two\n"
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


struct PlainRecord {
    int id;
    std::string name;
    Option option;
    bool flag;
};


TEST_CASE( "Magic save/load", "[data]")
{
    // This record has no special decoration, it's just a struct.
    // Thanks to pfr + magic_enum, it can be serialized/deserialized directly.
    PlainRecord record { 7, "test", Option::Two, true};
    std::string data;

    // save
    {
        std::ostringstream s(data);
        {
            xci::data::BinaryWriter bw(s);
            bw(record);
        }
        data = s.str();
    }

    // load
    {
        PlainRecord loaded;

        std::istringstream s(data);
        {
            xci::data::BinaryReader br(s);
            br(loaded);
            br.finish_and_check();
        }

        CHECK(loaded.id == record.id);
        CHECK(loaded.name == record.name);
        CHECK(loaded.option == record.option);
        CHECK(loaded.flag == record.flag);
    }

    // dump
    {
        std::ostringstream s("");
        xci::data::Dumper dumper(s);
        dumper(record);

        // names are not available, but sequential keys are
        CHECK(s.str() == "(0):\n"               // PlainRecord:
                         "    (0): 7\n"         //   id
                         "    (1): \"test\"\n"  //   name
                         "    (2): Two\n"       //   option
                         "    (3): true\n");    //   flag
    }
}


TEST_CASE( "Save/load ChunkedStack", "[data]")
{
    ChunkedStack<std::string> c(4);
    std::string data;

    c.push("first");
    c.push("second");
    c.push("third");
    c.push("fourth");

    // save
    {
        std::ostringstream s(data);
        {
            xci::data::BinaryWriter bw(s);
            bw(c);
        }
        data = s.str();
    }

    // load
    {
        ChunkedStack<std::string> c2;

        std::istringstream s(data);
        {
            xci::data::BinaryReader br(s);
            br(c2);
            br.finish_and_check();
        }

        CHECK(c2 == c);
    }
}
