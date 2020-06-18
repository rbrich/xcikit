// test_serialization.cpp created on 2018-03-30, part of XCI toolkit

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include <xci/data/reflection.h>
#include <xci/data/serialization.h>
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
XCI_METAOBJECT_FOR_ENUM(Option, ThisOne, ThatOne, OtherOne);


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
};
XCI_METAOBJECT(Node, name, option, child, f);


TEST_CASE( "node tree", "[reflection]" )
{
    Node root{"root", Option::ThisOne, {
        Node{"child1", Option::ThatOne, {}, 1.1},
        Node{"child2", Option::OtherOne, {}, 2.2},
    }, 0.0};
    const char* node_text = "name: \"root\"\n"
                            "option: ThisOne\n"
                            "child:\n"
                            "    name: \"child1\"\n"
                            "    option: ThatOne\n"
                            "    f: 1.1\n"
                            "child:\n"
                            "    name: \"child2\"\n"
                            "    option: OtherOne\n"
                            "    f: 2.2\n"
                            "f: 0\n";

    SECTION( "textual serialization" ) {
        std::stringstream s("");
        TextualWriter wtext(s);
        wtext.write(root);
        CHECK(s.str() == node_text);
    }
/*
    SECTION( "binary serialization/deserialization" ) {
        std::stringstream s("");
        BinaryWriter wbin(s);
        wbin(root);

        //s.seekg(std::ios::beg);
        Node reconstructed_node;
        BinaryReader rbin(s);
        rbin.load(reconstructed_node);
        INFO(rbin.get_error_cstr());
        CHECK(!s.fail());

        root.check_equal(reconstructed_node);

        std::stringstream st("");
        TextualWriter wtext(st);
        wtext.write(reconstructed_node);
        CHECK(st.str() == node_text);
    }
    */
}


struct Record {
    int32_t id = 100;
    bool flag = false;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(id, flag);
    }
};


struct MasterRecord {
    Record rec1 { 1, false };
    Record rec2 { 2, true };

    template <class Archive>
    void serialize(Archive& ar) {
        ar(rec1, rec2);
    }
};


TEST_CASE( "BinaryWriter", "[serialization]" )
{
    std::stringstream buf("");

    // header
#if BYTE_ORDER == LITTLE_ENDIAN
    std::string expected = "\xCB\xDF\x30\x01";
#else
    std::string expected = "\xCB\xDF\x30\x02";
#endif

    SECTION( "empty archive" ) {
        expected.append(1, '\x00');  // SIZE=0
        {
            BinaryWriter writer(buf);
        }
        CHECK(buf.str() == expected);
    }

    SECTION( "POD values" ) {
        expected.append(1, '\x12');  // SIZE=18 (5+9+1+1+2)

        uint32_t x = 123;
        expected.append(1, '\x40');  // TYPE=4, KEY=0
        expected.append((char*)&x, 4);

        double f = 3.14;
        expected.append(1, '\x91');  // TYPE=9, KEY=1
        expected.append((char*)&f, 8);

        bool b = true;
        expected.append(1, '\x22');  // TYPE=2, KEY=2

        // nullptr
        int* n = nullptr;
        expected.append(1, '\x03');  // TYPE=0, KEY=3

        std::byte z {42};
        expected.append(1, '\x34');  // TYPE=3, KEY=4
        expected.append((char*)&z, 1);

        {
            BinaryWriter writer(buf);
            writer(x, f, b, n, z);
        }
        CHECK(buf.str() == expected);
    }

    SECTION( "Record" ) {
        Record rec;

        // content size
        expected.append(1, '\x08');  // SIZE=8 (6 + 2)

        // group start
        expected.append("\xE0\x06");  // TYPE=14, KEY=0; LEN=6

        // int32_t id
        expected.append(1, '\x60');  // TYPE=6, KEY=0
        expected.append((char*)&rec.id, 4);

        // bool flag
        expected.append(1, '\x11');  // TYPE=1, KEY=1

        {
            BinaryWriter writer(buf);
            writer(rec);
        }
        CHECK(buf.str() == expected);
    }

    SECTION( "MasterRecord + CRC-32" ) {
        MasterRecord rec;

        // add CRC32 to flags
        expected.back() |= 4;

        // content size
        expected.append(1, '\x18');  // SIZE=18 (16 + 2 + 6)

        // MasterRecord: group start
        expected.append("\xE0\x10");  // TYPE=14, KEY=0; LEN=16 (2*8)

        // rec1
        expected.append("\xE0\x06\x60");
        expected.append((char*)&rec.rec1.id, 4);
        expected.append("\x11");

        // rec2
        expected.append("\xE1\x06\x60");
        expected.append((char*)&rec.rec2.id, 4);
        expected.append("\x21");  // NOLINT

        // Control: metadata (expecting CRC32)
        expected.append("\xF0");

        {
            BinaryWriter writer(buf, true);
            writer(rec);
        }

        Crc32 crc;
        expected.append(1, '\x41');
        crc.feed(expected.data(), expected.size());
        expected.append((char*)&crc, sizeof(crc));

        CHECK(buf.str() == expected);
    }
}


TEST_CASE( "BinaryReader", "[serialization]" )
{
    std::stringstream buf("");

    // header
    #if BYTE_ORDER == LITTLE_ENDIAN
        std::string input = "\xCB\xDF\x30\x01";
    #else
        std::string input = "\xCB\xDF\x30\x02";
    #endif

    SECTION( "empty archive" ) {
        input.append(1, '\x00');  // SIZE=0
        buf.str(input);

        uint32_t x = 123;
        try {
            BinaryReader reader(buf);
            reader(x);
            reader.finish_and_check();
        } catch (const ArchiveError& e) {
            INFO(e.what());
            FAIL();
        }
        CHECK(x == 123);  // not changed
        CHECK(buf);
    }

    SECTION( "POD values" ) {
        // feed input
        input.append(1, '\x12');  // SIZE=18 (5+9+1+1+2)

        uint32_t in_x = 123;
        input.append(1, '\x40');  // TYPE=4, KEY=0
        input.append((char*)&in_x, 4);

        double in_f = 3.14;
        input.append(1, '\x91');  // TYPE=9, KEY=1
        input.append((char*)&in_f, 8);

        input.append(1, '\x22');  // TYPE=2 (true), KEY=2
        input.append(1, '\x03');  // TYPE=0 (null), KEY=3

        std::byte in_z {42};
        input.append(1, '\x34');  // TYPE=3, KEY=4
        input.append((char*)&in_z, 1);
        buf.str(input);

        // read values from input
        uint32_t x = 0;
        double f = 0.0;
        bool b = false;
        int *n = nullptr;
        std::byte z {0};
        try {
            BinaryReader reader(buf);
            reader(x, f, b, n, z);
            reader.finish_and_check();
        } catch (const ArchiveError& e) {
            INFO(e.what());
            FAIL();
        }
        CHECK(x == in_x);
        CHECK(f == in_f);
        CHECK(b);
        CHECK(z == in_z);
        CHECK(buf);
    }

    SECTION( "Record" ) {
        Record rec = {91, true};

        // feed input
        input.append("\x08\xE0\x06\x60", 4);  // SIZE=8, group 0 start, LEN=6, chunk Int32/0
        input.append((char*)&rec.id, 4);
        input.append(1, '\x21');  // flag = true
        buf.str(input);

        // read record from input
        rec = {0, false};
        try {
            BinaryReader reader(buf);
            reader(rec);
            reader.finish_and_check();
        } catch (const ArchiveError& e) {
            INFO(e.what());
            FAIL();
        }
        CHECK(rec.id == 91);
        CHECK(rec.flag);
        CHECK(buf);
    }

    SECTION( "MasterRecord + CRC-32" ) {
        int32_t id1 = 111;
        int32_t id2 = -222;

        // add CRC32 to flags
        input.back() |= 4;

        // content size
        input.append(1, '\x18');  // SIZE=18 (16 + 2 + 6)

        // MasterRecord: group start
        input.append("\xE0\x10");  // TYPE=14, KEY=0; LEN=16 (2*8)

        // rec1
        input.append("\xE0\x06\x60", 3);
        input.append((char*)&id1, 4);
        input.append("\x11");  // flag = false

        // rec2
        input.append("\xE1\x06\x60", 3);
        input.append((char*)&id2, 4);
        input.append("\x21");  // NOLINT; flag = true

        // Control: metadata, CRC32 head
        input.append("\xF0\x41", 2);

        Crc32 crc;
        crc.feed(input.data(), input.size());
        input.append((char*)&crc, sizeof(crc));

        buf.str(input);

        // read record from input
        MasterRecord rec;
        try {
            BinaryReader reader(buf);
            reader(rec);
            reader.finish_and_check();
        } catch (const ArchiveError& e) {
            INFO(e.what());
            FAIL();
        }
        CHECK(rec.rec1.id == id1);
        CHECK(!rec.rec1.flag);
        CHECK(rec.rec2.id == id2);
        CHECK(rec.rec2.flag);

        CHECK(buf);
    }
}
