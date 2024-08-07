// dump.cpp created on 2019-10-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "dump.h"
#include "Function.h"
#include "Module.h"
#include "typing/type_index.h"
#include <xci/data/coding/leb128.h>
#include <xci/compat/macros.h>
#include <fmt/format.h>
#include <iomanip>
#include <bitset>

namespace xci::script {

using xci::data::leb128_decode;
using std::endl;
using std::left;
using std::right;
using std::setw;


// stream manipulators

struct StreamOptions {
    bool enable_tree : 1 = false;
    bool module_verbose : 1 = false;  // Module: dump function bodies etc.
    bool enable_disassembly : 1 = false;  // When dumping function bytecode, disassemble it (via CodeAssembly)
    bool parenthesize_fun_types : 1 = false;
    bool multiline : 1 = false;
    bool qualify_type_vars : 1 = false;
    unsigned level : 6 = 0;
    std::bitset<32> rules;
};

static StreamOptions& stream_options(std::ostream& os) {
    static int idx = std::ios_base::xalloc();
    auto* p = reinterpret_cast<StreamOptions*>(os.pword(idx));
    if (!p) {
        p = new StreamOptions();
        os.pword(idx) = p;
        os.register_callback([](std::ios_base::event ev, std::ios_base& ios, int index){
            if (ev == std::ios_base::event::erase_event)
                delete reinterpret_cast<StreamOptions*>(ios.pword(index));
        }, idx);
    }
    return *p;
}

std::ostream& dump_tree(std::ostream& os)
{
    stream_options(os).enable_tree = true;
    return os;
}

std::ostream& dump_module_verbose(std::ostream& os)
{
    stream_options(os).module_verbose = true;
    return os;
}

std::ostream& dump_disassemble(std::ostream& os)
{
    stream_options(os).enable_disassembly = true;
    return os;
}

std::ostream& put_indent(std::ostream& os)
{
    auto& so = stream_options(os);
    for (unsigned i = 0; i != so.level; ++i)
        if (so.rules[i])
            os << ".  ";
        else
            os << "   ";
    return os;
}

std::ostream& rule_indent(std::ostream& os)
{
    auto& so = stream_options(os);
    so.rules[so.level] = true;
    so.level += 1;
    return os;
}

std::ostream& more_indent(std::ostream& os)
{
    auto& so = stream_options(os);
    so.rules[so.level] = false;
    so.level += 1;
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
    void visit(const Block& v) override { m_os << v; }
    void visit(const Definition& v) override { m_os << v; }
    void visit(const Invocation& v) override { m_os << v; }
    void visit(const Return& v) override { m_os << v; }
    void visit(const Class& v) override { m_os << v; }
    void visit(const Instance& v) override { m_os << v; }
    void visit(const TypeDef& v) override { m_os << v; }
    void visit(const TypeAlias& v) override { m_os << v; }
    void visit(const Literal& v) override { m_os << v; }
    void visit(const Parenthesized& v) override { m_os << v; }
    void visit(const Tuple& v) override { m_os << v; }
    void visit(const List& v) override { m_os << v; }
    void visit(const StructInit& v) override { m_os << v; }
    void visit(const Reference& v) override { m_os << v; }
    void visit(const Call& v) override { m_os << v; }
    void visit(const OpCall& v) override { m_os << v; }
    void visit(const Condition& v) override { m_os << v; }
    void visit(const WithContext& v) override { m_os << v; }
    void visit(const Function& v) override { m_os << v; }
    void visit(const Cast& v) override { m_os << v; }
    void visit(const TypeName& v) override { m_os << v; }
    void visit(const FunctionType& v) override { m_os << v; }
    void visit(const ListType& v) override { m_os << v; }
    void visit(const TupleType& v) override { m_os << v; }
    void visit(const StructType& v) override { m_os << v; }

private:
    std::ostream& m_os;
};

std::ostream& operator<<(std::ostream& os, const Literal& v)
{
    if (stream_options(os).enable_tree) {
        os << "Literal(Expression) " << v.value;
        if (v.ti)
            os << " [type_info=" << v.ti << ']';
        return os << endl;
    } else {
        return os << v.value;
    }
}


std::ostream& operator<<(std::ostream& os, const Parenthesized& v)
{
    if (stream_options(os).enable_tree) {
        os << "Parenthesized(Expression)" << endl;
        return os << more_indent << put_indent << *v.expression << less_indent;
    } else {
        return os << "(" << *v.expression << ")";
    }
}


std::ostream& operator<<(std::ostream& os, const Tuple& v)
{
    if (stream_options(os).enable_tree) {
        os << "Tuple(Expression)";
        if (v.ti)
            os << " [type_info=" << v.ti << ']';
        os << endl << more_indent;
        for (const auto& item : v.items)
            os << put_indent << *item;
        return os << less_indent;
    } else {
        os << '(';
        for (const auto& item : v.items) {
            os << *item;
            if (&item != &v.items.back())
                os << ", ";
        }
        return os << ')';
    }
}


std::ostream& operator<<(std::ostream& os, const List& v)
{
    if (stream_options(os).enable_tree) {
        os << "List(Expression)";
        if (v.ti)
            os << " [type_info=" << v.ti << ']';
        os << endl << more_indent;
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


std::ostream& operator<<(std::ostream& os, const StructInit& v)
{
    if (stream_options(os).enable_tree) {
        os << "StructInit(Expression)";
        if (v.ti)
            os << " [type_info=" << v.ti << ']';
        os << endl << more_indent;
        for (const auto& item : v.items) {
            os << put_indent << item.first;
            os << put_indent << *item.second;
        }
        return os << less_indent;
    } else {
        for (const auto& item : v.items) {
            os << item.first << "=" << *item.second;
            if (&item != &v.items.back())
                os << ", ";
        }
        return os;
    }
}


std::ostream& operator<<(std::ostream& os, const StructItem& v)
{
    if (stream_options(os).enable_tree) {
        os << "StructItem" << endl;
        os << more_indent
           << put_indent << v.identifier;
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
            os << put_indent << *v.type;
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
        fmt::print(os, "Identifier {}", v.name);
        if (v.symbol) {
            os << " [" << v.symbol << "]";
        }
        return os << endl;
    } else {
        return os << v.name.view();
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
        if (v) {
            fmt::print(os, "TypeName(Type) {}", v.name);
            if (v.symbol) {
                os << " [" << v.symbol << "]";
            }
        }
        return os << endl;
    } else {
        return os << v.name.view();
    }
}

std::ostream& operator<<(std::ostream& os, const FunctionType& v)
{
    if (stream_options(os).enable_tree) {
        os << "FunctionType(Type)" << endl;
        os << more_indent;
        for (const auto& tp : v.type_params)
            os << put_indent << tp;
        os << put_indent << v.param;
        if (v.return_type)
            os << put_indent << "result: " << *v.return_type;
        for (const auto& ctx : v.context) {
            os << put_indent << ctx;
        }
        return os << less_indent;
    } else {
        if (!v.type_params.empty()) {
            os << '<';
            for (const auto& tp : v.type_params) {
                os << tp;
                if (&tp != &v.type_params.back())
                    os << ", ";
            }
            os << "> ";
        }
        os << v.param;
        if (v.return_type) {
            os << " -> " << *v.return_type;
        }
        if (!v.context.empty()) {
            os << " with (";
            for (const auto& ctx : v.context) {
                os << ctx;
                if (&ctx != &v.context.back())
                    os << ", ";
            }
            os << ")";
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

std::ostream& operator<<(std::ostream& os, const TupleType& v)
{
    if (stream_options(os).enable_tree) {
        os << "TupleType(Type)" << endl << more_indent;
        for (const auto& t : v.subtypes)
            os << put_indent << *t;
        return os << less_indent;
    } else {
        os << '(';
        for (const auto& t : v.subtypes) {
            os << *t;
            if (t.get() != v.subtypes.back().get())
                os << ", ";
        }
        return os << ')';
    }
}


std::ostream& operator<<(std::ostream& os, const StructType& v)
{
    if (stream_options(os).enable_tree) {
        os << "StructType(Type)" << endl << more_indent;
        for (const auto& t : v.subtypes)
            os << put_indent << t;
        return os << less_indent;
    } else {
        os << '(';
        for (const auto& t : v.subtypes) {
            os << t;
            if (&t != &v.subtypes.back())
                os << ", ";
        }
        return os << ')';
    }
}


std::ostream& operator<<(std::ostream& os, const TypeConstraint& v)
{
    if (stream_options(os).enable_tree) {
        os << "TypeConstraint" << endl
           << more_indent
           << put_indent << v.type_class
           << put_indent << v.type_name
           << less_indent;
        return os;
    } else {
        return os << v.type_class << ' ' << v.type_name;
    }
}

std::ostream& operator<<(std::ostream& os, const Reference& v)
{
    if (stream_options(os).enable_tree) {
        os << "Reference(Expression)";
        if (v.ti)
            os << " [type_info=" << v.ti << ']';
        const auto symptr = v.identifier.symbol;
        if (symptr && symptr->type() == Symbol::Function && v.index != no_index) {
            fmt::print(os, " [Function #{} @{}: {}]", v.index, v.module->name(),
                       v.module->get_scope(v.index).function().signature());
        }
        os << endl
           << more_indent
           << put_indent << v.identifier;
        for (const auto& type_arg : v.type_args)
           os << put_indent << "type_arg: " << *type_arg;
        return os << less_indent;
    } else {
        os << v.identifier;
        for (const auto& type_arg : v.type_args) {
            if (&type_arg == &v.type_args.front())
                os << '<';
            else
                os << ", ";
            os << *type_arg;
            if (&type_arg == &v.type_args.back())
                os << '>';
        }
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Call& v)
{
    if (stream_options(os).enable_tree) {
        os << "Call(Expression)";
        if (v.ti)
            os << " [type_info=" << v.ti << ']';
        os << endl << more_indent;
        if (v.callable)
            os << put_indent << "callable: " << *v.callable;
        if (v.arg) {
            os << put_indent << "arg: " << *v.arg;
        }
        return os << less_indent;
    } else {
        if (v.callable)
            os << *v.callable;
        if (v.callable && v.arg)
            os << ' ';
        if (v.arg)
            os << *v.arg;
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const OpCall& v)
{
    if (stream_options(os).enable_tree) {
        os << "OpCall(Expression)" << endl;
        os << more_indent << put_indent << v.op;
        if (v.callable)
            os << put_indent << "callable: " << *v.callable;
        if (v.arg)
            os << put_indent << "arg: " << *v.arg;
        if (v.right_arg)
            os << put_indent << "right_arg: " << *v.right_arg;
        return os << less_indent;
    } else {
        if (!v.right_arg && v.op.op != Operator::Comma && v.op.op != Operator::Call) {
            os << '(' << v.op << ") " << *v.arg;
            return os;
        }
        os << "(";
        if (v.arg)
            os << *v.arg;
        if (v.arg && v.right_arg) {
            switch (v.op.op) {
                case Operator::Comma:
                    os << ", ";  // no leading space
                    break;
                case Operator::Call:
                    os << ' ';
                    break;
                default:
                    os << ' ' << v.op << ' ';
                    break;
            }
        }
        if (v.right_arg)
            os << *v.right_arg;
        return os << ")";
    }
}

std::ostream& operator<<(std::ostream& os, const Condition& v)
{
    if (stream_options(os).enable_tree) {
        os << "Condition(Expression)" << endl;
        os << more_indent;
        for (auto& item : v.if_then_expr) {
           os << put_indent << "if: " << *item.first
              << put_indent << "then: " << *item.second;
        }
        os << put_indent << "else: " << *v.else_expr;
        return os << less_indent;
    } else {
        for (auto& item : v.if_then_expr) {
            os << "if " << *item.first << " then " << *item.second << '\n';
        }
        os << "else " << *v.else_expr << ";";
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const WithContext& v)
{
    if (stream_options(os).enable_tree) {
        os << "WithContext(Expression)" << endl;
        os << more_indent
           << put_indent << *v.context
           << put_indent << *v.expression;
        if (v.enter_function.identifier)
            os << put_indent << v.enter_function;
        if (v.leave_function.identifier)
            os << put_indent << v.leave_function;
        return os << less_indent;
    } else {
        os << "with " << *v.context << " " << *v.expression << ";";
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const Operator& v)
{
    if (stream_options(os).enable_tree) {
        return os << "Operator '" << v.to_cstr()
                  << "' [L" << v.precedence() << "]" << endl;
    } else {
        return os << v.to_cstr();
    }
}

std::ostream& operator<<(std::ostream& os, const Function& v)
{
    if (stream_options(os).enable_tree) {
        os << "Function(Expression)";
        if (v.ti)
            os << " [type_info=" << v.ti << ']';
        os << endl;
        return os << rule_indent
                  << put_indent << v.type
                  << put_indent << v.body
                  << less_indent;
    } else {
        if (v.type)
            os << "fun " << v.type << ' ';
        if (stream_options(os).multiline)
            return os << "{" << endl
                      << more_indent << put_indent << v.body << endl
                      << less_indent << put_indent << "}";
        else
            return os << "{" << v.body << "}";
    }
}

std::ostream& operator<<(std::ostream& os, const Cast& v)
{
    if (stream_options(os).enable_tree) {
        os << (v.is_init ? "Init(Expression)" : "Cast(Expression)");
        if (v.ti)
            os << " [type_info=" << v.ti << ']';
        os << endl
           << more_indent;
        if (v.expression)
           os << put_indent << *v.expression;
        os << put_indent << *v.type;
        if (v.cast_function)
            os << put_indent << *v.cast_function;
        return os << less_indent;
    } else {
        if (v.is_init)
            return os << *v.type << ' ' << *v.expression;
        else
            return os << *v.expression << ':' << *v.type;
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
        os << v.variable;
        if (v.expression)
            os << " = " << *v.expression;
        return os;
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
           << put_indent << "name: " << v.class_name;
        for (const auto& type_var : v.type_vars)
            os << put_indent << "var: " << type_var;
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
        os << more_indent << put_indent << v.class_name;
        for (const auto& t : v.type_inst)
            os << put_indent << *t;
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

std::ostream& operator<<(std::ostream& os, const TypeDef& v)
{
    if (stream_options(os).enable_tree) {
        os << "TypeDef" << endl;
        os << more_indent
           << put_indent << v.type_name
           << put_indent << *v.type
           << less_indent;
        return os;
    } else {
        return os << "type " << v.type_name << " = " << *v.type;
    }
}

std::ostream& operator<<(std::ostream& os, const TypeAlias& v)
{
    if (stream_options(os).enable_tree) {
        os << "TypeAlias" << endl;
        os << more_indent
           << put_indent << v.type_name
           << put_indent << *v.type
           << less_indent;
        return os;
    } else {
        return os << v.type_name << " = " << *v.type;
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
        const bool multiline = stream_options(os).multiline;
        for (const auto& stmt : v.statements) {
            stmt->apply(visitor);
            if (&stmt != &v.statements.back()) {
                if (multiline)
                    os << endl << put_indent;
                else
                    os << "; ";
            }
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
        case Function::Kind::Bytecode:
            if (stream_options(os).enable_disassembly) {
                CodeAssembly dis;
                dis.disassemble(f.bytecode());
                for (const auto& instr : dis)
                    os << ' ' << DumpInstruction{f, instr} << '\n';
            } else {
                for (auto it = f.bytecode().begin(); it != f.bytecode().end();)
                    os << ' ' << DumpBytecode{f, it} << '\n';
            }
            return os;
        case Function::Kind::Assembly:
            for (const auto& instr : f.asm_code()) {
                os << ' ' << DumpInstruction{f, instr} << '\n';
            }
            return os;
        case Function::Kind::Generic:
            return os << dump_tree << f.ast() << endl;
        case Function::Kind::Native:    return os << "<native>" << endl;
        case Function::Kind::Undefined: return os << "<undefined>" << endl;
    }
    XCI_UNREACHABLE;
}

std::ostream& operator<<(std::ostream& os, Function::Kind v)
{
    switch (v) {
        case Function::Kind::Undefined: return os << "undefined";
        case Function::Kind::Bytecode:  return os << "bytecode";
        case Function::Kind::Assembly:  return os << "assembly";
        case Function::Kind::Generic:   return os << "generic";
        case Function::Kind::Native:    return os << "native";
    }
    XCI_UNREACHABLE;
}

static void dump_b1_instruction(std::ostream& os, Opcode opcode, uint8_t arg)
{
    os << fmt::format("0x{:02x}", arg);
    switch (opcode) {  // NOLINT
        case Opcode::Jump:
        case Opcode::JumpIfNot:
            os << fmt::format(" (+{})", arg);
            break;
        case Opcode::Cast: {
            const auto from_type = decode_arg_type(arg >> 4);
            const auto to_type = decode_arg_type(arg & 0xf);
            os << " (" << TypeInfo{from_type} << " -> " << TypeInfo{to_type} << ")";
            break;
        }
        default:
            break;
    }
}

static void dump_l1_instruction(std::ostream& os, Opcode opcode, size_t arg, const Module& mod)
{
    os << arg;
    switch (opcode) {
        case Opcode::LoadStatic: {
            const auto& value = mod.get_value(Index(arg));
            os << " (" << value << ':' << value.type_info() << ")";
            break;
        }
        case Opcode::LoadFunction:
        case Opcode::MakeClosure:
        case Opcode::Call0:
        case Opcode::TailCall0: {
            const auto& fn = mod.get_function(Module::FunctionIdx(arg));
            fmt::print(os, " ({} {})", fn.symtab().name(), fn.signature());
            break;
        }
        case Opcode::Call1:
        case Opcode::TailCall1: {
            const auto& fn = mod.get_imported_module(0).get_function(Module::FunctionIdx(arg));
            fmt::print(os, " ({} {})", fn.symtab().name(), fn.signature());
            break;
        }
        case Opcode::ListSubscript:
        case Opcode::ListLength:
        case Opcode::ListSlice:
        case Opcode::ListConcat:
        case Opcode::Invoke: {
            const TypeInfo& ti = get_type_info(mod.module_manager(), Index(arg));
            fmt::print(os, " ({})", ti);
            break;
        }
        default:
            break;
    }
}

static void dump_l2_instruction(std::ostream& os, Opcode opcode, size_t arg1, size_t arg2, const Module& mod)
{
    os << arg1 << ' ' << arg2;
    switch (opcode) {  // NOLINT
        case Opcode::Call:
        case Opcode::TailCall: {
            const auto& fn = mod.get_imported_module(Index(arg1)).get_function(Module::FunctionIdx(arg2));
            fmt::print(os, " ({} {})", fn.symtab().name(), fn.signature());
            break;
        }
        case Opcode::MakeList: {
            const TypeInfo& ti = get_type_info(mod.module_manager(), Index(arg2));
            fmt::print(os, " ({})", ti);
            break;
        }
        default:
            break;
    }
}


std::ostream& operator<<(std::ostream& os, DumpInstruction&& v)
{
    const auto opcode = v.instr.opcode;
    if (opcode == Opcode::Annotation) {
        switch (static_cast<CodeAssembly::Annotation>(v.instr.args.first)) {
            case CodeAssembly::Annotation::Label:
                fmt::print(os, ".j{}:", v.instr.args.second);
                return os;
            case CodeAssembly::Annotation::Jump:
            case CodeAssembly::Annotation::JumpIfNot:
                fmt::print(os, "     {:<20}.j{}", Opcode(v.instr.args.first), v.instr.args.second);
                return os;
        }
    }
    fmt::print(os, "     {:<20}", opcode);
    if (opcode >= Opcode::B1First && opcode <= Opcode::B1Last)
        dump_b1_instruction(os, opcode, v.instr.arg_B1());
    else if (opcode >= Opcode::L1First && opcode <= Opcode::L1Last)
        dump_l1_instruction(os, opcode, v.instr.args.first, v.func.module());
    else if (opcode >= Opcode::L2First && opcode <= Opcode::L2Last)
        dump_l2_instruction(os, opcode, v.instr.args.first, v.instr.args.second, v.func.module());
    return os;
}


std::ostream& operator<<(std::ostream& os, DumpBytecode&& v)
{
    auto inum = v.pos - v.func.bytecode().begin();
    auto opcode = static_cast<Opcode>(*v.pos++);
    os << right << setw(3) << inum << "  " << left << setw(20) << opcode;
    if (opcode >= Opcode::B1First && opcode <= Opcode::B1Last) {
        const auto arg = *(v.pos++);
        dump_b1_instruction(os, opcode, arg);
    }
    else if (opcode >= Opcode::L1First && opcode <= Opcode::L1Last) {
        const auto arg = leb128_decode<Index>(v.pos);
        dump_l1_instruction(os, opcode, arg, v.func.module());
    }
    else if (opcode >= Opcode::L2First && opcode <= Opcode::L2Last) {
        const auto arg1 = leb128_decode<Index>(v.pos);
        const auto arg2 = leb128_decode<Index>(v.pos);
        dump_l2_instruction(os, opcode, arg1, arg2, v.func.module());
    }
    return os;
}


// Module

std::ostream& operator<<(std::ostream& os, const Module& v)
{
    bool verbose = stream_options(os).module_verbose;
    bool dump_tree = stream_options(os).enable_tree;
    os << "* " << v.num_imported_modules() << " imported modules\n" << more_indent;
    for (Index i = 0; i < v.num_imported_modules(); ++i) {
        os << put_indent;
        fmt::print(os, "[{}] {}\n", i, v.get_imported_module(i).name());;
    }
    os << less_indent;

    os << "* " << v.num_functions() << " functions\n" << more_indent;
    for (Index i = 0; i < v.num_functions(); ++i) {
        const auto& f = v.get_function(i);
        os << put_indent << '[' << i << "] ";
        if (f.kind() == Function::Kind::Generic) {
            os << '(' << f.kind();
            if (f.is_expression())
                os << ", expr";
            if (f.has_compile())
                os << ", compile";
            if (f.is_specialized())
                os << ", specialized";
            if (f.has_nonlocals_resolved())
                os << ", nlres";
            os << ") ";
        }
        os << f.qualified_name();
        if (f.signature())
            os << ' ' << f.signature();
        os << '\n';
        if (verbose && f.kind() == Function::Kind::Generic) {
            os << more_indent << put_indent << f.ast() << less_indent;
            if (!dump_tree)
                os << '\n';
        }
        if (verbose && f.kind() == Function::Kind::Assembly) {
            os << more_indent;
            for (const auto& instr : f.asm_code()) {
                os << put_indent << DumpInstruction{f, instr} << '\n';
            }
            os << less_indent;
        }
        if (verbose && f.kind() == Function::Kind::Bytecode) {
            os << more_indent;
            if (stream_options(os).enable_disassembly) {
                CodeAssembly dis;
                dis.disassemble(f.bytecode());
                for (const auto& instr : dis)
                    os << put_indent << DumpInstruction{f, instr} << '\n';
            } else {
                for (auto it = f.bytecode().begin(); it != f.bytecode().end();)
                    os << put_indent << DumpBytecode{f, it} << '\n';
            }
            os << less_indent;
        }
    }
    os << less_indent;

    os << "* " << v.num_values() << " static values\n" << more_indent;
    for (Index i = 0; i < v.num_values(); ++i) {
        const auto& val = v.get_value(i);
        os << put_indent << '[' << i << "] " << val << '\n';
    }
    os << less_indent;

    os << "* " << v.num_types() << " types\n" << more_indent;
    for (Index i = 0; i < v.num_types(); ++i) {
        const auto& ti = v.get_type(i);
        os << put_indent << '[' << i << "] " << ti;
        if (ti.is_named())
            os << " = " << ti.underlying();
        os << '\n';
    }
    os << less_indent;

    os << "* " << v.num_classes() << " type classes\n" << more_indent;
    for (Index i = 0; i < v.num_classes(); ++i) {
        const auto& cls = v.get_class(i);
        os << put_indent;
        fmt::print(os, "[{}] {}", i, cls.name());
        bool first_method = true;
        for (const auto& sym : cls.symtab()) {
            switch (sym.type()) {
                case Symbol::Parameter:
                    break;
                case Symbol::TypeVar:
                    fmt::print(os, " {}", sym.name());
                    break;
                case Symbol::Function: {
                    if (first_method) {
                        os << '\n' << more_indent;
                        first_method = false;
                    }
                    SymbolPointer symptr = cls.symtab().find(sym);
                    os << put_indent;
                    fmt::print(os, "{}: {}\n", sym.name(),
                               symptr.get_generic_scope().function().signature());
                    break;
                }
                default:
                    assert(!"unexpected symbol type");
                    break;
            }
        }
        os << less_indent;
    }
    os << less_indent;

    os << "* " << v.num_instances() << " instances\n" << more_indent;
    for (Index i = 0; i < v.num_instances(); ++i) {
        const auto& inst = v.get_instance(i);
        os << put_indent;
        fmt::print(os, "[{}] {}", i, inst.class_().name());
        for (const auto& t : inst.types())
            os << ' ' << t;
        os << '\n' << more_indent;
        for (Index j = 0; j < inst.num_functions(); ++j) {
            const auto& inst_fn_info = inst.get_function(j);
            const auto& f = inst_fn_info.module->get_scope(inst_fn_info.scope_index).function();
            os << put_indent;
            fmt::print(os, "{}: {}\n", f.name(), f.signature());
        }
        os << less_indent;
    }
    os << less_indent;

    return os;
}


// Type

std::ostream& operator<<(std::ostream& os, const TypeInfo& v)
{
    const bool qualify = stream_options(os).qualify_type_vars;
    switch (v.type()) {
        case Type::Unknown: {
            auto var = v.generic_var();
            if (!var)
                return os << '?';
            if (qualify)
                os << var.symtab()->qualified_name() << "::";
            return os << var->name().view();
        }
        case Type::Bool:        return os << "Bool";
        case Type::Char:        return os << "Char";
        case Type::UInt8:       return os << "UInt8";
        case Type::UInt16:      return os << "UInt16";
        case Type::UInt32:      return os << "UInt32";
        case Type::UInt64:      return os << "UInt64";
        case Type::UInt128:     return os << "UInt128";
        case Type::Int8:        return os << "Int8";
        case Type::Int16:       return os << "Int16";
        case Type::Int32:       return os << "Int32";
        case Type::Int64:       return os << "Int64";
        case Type::Int128:      return os << "Int128";
        case Type::Float32:     return os << "Float32";
        case Type::Float64:     return os << "Float64";
        case Type::Float128:    return os << "Float128";
        case Type::String:      return os << "String";
        case Type::List:
            return os << "[" << v.elem_type() << "]";
        case Type::Tuple:
        case Type::Struct: {
            os << "(";
            for (const auto& item : v.subtypes()) {
                if (&item != &v.subtypes().front())
                    os << ", ";
                if (item.key())
                    fmt::print(os, "{}: ", item.key());
                os << item;
            }
            return os << ")";
        }
        case Type::Function:
            if (stream_options(os).parenthesize_fun_types)
                return os << '(' << v.signature() << ')';
            else
                return os << v.signature();
        case Type::Module:      return os << "Module";
        case Type::Stream:      return os << "Stream";
        case Type::TypeIndex:   return os << "TypeIndex";
        case Type::Named:       return os << v.name().view();
    }
    XCI_UNREACHABLE;
}


std::ostream& operator<<(std::ostream& os, const Signature& v)
{
    if (!v.nonlocals.empty()) {
        os << "{ ";
        for (const auto& ti : v.nonlocals) {
            os << ti;
            if (&ti != &v.nonlocals.back())
                os << ", ";
        }
        os << " } ";
    }
    const bool orig_parenthesize_fun_types = stream_options(os).parenthesize_fun_types;
    stream_options(os).parenthesize_fun_types = true;
    os << v.param_type << ' ' << "-> ";
    stream_options(os).parenthesize_fun_types = orig_parenthesize_fun_types;
    return os << v.return_type;
}


// SymbolTable

std::ostream& operator<<(std::ostream& os, Symbol::Type v)
{
    switch (v) {
        case Symbol::Unresolved:        return os << "Unresolved";
        case Symbol::Value:             return os << "Value";
        case Symbol::Parameter:         return os << "Parameter";
        case Symbol::Nonlocal:          return os << "Nonlocal";
        case Symbol::Function:          return os << "Function";
        case Symbol::Module:            return os << "Module";
        case Symbol::Instruction:       return os << "Instruction";
        case Symbol::Class:             return os << "Class";
        case Symbol::Method:            return os << "Method";
        case Symbol::Instance:          return os << "Instance";
        case Symbol::TypeName:          return os << "TypeName";
        case Symbol::TypeVar:           return os << "TypeVar";
        case Symbol::StructItem:        return os << "StructItem";
        case Symbol::TypeIndex:         return os << "TypeIndex";
    }
    return os;
}


std::ostream& operator<<(std::ostream& os, const SymbolPointer& v)
{
    os << v->type();
    if (v->index() != no_index)
        os << " #" << v->index();
    if (v.symtab() != nullptr) {
        fmt::print(os, " @{} ({:x})", v.symtab()->name(), intptr_t(v.symtab()));
        if (v->type() == Symbol::Function && v.symtab()->module() && v->index() != no_index)
            os << ": " << v.get_generic_scope().function().signature();
    }
    if (v->ref())
        os << " -> " << v->ref();
    return os;
}


std::ostream& operator<<(std::ostream& os, const Symbol& v)
{
    fmt::print(os, "{:<20} {:<18}", v.name(), v.type());
    if (v.index() != no_index)
        os << " #" << v.index();
    if (v.ref()) {
        os << " -> " << v.ref()->type()
           << " #" << v.ref()->index();
        if (v.depth() == 0 && v.ref().symtab()->level() != 0)
            fmt::print(os, " @ {}", v.ref().symtab()->name());
        fmt::print(os, " ({})", v.ref()->name());
    }
    if (v.depth() != 0)
        os << ", depth -" << v.depth();
    return os;
}


std::ostream& operator<<(std::ostream& os, const SymbolTable& v)
{
    os << put_indent << "--- ";
    if (v.scope() != nullptr)
        os << '#' << v.scope()->function_index();
    fmt::print(os, " {} ---\n", v.name());
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


std::ostream& operator<<(std::ostream& os, const Scope& v)
{
    if (v.has_function()) {
        fmt::print(os, "Function #{} ({})", v.function_index(), v.function().name());
    }
    os << '\t';
    if (v.has_subscopes()) {
        os << "Subscopes: ";
        for (unsigned i = 0; i != v.num_subscopes(); ++i) {
            if (i != 0)
                os << ", ";
            os << v.get_subscope_index(i);
            assert(v.get_subscope(i).parent() == &v);
        }
    }
    os << '\t';
    if (v.has_nonlocals()) {
        os << "Nonlocals: ";
        bool first = true;
        for (auto nl : v.nonlocals()) {
            if (!first)
                os << ", ";
            else
                first = false;
            os << nl.index;
        }
    }
    os << '\t';
    const bool orig_parenthesize_fun_types = stream_options(os).parenthesize_fun_types;
    const bool orig_qualify_type_args = stream_options(os).qualify_type_vars;
    stream_options(os).parenthesize_fun_types = true;
    if (v.has_type_args()) {
        os << "Type args: ";
        bool first = true;
        for (const auto& arg : v.type_args()) {
            if (!first)
                os << ", ";
            else
                first = false;
            if (&v.function().symtab() != arg.first.symtab())
                os << arg.first.symtab()->qualified_name() << "::";  // qualify non-own symbols
            fmt::print(os, "{}=", arg.first->name());
            if (arg.second.is_unknown()) {
                auto var = arg.second.generic_var();
                stream_options(os).qualify_type_vars = (var.symtab() != &v.function().symtab());
            }
            os << arg.second;
        }
    }
    stream_options(os).parenthesize_fun_types = orig_parenthesize_fun_types;
    stream_options(os).qualify_type_vars = orig_qualify_type_args;
    return os;
}


std::ostream& operator<<(std::ostream& os, const TypeArgs& v)
{
    const bool qualify = stream_options(os).qualify_type_vars;
    bool first = true;
    for (const auto& arg : v) {
        if (!first)
            os << ", ";
        else
            first = false;
        if (qualify)
            os << arg.first.symtab()->qualified_name() << "::";
        fmt::print(os, "{}={}", arg.first->name(), arg.second);
    }
    return os;
}


} // namespace xci::script
