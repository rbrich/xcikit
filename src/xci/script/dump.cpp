// dump.cpp created on 2019-10-08, part of XCI toolkit
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

#include "dump.h"
#include "Function.h"
#include "Module.h"

namespace xci::script {

using std::endl;
using std::left;
using std::setw;
using std::string;
using std::ostringstream;


// stream manipulators

struct StreamOptions {
    bool enable_tree : 1;
    unsigned level : 5;
};

static StreamOptions& stream_options(std::ostream& os) {
    static int idx = std::ios_base::xalloc();
    return reinterpret_cast<StreamOptions&>(os.iword(idx));
}

std::ostream& dump_tree(std::ostream& os)
{
    stream_options(os).enable_tree = true;
    return os;
}

std::ostream& put_indent(std::ostream& os)
{
    std::string pad(stream_options(os).level * 3u, ' ');
    return os << pad;
}

std::ostream& more_indent(std::ostream& os)
{
    stream_options(os).level += 1;
    return os;
}

std::ostream& less_indent(std::ostream& os)
{
    assert(stream_options(os).level >= 1);
    stream_options(os).level -= 1;
    return os;
}


// AST

namespace ast {

class DumpVisitor: public ConstVisitor {
public:
    explicit DumpVisitor(std::ostream& os) : m_os(os) {}
    void visit(const Definition& v) override { m_os << v; }
    void visit(const Invocation& v) override { m_os << v; }
    void visit(const Return& v) override { m_os << v; }
    void visit(const Class& v) override { m_os << v; }
    void visit(const Instance& v) override { m_os << v; }
    void visit(const Integer& v) override { m_os << v; }
    void visit(const Float& v) override { m_os << v; }
    void visit(const String& v) override { m_os << v; }
    void visit(const Tuple& v) override { m_os << v; }
    void visit(const List& v) override { m_os << v; }
    void visit(const Reference& v) override { m_os << v; }
    void visit(const Call& v) override { m_os << v; }
    void visit(const OpCall& v) override { m_os << v; }
    void visit(const Condition& v) override { m_os << v; }
    void visit(const Function& v) override { m_os << v; }
    void visit(const TypeName& v) override { m_os << v; }
    void visit(const FunctionType& v) override { m_os << v; }
    void visit(const ListType& v) override { m_os << v; }

private:
    std::ostream& m_os;
};

std::ostream& operator<<(std::ostream& os, const Integer& v)
{
    if (stream_options(os).enable_tree) {
        return os << put_indent << "Integer(Expression) " << v.value << endl;
    } else {
        return os << v.value;
    }
}

std::ostream& operator<<(std::ostream& os, const Float& v)
{
    if (stream_options(os).enable_tree) {
        return os << put_indent << "Float(Expression) " << v.value << endl;
    } else {
        ostringstream sbuf;
        sbuf << v.value;
        auto str = sbuf.str();
        if (str.find('.') == string::npos)
            return os << str << ".0";
        else
            return os << str;
    }
}

std::ostream& operator<<(std::ostream& os, const String& v)
{
    if (stream_options(os).enable_tree) {
        return os << put_indent << "String(Expression) " << v.value << endl;
    } else {
        return os << '"' << v.value << '"';
    }
}

std::ostream& operator<<(std::ostream& os, const Tuple& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Tuple(Expression)" << endl;
        os << more_indent;
        for (const auto& item : v.items)
            os << *item;
        return os << less_indent;
    } else {
        os << "(";
        for (const auto& item : v.items) {
            os << *item;
            if (&item != &v.items.back())
                os << ", ";
        }
        return os << ")";
    }
}

std::ostream& operator<<(std::ostream& os, const List& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "List(Expression)" << endl;
        os << more_indent;
        for (const auto& item : v.items)
            os << *item;
        return os << less_indent;
    } else {
        os << "[";
        for (const auto& item : v.items) {
            os << *item;
            if (&item != &v.items.back())
                os << ", ";
        }
        return os << "]";
    }
}

std::ostream& operator<<(std::ostream& os, const Variable& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Variable" << endl;
        os << more_indent << v.identifier;
        if (v.type)
            os << *v.type;
        return os << less_indent;
    } else {
        os << v.identifier;
        if (v.type)
            os << ':' << *v.type;
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Parameter& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Parameter" << endl;
        os << more_indent;
        if (v.identifier)
            os << v.identifier;
        if (v.type)
            os << *v.type << endl;
        return os << less_indent;
    } else {
        if (v.identifier) {
            os << v.identifier;
            if (v.type)
                os << ':';
        }
        if (v.type)
            os << *v.type;
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Identifier& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Identifier " << v.name;
        if (v.symbol) {
            os << " [" << v.symbol << "]";
        }
        return os << endl;
    } else {
        return os << v.name;
    }
}

std::ostream& operator<<(std::ostream& os, const Type& v)
{
    DumpVisitor visitor(os);
    v.apply(visitor);
    return os;
}

std::ostream& operator<<(std::ostream& os, const TypeName& v)
{
    if (stream_options(os).enable_tree) {
        if (!v.name.empty()) {
            os << put_indent << "TypeName(Type) " << v.name;
            if (v.symbol) {
                os << " [" << v.symbol << "]";
            }
        }
        return os;
    } else {
        return os << v.name;
    }
}

std::ostream& operator<<(std::ostream& os, const FunctionType& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "FunctionType(Type)" << endl;
        os << more_indent;
        for (const auto& prm : v.params)
            os << prm;
        if (v.result_type)
            os << *v.result_type << endl;
        return os << less_indent;
    } else {
        if (!v.params.empty()) {
            os << "|";
            for (const auto& prm : v.params) {
                if (&prm != &v.params.front())
                    os << ' ';
                os << prm;
            }
            os << "| ";
        }
        if (v.result_type) {
            os << "-> " << *v.result_type << " ";
        }
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const ListType& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "ListType(Type) " << endl;
        if (v.elem_type)
            os << more_indent << *v.elem_type << less_indent;
        return os;
    } else {
        os << "[";
        if (v.elem_type)
            os << *v.elem_type;
        return os << "]";
    }
}

std::ostream& operator<<(std::ostream& os, const TypeConstraint& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "TypeConstraint " << v.type_class << ' ' << v.type_name << endl;
        return os;
    } else {
        return os << v.type_class << ' ' << v.type_name;
    }
}

std::ostream& operator<<(std::ostream& os, const Reference& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Reference(Expression)" << endl;
        return os << more_indent << v.identifier << less_indent;
    } else {
        return os << v.identifier;
    }
}

std::ostream& operator<<(std::ostream& os, const Call& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Call(Expression)" << endl;
        os << more_indent << *v.callable;
        for (const auto& arg : v.args) {
            os << *arg;
        }
        return os << less_indent;
    } else {
        os << *v.callable;
        for (const auto& arg : v.args) {
            os << ' ' << *arg;
        }
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const OpCall& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "OpCall(Expression)" << endl;
        os << more_indent << v.op;
        if (v.callable)
            os << *v.callable;
        for (const auto& arg : v.args) {
            os << *arg;
        }
        return os << less_indent;
    } else {
        os << "(";
        for (const auto& arg : v.args) {
            if (&arg != &v.args.front())
                os << ' ' << v.op << ' ';
            os << *arg;
        }
        return os << ")";
    }
}

std::ostream& operator<<(std::ostream& os, const Condition& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Condition(Expression)" << endl;
        os << more_indent << *v.cond << *v.then_expr << *v.else_expr;
        return os << less_indent;
    } else {
        os << "if " << *v.cond << " then " << *v.then_expr << " else " << *v.else_expr << ";";
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Operator& v)
{
    if (stream_options(os).enable_tree) {
        return os << put_indent << "Operator " << v.to_cstr()
                  << " [L" << v.precedence() << "]" << endl;
    } else {
        return os << v.to_cstr();
    }
}

std::ostream& operator<<(std::ostream& os, const Function& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Function(Expression)" << endl;
        return os << more_indent << v.type << v.body << less_indent;
    } else {
        return os << "(" << v.type << "{" << v.body << "})";
    }
}

std::ostream& operator<<(std::ostream& os, const Expression& v)
{
    DumpVisitor visitor(os);
    v.apply(visitor);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Definition& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Definition(Statement)" << endl;
        os << more_indent << v.variable;
        if (v.expression)
            os << *v.expression;
        return os << less_indent;
    } else {
        os << "/*def*/ " << v.variable;
        if (v.expression)
            os << " = (" << *v.expression << ")";
        return os << ';';
    }
}

std::ostream& operator<<(std::ostream& os, const Invocation& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Invocation(Statement)" << endl;
        return os << more_indent << *v.expression << less_indent;
    } else {
        return os << *v.expression;
    }
}

std::ostream& operator<<(std::ostream& os, const Return& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Return(Statement)" << endl;
        return os << more_indent << *v.expression << less_indent;
    } else {
        return os << *v.expression;
    }
}

std::ostream& operator<<(std::ostream& os, const Class& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Class" << endl;
        os << more_indent << v.class_name << endl;
        os << v.type_var << endl;
        for (const auto& cst : v.context)
            os << cst;
        for (const auto& def : v.defs)
            os << def;
        return os << less_indent;
    } else {
        os << "class " << v.class_name << ' ' << v.type_var;
        if (!v.context.empty()) {
            os << " (";
            for (const auto& cst : v.context) {
                os << cst;
                if (&cst != &v.context.back())
                    os << ", ";
            }
            os << ")";
        }
        os << " {" << endl << more_indent;
        for (const auto& def : v.defs)
            os << put_indent << def << endl;
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Instance& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Instance" << endl;
        os << more_indent << v.class_name << endl;
        os << *v.type_inst << endl;
        for (const auto& cst : v.context)
            os << cst;
        for (const auto& def : v.defs)
            os << def;
        return os << less_indent;
    } else {
        os << "instance " << v.class_name << ' ' << *v.type_inst;
        if (!v.context.empty()) {
            os << " (";
            for (const auto& cst : v.context) {
                os << cst;
                if (&cst != &v.context.back())
                    os << ", ";
            }
            os << ")";
        }
        os << " {" << endl << more_indent;
        for (const auto& def : v.defs)
            os << put_indent << def << endl;
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Block& v)
{
    DumpVisitor visitor(os);
    if (stream_options(os).enable_tree) {
        os << put_indent << "Block";
        if (v.symtab != nullptr)
            os << " [" << std::hex << v.symtab << std::dec << "]";
        os << endl;
        os << more_indent;
        for (const auto& stmt : v.statements)
            stmt->apply(visitor);
        return os << less_indent;
    } else {
        for (const auto& stmt : v.statements) {
            stmt->apply(visitor);
            if (&stmt != &v.statements.back())
                os << endl;
        }
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Module& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Module" << endl;
        return os << more_indent << v.body << less_indent;
    } else {
        return os << v.body;
    }
}

}  // namespace ast


// Module

std::ostream& operator<<(std::ostream& os, const Module& v)
{
    os << "* " << v.num_imported_modules() << " imported modules" << endl << more_indent;
    for (size_t i = 0; i < v.num_imported_modules(); ++i)
        os << put_indent << '[' << i << "] " << v.get_imported_module(i).symtab().name() << endl;
    os << less_indent;

    os << "* " << v.num_functions() << " functions" << endl << more_indent;
    for (size_t i = 0; i < v.num_functions(); ++i) {
        const auto& f = v.get_function(i);
        os << put_indent << '[' << i << "] " << f.symtab().name() << ": " << f << endl;
    }
    os << less_indent;

    os << "* " << v.num_values() << " static values" << endl << more_indent;
    for (size_t i = 0; i < v.num_values(); ++i) {
        const auto& val = v.get_value(i);
        os << put_indent << '[' << i << "] " << val << endl;
    }
    os << less_indent;

    os << "* " << v.num_types() << " types" << endl << more_indent;
    for (size_t i = 0; i < v.num_types(); ++i) {
        const auto& typ = v.get_type(i);
        os << put_indent << '[' << i << "] " << typ << endl;
    }
    os << less_indent;

    os << "* " << v.num_classes() << " type classes" << endl << more_indent;
    for (size_t i = 0; i < v.num_classes(); ++i) {
        const auto& cls = v.get_class(i);
        os << put_indent << '[' << i << "] " << cls.name();
        for (const auto& sym : cls.symtab()) {
            if (sym.type() == Symbol::TypeVar) {
                os << ' ' << sym.name() << endl << more_indent;
                continue;
            }
            assert(sym.type() == Symbol::Value);
            os << put_indent << sym.name() << ": "
               << cls.get_function_type(sym.index()) << endl;
        }
        os << less_indent;
    }
    os << less_indent;

    os << "* " << v.num_instances() << " instances" << endl << more_indent;
    for (size_t i = 0; i < v.num_instances(); ++i) {
        const auto& inst = v.get_instance(i);
        os << put_indent << '[' << i << "] " << inst.class_().name()
           << ' ' << inst.type_inst() << endl;
        os << more_indent;
        for (size_t j = 0; j < inst.num_functions(); ++j) {
            const auto& f = inst.get_function(j);
            os << put_indent << f.symtab().name() << ": " << f << endl;
        }
        os << less_indent;
    }
    os << less_indent;

    return os;
}


// SymbolTable

std::ostream& operator<<(std::ostream& os, Symbol::Type v)
{
    switch (v) {
        case Symbol::Unresolved:    return os << "Unresolved";
        case Symbol::Value:         return os << "Value";
        case Symbol::Parameter:     return os << "Parameter";
        case Symbol::Nonlocal:      return os << "Nonlocal";
        case Symbol::Function:      return os << "Function";
        case Symbol::Module:        return os << "Module";
        case Symbol::Instruction:   return os << "Instruction";
        case Symbol::Class:         return os << "Class";
        case Symbol::ClassFunction: return os << "ClassFunction";
        case Symbol::Instance:      return os << "Instance";
        case Symbol::TypeVar:       return os << "TypeVar";
    }
    return os;
}


std::ostream& operator<<(std::ostream& os, const SymbolPointer& v)
{
    os << v->type();
    if (v->index() != no_index)
        os << " #" << v->index();
    if (v.symtab() != nullptr)
        os << " @" << std::hex << v.symtab() << std::dec;
    if (v->ref())
        os << " -> " << v->ref();
    return os;
}


std::ostream& operator<<(std::ostream& os, const Symbol& v)
{
    os << left << setw(20) << v.name() << " "
       << left << setw(18) << v.type();
    if (v.index() != no_index)
        os << " #" << v.index();
    if (v.ref())
        os << " -> " << v.ref()->type() << " #" << v.ref()->index();
    if (v.depth() != 0)
        os << ", depth -" << v.depth();
    return os;
}


std::ostream& operator<<(std::ostream& os, const SymbolTable& v)
{
    os << put_indent << "--- " << v.name() << " ---" << endl;
    for (const auto& sym : v) {
        os << put_indent << sym << endl;
    }

    os << more_indent;
    for (const auto& child : v.children()) {
        os << child << endl;
    }
    os << less_indent;
    return os;
}


} // namespace xci::script
