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

#include <xci/data/Dumper.h>
#include <xci/data/BinaryWriter.h>
#include <xci/data/BinaryReader.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>


struct DialogReply
{
    std::string text;
    unsigned int next;  // -> DialogState::id

    template <class Archive>
    void serialize(Archive& ar) {
        XCI_ARCHIVE(ar, text, next);
    }
};


struct DialogState
{
    unsigned int id = 0;
    std::string text;
    std::vector<DialogReply> re;

    template <class Archive>
    void serialize(Archive& ar) {
        XCI_ARCHIVE(ar, id, text, re);
    }
};


struct Dialog
{
    std::string title;
    DialogState state;

    template <class Archive>
    void serialize(Archive& ar) {
        XCI_ARCHIVE(ar, title, state);
    }
};


enum class Option {
    Zero,
    One,
    Two,
};


struct Node
{
    std::string name;
    Option option;
    std::vector<Node> child;

    template <class Archive>
    void serialize(Archive& ar) {
        XCI_ARCHIVE(ar, name, option, child);
    }
};


int main()
{
    Dialog dialog;
    dialog.title = "Use the stabilizers!";
    dialog.state.id = 0;
    dialog.state.text = "";

    dialog.state.re.push_back({"It doesn't have stabilizers!", 1});
    dialog.state.re.push_back({"What is a stabilizer?", 2});

    xci::data::Dumper wcout(std::cout);
    std::cout << "=== Dialog ===\n";
    wcout(dialog);

    std::cout << "=== Dialog (after binary write / read) ===\n";
    std::stringstream ss("");
    {
        xci::data::BinaryWriter bw(ss);
        bw(dialog);
    }
    ss.seekg(std::ios::beg);

    Dialog loaded_dialog;
    {
        xci::data::BinaryReader br(ss);
        br(loaded_dialog);
        br.finish_and_check();
    }
    wcout(loaded_dialog);

    // ---------------

    std::cout << "=== Node ===\n";
    Node root{"root", Option::Zero, {
        Node{"child1", Option::One, {}},
        Node{"child2", Option::Two, {}},
        }};

    wcout(root);

    return 0;
}
