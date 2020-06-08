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
        expected.append(1, '\xFF');
        {
            BinaryWriter writer(buf);
        }
        CHECK(buf.str() == expected);
    }

    SECTION( "POD values" ) {
        uint32_t x = 123;
        expected.append(1, '\x40');  // TYPE=4, KEY=0
        expected.append((char*)&x, 4);

        double f = 3.14;
        expected.append(1, '\x91');  // TYPE=9, KEY=1
        expected.append((char*)&f, 8);

        bool b = true;
        expected.append(1, '\x22');  // TYPE=2, KEY=2

        // nullptr
        expected.append(1, '\x03');  // TYPE=0, KEY=3

        std::byte z {42};
        expected.append(1, '\x34');  // TYPE=3, KEY=4
        expected.append((char*)&z, 1);

        expected.append(1, '\xFF');  // Terminator

        {
            BinaryWriter writer(buf);
            writer(x, f, b, nullptr, z);
        }
        CHECK(buf.str() == expected);
    }

    SECTION( "Record" ) {
        Record rec;

        // group start
        expected.append(1, '\xE0');  // TYPE=14, KEY=0
        uint32_t len = 6;
        expected.append((char*)&len, 4);

        // int32_t id
        expected.append(1, '\x60');  // TYPE=6, KEY=0
        expected.append((char*)&rec.id, 4);

        // bool flag
        expected.append(1, '\x11');  // TYPE=1, KEY=1

        // group end
        expected.append(1, '\xF0');  // TYPE=15, KEY=0

        expected.append(1, '\xFF');  // Terminator

        {
            BinaryWriter writer(buf);
            writer(rec);
        }
        CHECK(buf.str() == expected);
    }

    SECTION( "MasterRecord" ) {
        MasterRecord rec;

        // MasterRecord: group start
        expected.append(1, '\xE0');  // TYPE=14, KEY=0
        uint32_t len = 2 * 12;
        expected.append((char*)&len, 4);

        // rec1
        len = 6;  // + 4 size, + 2 begin/end
        expected.append(1, '\xE0');
        expected.append((char*)&len, 4);
        expected.append(1, '\x60');
        expected.append((char*)&rec.rec1.id, 4);
        expected.append("\x11\xF0");

        // rec2
        len = 6;
        expected.append(1, '\xE1');
        expected.append((char*)&len, 4);
        expected.append(1, '\x60');
        expected.append((char*)&rec.rec2.id, 4);
        expected.append("\x21\xF1");

        // MasterRecord: group end, file end
        expected.append(1, '\xF0');  // TYPE=15, KEY=0
        expected.append(1, '\xFF');  // Terminator


        BinaryWriter writer(buf);
        writer(rec);
        writer.terminate_content();

        Crc32 crc;
        crc(buf.str());
        writer.write_crc32(crc.as_uint32());

        crc.reset();
        crc.feed(expected.data(), expected.size());
        expected.append(1, '\x41');
        expected.append((char*)&crc, sizeof(crc));

        CHECK(buf.str() == expected);
    }
}


TEST_CASE( "BinaryReader", "[serialization]" )
{
    std::stringstream buf("");

    auto add_crc = [&buf] {
        Crc32 crc;
        crc.feed(buf.str().data(), buf.str().size());
        buf.seekp(0, std::ios::seekdir::end);
        buf.write((char*)&crc, sizeof(crc));
    };

    SECTION( "empty archive" ) {
        buf.str("\xCB\xDF\x30\x01\xFF");
        add_crc();

        uint32_t x = 123;
        try {
            BinaryReader reader(buf);
            reader(x);
            CHECK(x == 123);  // not changed
            reader.finish_and_check_crc();
        } catch (const ArchiveError& e) {
            INFO(e.what());
            FAIL();
        }
        CHECK(buf);
    }

    SECTION( "POD values (in order)" ) {

    }

    SECTION( "POD values (out of order)" ) {

    }

    SECTION( "Record" ) {

    }

    SECTION( "MasterRecord" ) {

    }
}
