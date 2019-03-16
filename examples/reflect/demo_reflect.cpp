// demo_reflect.cpp created on 2019-03-13, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <xci/data/reflection.h>
#include <xci/data/serialization.h>
#include <xci/data/BinaryWriter.h>
#include <xci/data/BinaryReader.h>
#include <xci/data/Property.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using xci::data::Property;


struct DialogReply
{
    std::string text;
    unsigned int next;  // -> DialogState::id
};


struct DialogState
{
    unsigned int id = 0;
    std::string text;
    std::vector<DialogReply> re;
};


struct Dialog
{
    std::string title;
    DialogState state;
};


XCI_METAOBJECT(DialogReply, text, next);
XCI_METAOBJECT(DialogState, id, text, re);
XCI_METAOBJECT(Dialog, title, state);


enum class Option {
    ThisOne,
    ThatOne,
    OtherOne,
};
XCI_METAOBJECT_FOR_ENUM(Option, ThisOne, ThatOne, OtherOne);


struct Node
{
    Property<std::string> name;
    Option option;
    std::vector<Node> child;
};
XCI_METAOBJECT(Node, name, option, child);


int main()
{
    Dialog dialog;
    dialog.title = "Hello";
    dialog.state.id = 0;
    dialog.state.text = "Nice day to you, sir!";

    dialog.state.re.push_back({"Please continue...", 1});
    dialog.state.re.push_back({"Please stop!", 2});

    xci::data::TextualWriter wcout(std::cout);
    std::cout << "BEGIN" << std::endl;
    wcout.write(dialog);
    std::cout << "END" << std::endl;

    std::ofstream f("/tmp/xci-sertest.bin");
    xci::data::BinaryWriter bw(f);
    bw.dump(dialog);
    f.close();

    std::ifstream fi("/tmp/xci-sertest.bin");
    xci::data::BinaryReader br(fi);
    Dialog loaded_dialog;
    br.load(loaded_dialog);

    if (fi.fail()) {
        std::cerr << "Load failed at " << br.get_last_pos() << ": "
                  << br.get_error_cstr() << std::endl;
    }

    wcout.write(loaded_dialog);

    // ---------------

    std::cout << "=== Node ===\n";
    Node root{"root", Option::ThisOne, {
        Node{"child1", Option::ThatOne, {}},
        Node{"child2", Option::OtherOne, {}},
        }};

    wcout.write(root);

    std::cout << "Option::ThatOne = "
        << (int) xci::data::metaobject::get_enum_constant_value<Option>("thatone") << std::endl;

    return 0;
}
