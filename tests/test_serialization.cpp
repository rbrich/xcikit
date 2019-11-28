// test_serialization.cpp created on 2018-03-30, part of XCI toolkit

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <xci/data/reflection.h>
#include <xci/data/serialization.h>
#include <xci/data/BinaryWriter.h>
#include <xci/data/BinaryReader.h>

#include <sstream>

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

    SECTION( "binary serialization/deserialization" ) {
        std::stringstream s("");
        BinaryWriter wbin(s);
        wbin.dump(root);

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
}
