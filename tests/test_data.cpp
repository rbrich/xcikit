// test_data.cpp created on 2018-03-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/data/Dumper.h>
#include <xci/data/Schema.h>
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
            "(0) root:\n"
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
    const char* schema_text =
            "(0) schema:\n"
            "    (0) struct:\n"
            "        (0) name: \"struct_0\"\n"
            "        (1) member:\n"
            "            (0) key: 0\n"
            "            (1) name: \"root\"\n"
            "            (2) type: \"struct_1\"\n"
            "    (0) struct:\n"
            "        (0) name: \"struct_1\"\n"
            "        (1) member:\n"
            "            (0) key: 0\n"
            "            (1) name: \"name\"\n"
            "            (2) type: \"string\"\n"
            "        (1) member:\n"
            "            (0) key: 1\n"
            "            (1) name: \"option\"\n"
            "            (2) type: \"enum\"\n"
            "        (1) member:\n"
            "            (0) key: 2\n"
            "            (1) name: \"child\"\n"
            "            (2) type: \"struct_1\"\n"
            "        (1) member:\n"
            "            (0) key: 3\n"
            "            (1) name: \"f\"\n"
            "            (2) type: \"float64\"\n";

    SECTION( "Dumper" ) {
        std::stringstream s("");
        Dumper dumper(s);
        XCI_ARCHIVE(dumper, root);
        CHECK(s.str() == node_text);
    }

    SECTION( "BinaryWriter / BinaryReader" ) {
        std::stringstream s("");
        {
            BinaryWriter binary_writer(s);
            XCI_ARCHIVE(binary_writer, root);
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
        Dumper dumper(st);
        dumper("root", reconstructed_node);
        CHECK(st.str() == node_text);
    }

    SECTION( "Schema" ) {
        Schema schema;
        schema("root", root);
        std::stringstream s("");
        Dumper dumper(s);
        XCI_ARCHIVE(dumper, schema);
        CHECK(s.str() == schema_text);
    }
}


TEST_CASE( "Schema schema", "[data]" ) {
    const char* schema_text =
            "(0) schema:\n"
            "    (0) struct:\n"
            "        (0) name: \"root_0\"\n"
            "        (1) member:\n"
            "            (0) key: 0\n"
            "            (1) name: \"schema\"\n"
            "            (2) type: \"schema_1\"\n"
            "    (0) struct:\n"
            "        (0) name: \"schema_1\"\n"
            "        (1) member:\n"
            "            (0) key: 0\n"
            "            (1) name: \"struct\"\n"
            "            (2) type: \"struct_2\"\n"
            "    (0) struct:\n"
            "        (0) name: \"struct_2\"\n"
            "        (1) member:\n"
            "            (0) key: 0\n"
            "            (1) name: \"name\"\n"
            "            (2) type: \"string\"\n"
            "        (1) member:\n"
            "            (0) key: 1\n"
            "            (1) name: \"member\"\n"
            "            (2) type: \"member_3\"\n"
            "    (0) struct:\n"
            "        (0) name: \"member_3\"\n"
            "        (1) member:\n"
            "            (0) key: 0\n"
            "            (1) name: \"key\"\n"
            "            (2) type: \"uint8\"\n"
            "        (1) member:\n"
            "            (0) key: 1\n"
            "            (1) name: \"name\"\n"
            "            (2) type: \"string\"\n"
            "        (1) member:\n"
            "            (0) key: 2\n"
            "            (1) name: \"type\"\n"
            "            (2) type: \"string\"\n";
    Schema schema;
    schema("schema", schema);
    std::stringstream s("");
    Dumper dumper(s);
    XCI_ARCHIVE(dumper, schema);
    CHECK(s.str() == schema_text);
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
