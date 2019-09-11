// Module.cpp created on 2019-06-12, part of XCI toolkit
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

#include "Module.h"
#include "Function.h"
#include "Error.h"

namespace xci::script {

using namespace std;


Index Module::get_imported_module_index(Module* module) const
{
    auto it = find(m_modules.begin(), m_modules.end(), module);
    if (it == m_modules.end())
        return no_index;
    return it - m_modules.begin();
}


Index Module::add_function(std::unique_ptr<Function>&& fn)
{
    m_functions.push_back(move(fn));
    return m_functions.size() - 1;
}


Index Module::add_value(std::unique_ptr<Value>&& value)
{
    m_values.add(move(value));
    return m_values.size() - 1;
}


Index Module::add_type(TypeInfo type_info)
{
    m_types.push_back(move(type_info));
    return m_types.size() - 1;
}


Index Module::add_class(std::unique_ptr<Class>&& cls)
{
    m_classes.push_back(move(cls));
    return m_classes.size() - 1;
}


Index Module::add_instance(std::unique_ptr<Instance>&& inst)
{
    m_instances.push_back(move(inst));
    return m_instances.size() - 1;
}


bool Module::operator==(const Module& rhs) const
{
    return m_modules == rhs.m_modules &&
           m_functions == rhs.m_functions &&
           m_values == rhs.m_values;
}


struct StreamOptions {
    unsigned level;
};

static StreamOptions& stream_options(std::ostream& os) {
    static int idx = std::ios_base::xalloc();
    return reinterpret_cast<StreamOptions&>(os.iword(idx));
}

static std::ostream& put_indent(std::ostream& os)
{
    std::string pad(stream_options(os).level * 3, ' ');
    return os << pad;
}

static std::ostream& more_indent(std::ostream& os)
{
    stream_options(os).level += 1;
    return os;
}

static std::ostream& less_indent(std::ostream& os)
{
    assert(stream_options(os).level >= 1);
    stream_options(os).level -= 1;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Module& v)
{
    os << v.num_imported_modules() << " imported modules" << endl << more_indent;
    for (size_t i = 0; i < v.num_imported_modules(); ++i)
        os << put_indent << i << ": " << v.get_imported_module(i).symtab().name() << endl;
    os << less_indent;

    os << v.num_functions() << " functions" << endl << more_indent;
    for (size_t i = 0; i < v.num_functions(); ++i) {
        const auto& f = v.get_function(i);
        os << put_indent << i << ": " << f.symtab().name() << ": " << f << endl;
    }
    os << less_indent;

    os << v.num_values() << " static values" << endl << more_indent;
    for (size_t i = 0; i < v.num_values(); ++i) {
        const auto& val = v.get_value(i);
        os << put_indent << i << ": " << val << endl;
    }
    os << less_indent;

    os << v.num_types() << " types" << endl << more_indent;
    for (size_t i = 0; i < v.num_types(); ++i) {
        const auto& typ = v.get_type(i);
        os << put_indent << i << ": " << typ << endl;
    }
    os << less_indent;

    os << v.num_classes() << " type classes" << endl << more_indent;
    for (size_t i = 0; i < v.num_classes(); ++i) {
        const auto& cls = v.get_class(i);
        os << put_indent << i << ": " << cls.symtab() << endl;
    }
    os << less_indent;

    os << v.num_instances() << " instances" << endl << more_indent;
    for (size_t i = 0; i < v.num_instances(); ++i) {
        const auto& inst = v.get_instance(i);
        os << put_indent << i << ": " << inst.type_inst() << endl;
    }
    os << less_indent;

    return os;
}


} // namespace xci::script
