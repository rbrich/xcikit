// dump.cpp created on 2019-10-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "dump.h"
#include "Function.h"
#include "Module.h"
#include <xci/core/string.h>
#include <xci/compat/macros.h>
#include <iomanip>

namespace xci::script {

using std::endl;
using std::left;
using std::right;
using std::setw;


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
    void visit(const Literal& v) override { m_os << v; }
    void visit(const Bracketed& v) override { m_os << v; }
    void visit(const Tuple& v) override { m_os << v; }
    void visit(const List& v) override { m_os << v; }
    void visit(const Reference& v) override { m_os << v; }
    void visit(const Call& v) override { m_os << v; }
    void visit(const OpCall& v) override { m_os << v; }
    void visit(const Condition& v) override { m_os << v; }
    void visit(const Function& v) override { m_os << v; }
    void visit(const Cast& v) override { m_os << v; }
    void visit(const TypeName& v) override { m_os << v; }
    void visit(const FunctionType& v) override { m_os << v; }
    void visit(const ListType& v) override { m_os << v; }

private:
    std::ostream& m_os;
};

std::ostream& operator<<(std::ostream& os, const Literal& v)
{
    if (stream_options(os).enable_tree) {
        return os << "Literal(Expression) " << *v.value << endl;
    } else {
        return os << *v.value;
    }
}


std::ostream& operator<<(std::ostream& os, const Bracketed& v)
{
    if (stream_options(os).enable_tree) {
        os << "Bracketed(Expression)" << endl;
        return os << more_indent << put_indent << *v.expression << less_indent;
    } else {
        return os << "(" << *v.expression << ")";
    }
}


std::ostream& operator<<(std::ostream& os, const Tuple& v)
{
    if (stream_options(os).enable_tree) {
        os << "Tuple(Expression)" << endl;
        os << more_indent;
        for (const auto& item : v.items)
            os << put_indent << *item;
        return os << less_indent;
    } else {
        for (const auto& item : v.items) {
            os << *item;
            if (&item != &v.items.back())
                os << ", ";
        }
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const List& v)
{
    if (stream_options(os).enable_tree) {
        os << "List(Expression)" << endl;
        os << more_indent;
        for (const auto& item : v.items)
            os << put_indent << *item;
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
        os << "Variable" << endl;
        os << more_indent << put_indent << v.identifier;
        if (v.type)
            os << put_indent << *v.type;
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
        os << "Parameter" << endl;
        os << more_indent;
        if (v.identifier)
            os << put_indent << v.identifier;
        if (v.type)
            os << put_indent << *v.type << endl;
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
        os << "Identifier " << v.name;
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
            os << "TypeName(Type) " << v.name;
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
        os << "FunctionType(Type)" << endl;
        os << more_indent;
        for (const auto& prm : v.params)
            os << put_indent << prm;
        if (v.result_type)
            os << put_indent << "result: " << *v.result_type << endl;
        for (const auto& ctx : v.context) {
            os << put_indent << ctx << endl;
        }
        return os << less_indent;
    } else {
        if (!v.params.empty()) {
            for (const auto& prm : v.params) {
                os << prm << ' ';
            }
        }
        if (v.result_type) {
            os << "-> " << *v.result_type << " ";
        }
        if (!v.context.empty()) {
            os << "with (";
            for (const auto& ctx : v.context) {
                os << ctx;
                if (&ctx != &v.context.back())
                    os << ", ";
            }
            os << ") ";
        }
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const ListType& v)
{
    if (stream_options(os).enable_tree) {
        os << "ListType(Type)" << endl;
        if (v.elem_type)
            os << more_indent << put_indent << *v.elem_type << less_indent;
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
        os << "TypeConstraint" << endl
           << more_indent
           << put_indent << v.type_class << endl
           << put_indent << v.type_name << less_indent;
        return os;
    } else {
        return os << v.type_class << ' ' << v.type_name;
    }
}

std::ostream& operator<<(std::ostream& os, const Reference& v)
{
    if (stream_options(os).enable_tree) {
        os << "Reference(Expression)" << endl;
        return os << more_indent << put_indent << v.identifier << less_indent;
    } else {
        return os << v.identifier;
    }
}

std::ostream& operator<<(std::ostream& os, const Call& v)
{
    if (stream_options(os).enable_tree) {
        os << "Call(Expression)" << endl;
        os << more_indent << put_indent << *v.callable;
        for (const auto& arg : v.args) {
            os << put_indent << *arg;
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
        os << "OpCall(Expression)" << endl;
        os << more_indent << put_indent << v.op;
        if (v.callable)
            os << put_indent << *v.callable;
        for (const auto& arg : v.args) {
            os << put_indent << *arg;
        }
        return os << less_indent;
    } else {
        os << "(";
        for (const auto& arg : v.args) {
            if (&arg != &v.args.front()) {
                if (v.op.is_comma())
                    os << ", ";  // no leading space
                else
                    os << ' ' << v.op << ' ';
            }
            os << *arg;
        }
        return os << ")";
    }
}

std::ostream& operator<<(std::ostream& os, const Condition& v)
{
    if (stream_options(os).enable_tree) {
        os << "Condition(Expression)" << endl;
        os << more_indent
           << put_indent << *v.cond
           << put_indent << *v.then_expr
           << put_indent << *v.else_expr;
        return os << less_indent;
    } else {
        os << "if " << *v.cond << " then " << *v.then_expr << " else " << *v.else_expr << ";";
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Operator& v)
{
    if (stream_options(os).enable_tree) {
        return os << "Operator " << v.to_cstr()
                  << " [L" << v.precedence() << "]" << endl;
    } else {
        return os << v.to_cstr();
    }
}

std::ostream& operator<<(std::ostream& os, const Function& v)
{
    if (stream_options(os).enable_tree) {
        os << "Function(Expression)" << endl;
        return os << more_indent
                  << put_indent << v.type
                  << put_indent << v.body
                  << less_indent;
    } else {
        return os << "fun " << v.type << "{" << v.body << "}";
    }
}

std::ostream& operator<<(std::ostream& os, const Cast& v)
{
    if (stream_options(os).enable_tree) {
        os << "Cast(Expression)" << endl;
        os << more_indent
           << put_indent << *v.expression
           << put_indent << *v.type << endl;
        if (v.cast_function)
            os << put_indent << *v.cast_function;
        return os << less_indent;
    } else {
        return os << *v.expression << ":" << *v.type;
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
        os << "Definition(Statement)" << endl;
        os << more_indent << put_indent << v.variable;
        if (v.expression)
            os << put_indent << *v.expression;
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
        os << "Invocation(Statement)" << endl;
        return os << more_indent << put_indent << *v.expression << less_indent;
    } else {
        return os << *v.expression;
    }
}

std::ostream& operator<<(std::ostream& os, const Return& v)
{
    if (stream_options(os).enable_tree) {
        os << "Return(Statement)" << endl;
        return os << more_indent << put_indent << *v.expression << less_indent;
    } else {
        return os << *v.expression;
    }
}

std::ostream& operator<<(std::ostream& os, const Class& v)
{
    if (stream_options(os).enable_tree) {
        os << "Class" << endl;
        os << more_indent
           << put_indent << "name: " << v.class_name << endl;
        for (const auto& type_var : v.type_vars)
            os << put_indent << "var: " << type_var << endl;
        for (const auto& cst : v.context)
            os << put_indent << cst;
        for (const auto& def : v.defs)
            os << put_indent << def;
        return os << less_indent;
    } else {
        os << "class " << v.class_name;
        for (const auto& type_var : v.type_vars)
           os << ' ' << type_var;
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
        os << "Instance" << endl;
        os << more_indent << put_indent << v.class_name << endl;
        for (const auto& t : v.type_inst)
            os << put_indent << *t << endl;
        for (const auto& cst : v.context)
            os << put_indent << cst;
        for (const auto& def : v.defs)
            os << put_indent << def;
        return os << less_indent;
    } else {
        os << "instance " << v.class_name;
        for (const auto& t : v.type_inst)
            os << ' ' << *t;
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
        os << "Block";
        if (v.symtab != nullptr)
            os << " [" << std::hex << v.symtab << std::dec << "]";
        os << endl;
        os << more_indent;
        for (const auto& stmt : v.statements) {
            os << put_indent;
            stmt->apply(visitor);
        }
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
        os << "Module" << endl;
        return os << more_indent << put_indent << v.body << less_indent;
    } else {
        return os << v.body;
    }
}

}  // namespace ast


// Function

std::ostream& operator<<(std::ostream& os, const Function& f)
{
    os << f.signature() << endl;
    switch (f.kind()) {
        case Function::Kind::Compiled:
            for (auto it = f.code().begin(); it != f.code().end(); it++) {
                os << ' ' << DumpInstruction{f, it} << endl;
            }
            return os;
        case Function::Kind::Generic:   return os << dump_tree << f.ast() << endl;
        case Function::Kind::Native:    return os << "<native>" << endl;
        case Function::Kind::Undefined: return os << "<undefined>" << endl;
    }
    UNREACHABLE;
}


std::ostream& operator<<(std::ostream& os, Function::Kind v)
{
    switch (v) {
        case Function::Kind::Undefined:  return os << "undefined";
        case Function::Kind::Compiled:    return os << "compiled";
        case Function::Kind::Generic: return os << "generic";
        case Function::Kind::Native:  return os << "native";
    }
    UNREACHABLE;
}


std::ostream& operator<<(std::ostream& os, DumpInstruction&& v)
{
    auto inum = v.pos - v.func.code().begin();
    auto opcode = static_cast<Opcode>(*v.pos);
    os << right << setw(3) << inum << "  " << left << setw(20) << opcode;
    if (opcode >= Opcode::OneArgFirst && opcode <= Opcode::OneArgLast) {
        // 1 arg
        Index arg = *(++v.pos);
        os << static_cast<int>(arg);
        switch (opcode) {
            case Opcode::LoadStatic:
                os << " (" << v.func.module().get_value(arg) << ")";
                break;
            case Opcode::LoadFunction:
            case Opcode::Call0: {
                const auto& fn = v.func.module().get_function(arg);
                os << " (" << fn.symtab().name() << ' ' << fn.signature() << ")";
                break;
            }
            case Opcode::Call1: {
                const auto& fn = v.func.module().get_imported_module(0).get_function(arg);
                os << " (" << fn.symtab().name() << ' ' << fn.signature() << ")";
                break;
            }
            default:
                break;
        }
    }
    if (opcode >= Opcode::TwoArgFirst && opcode <= Opcode::TwoArgLast) {
        // 2 args
        Index arg1 = *(++v.pos);
        Index arg2 = *(++v.pos);
        os << static_cast<int>(arg1) << ' ' << static_cast<int>(arg2);
        switch (opcode) {
            case Opcode::Call: {
                const auto& fn = v.func.module().get_imported_module(arg1).get_function(arg2);
                os << " (" << fn.symtab().name() << ' ' << fn.signature() << ")";
                break;
            }
            default:
                break;
        }
    }
    return os;
}


// Module

std::ostream& operator<<(std::ostream& os, const Module& v)
{
    os << "* " << v.num_imported_modules() << " imported modules" << endl << more_indent;
    for (size_t i = 0; i < v.num_imported_modules(); ++i)
        os << put_indent << '[' << i << "] " << v.get_imported_module(i).name() << endl;
    os << less_indent;

    os << "* " << v.num_functions() << " functions" << endl << more_indent;
    for (size_t i = 0; i < v.num_functions(); ++i) {
        const auto& f = v.get_function(i);
        os << put_indent << '[' << i << "] ";
        if (f.kind() != Function::Kind::Compiled)
            os << '(' << f.kind() << ") ";
        os << f.name() << ": " << f.signature() << endl;
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
        [[maybe_unused]] bool first_typevar = true;
        for (const auto& sym : cls.symtab()) {
            switch (sym.type()) {
                case Symbol::Parameter:
                    break;
                case Symbol::TypeVar:
                    assert(first_typevar);
                    first_typevar = false;
                    os << ' ' << sym.name() << endl << more_indent;
                    break;
                case Symbol::Function:
                    os << put_indent << sym.name() << ": "
                       << cls.get_function_type(sym.index()) << endl;
                    break;
                default:
                    assert(!"unexpected symbol type");
                    break;
            }
        }
        os << less_indent;
    }
    os << less_indent;

    os << "* " << v.num_instances() << " instances" << endl << more_indent;
    for (size_t i = 0; i < v.num_instances(); ++i) {
        const auto& inst = v.get_instance(i);
        os << put_indent << '[' << i << "] " << inst.class_().name();
        for (const auto& t : inst.types())
            os << ' ' << t;
        os << endl << more_indent;
        for (size_t j = 0; j < inst.num_functions(); ++j) {
            const auto fi = inst.get_function(j);
            const auto& f = v.get_function(fi);
            os << put_indent << f.name() << ": " << f.signature() << endl;
        }
        os << less_indent;
    }
    os << less_indent;

    return os;
}


// Type

std::ostream& operator<<(std::ostream& os, const TypeInfo& v)
{
    switch (v.type()) {
        case Type::Unknown:     return os << "?";
        case Type::Void:        return os << "Void";
        case Type::Bool:        return os << "Bool";
        case Type::Byte:        return os << "Byte";
        case Type::Char:        return os << "Char";
        case Type::Int32:       return os << "Int32";
        case Type::Int64:       return os << "Int64";
        case Type::Float32:     return os << "Float32";
        case Type::Float64:     return os << "Float64";
        case Type::String:      return os << "String";
        case Type::List:
            return os << "[" << v.subtypes()[0] << "]";
        case Type::Tuple: {
            os << "(";
            for (const auto& ti : v.subtypes()) {
                os << ti;
                if (&ti != &v.subtypes().back())
                    os << ", ";
            }
            return os << ")";
        }
        case Type::Function:    return os << v.signature();
        case Type::Module:      return os << "Module";
    }
    UNREACHABLE;
}


std::ostream& operator<<(std::ostream& os, const Signature& v)
{
    if (!v.nonlocals.empty()) {
        os << "{ ";
        for (auto& ti : v.nonlocals) {
            os << ti << " ";
        }
        os << "} ";
    }
    if (!v.partial.empty()) {
        os << "( ";
        for (auto& ti : v.partial) {
            os << ti << " ";
        }
        os << ") ";
    }
    if (!v.params.empty()) {
        for (auto& ti : v.params) {
            os << ti << " ";
        }
        os << "-> ";
    }
    return os << v.return_type;
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
        case Symbol::Fragment:      return os << "Fragment";
        case Symbol::Module:        return os << "Module";
        case Symbol::Instruction:   return os << "Instruction";
        case Symbol::Class:         return os << "Class";
        case Symbol::Method:        return os << "Method";
        case Symbol::Instance:      return os << "Instance";
        case Symbol::TypeName:      return os << "TypeName";
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
        os << " @" << v.symtab()->name() << " ("
           << std::hex << intptr_t(v.symtab()) << ')' << std::dec;
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
