// AST.cpp created on 2019-05-15, part of XCI toolkit
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

#include "AST.h"
#include "Error.h"
#include <xci/compat/macros.h>

#include <range/v3/view/reverse.hpp>

#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cassert>
#include <iomanip>

using namespace std;

namespace xci::script {
namespace ast {


class DumpVisitor: public ConstVisitor {
public:
    explicit DumpVisitor(std::ostream& os) : m_os(os) {}
    void visit(const Definition& v) override { m_os << v; }
    void visit(const Invocation& v) override { m_os << v; }
    void visit(const Return& v) override { m_os << v; }
    void visit(const Integer& v) override { m_os << v; }
    void visit(const Float& v) override { m_os << v; }
    void visit(const String& v) override { m_os << v; }
    void visit(const Tuple& v) override { m_os << v; }
    void visit(const Call& v) override { m_os << v; }
    void visit(const OpCall& v) override { m_os << v; }
    void visit(const Condition& v) override { m_os << v; }
    void visit(const Function& v) override { m_os << v; }
    void visit(const TypeName& v) override { m_os << v; }
    void visit(const FunctionType& v) override { m_os << v; }

private:
    std::ostream& m_os;
};


void Block::finish()
{
    class FinishVisitor : public StatementVisitor {
    public:
        using StatementVisitor::visit;
        void visit(Definition&) override { /* skip */ }
        void visit(Invocation& inv) override {
            is_invocation = true;
            orig_expr = std::move(inv.expression);
        }
        void visit(Return&) override { is_return = true; }

    public:
        bool is_return = false;
        bool is_invocation = false;
        std::unique_ptr<Expression> orig_expr;
    };

    FinishVisitor visitor;
    for (auto& stmt : ranges::views::reverse(statements)) {
        stmt->apply(visitor);
        if (visitor.is_return) {
            // found a Return statement - all is fine
            return;
        }
        if (visitor.is_invocation) {
            // found the last Invocation - convert it to Return
            stmt = std::make_unique<Return>(std::move(visitor.orig_expr));
            return;
        }
    }

    // Missing return statement - insert `return void` automatically
    statements.push_back(std::make_unique<Return>(std::make_unique<Call>(Identifier{"void"})));
}


Integer::Integer(const std::string& s)
{
    char* end = nullptr;
    value = std::strtol(s.c_str(), &end, 10);
    if (end == nullptr || *end != '\0') {
        throw std::runtime_error("Integer not fully parsed.");
    }
}


Float::Float(const std::string& s) : value(0.0)
{
    std::istringstream is(s);
    is >> value;
    if (!is.eof()) {
        throw std::runtime_error("Float not fully parsed.");
    }
}

Operator::Operator(const std::string& s, bool prefix)
{
    assert(!s.empty());
    char c1 = s[0];
    char c2 = s.c_str()[1];
    switch (c1) {
        case '|':    op = (c2 == '|')? LogicalOr : BitwiseOr; break;
        case '&':    op = (c2 == '&')? LogicalAnd : BitwiseAnd; break;
        case '^':    op = BitwiseXor; break;
        case '=':    assert(c2 == '='); op = Equal; break;
        case '!':
            switch (c2) {  // NOLINT
                case '=':   op = NotEqual; break;
                default:    assert(prefix); op = LogicalNot; break;
            }
            break;
        case '<':
            switch (c2) {
                case '<':   op = ShiftLeft; break;
                case '=':   op = LessEqual; break;
                default:    op = LessThan; break;
            }
            break;
        case '>':
            switch (c2) {
                case '>':   op = ShiftRight; break;
                case '=':   op = GreaterEqual; break;
                default:    op = GreaterThan; break;
            }
            break;
        case '+':   op = prefix? UnaryPlus : Add; break;
        case '-':   op = prefix? UnaryMinus : Sub; break;
        case '*':   op = (c2 == '*')? Exp : Mul; break;
        case '/':   op = Div; break;
        case '%':   op = Mod; break;
        case '~':   assert(prefix); op = BitwiseNot; break;
        default: assert(!"Unreachable! Wrong OP!"); break;
    }
}

int Operator::precedence() const
{
    switch (op) {
        case Undefined:     return 0;
        case LogicalOr:     return 1;
        case LogicalAnd:    return 2;
        case Equal:         return 3;
        case NotEqual:      return 3;
        case LessEqual:     return 3;
        case GreaterEqual:  return 3;
        case LessThan:      return 3;
        case GreaterThan:   return 3;
        case BitwiseOr:     return 4;
        case BitwiseXor:    return 4;
        case BitwiseAnd:    return 5;
        case ShiftLeft:     return 6;
        case ShiftRight:    return 6;
        case Add:           return 7;
        case Sub:           return 7;
        case Mul:           return 8;
        case Div:           return 8;
        case Mod:           return 8;
        case Exp:           return 9;
        case LogicalNot:    return 10;
        case BitwiseNot:    return 10;
        case UnaryPlus:     return 10;
        case UnaryMinus:    return 10;
    }
    UNREACHABLE;
}


bool Operator::is_right_associative() const
{
    return op == Exp;
}


const char* Operator::to_cstr() const
{
    switch (op) {
        case Operator::Undefined:   return "<undef>";
        case Operator::LogicalOr:   return "||";
        case Operator::LogicalAnd:  return "&&";
        case Operator::Equal:       return "==";
        case Operator::NotEqual:    return "!=";
        case Operator::LessEqual:   return "<=";
        case Operator::GreaterEqual:return ">=";
        case Operator::LessThan:    return "<";
        case Operator::GreaterThan: return ">";
        case Operator::BitwiseOr:   return "|";
        case Operator::BitwiseAnd:  return "&";
        case Operator::BitwiseXor:  return "^";
        case Operator::ShiftLeft:   return "<<";
        case Operator::ShiftRight:  return ">>";
        case Operator::Add:         return "+";
        case Operator::Sub:         return "-";
        case Operator::Mul:         return "*";
        case Operator::Div:         return "/";
        case Operator::Mod:         return "%";
        case Operator::Exp:         return "**";
        case Operator::LogicalNot:  return "-";
        case Operator::BitwiseNot:  return "~";
        case Operator::UnaryPlus:   return "+";
        case Operator::UnaryMinus:  return "-";
    }
    UNREACHABLE;
}


struct StreamOptions {
    bool enable_tree : 1;
    unsigned level : 5;
};

static StreamOptions& stream_options(std::ostream& os) {
    static int idx = std::ios_base::xalloc();
    return reinterpret_cast<StreamOptions&>(os.iword(idx));
}


std::ostream& operator<<(std::ostream& os, const Integer& v)
{
    if (stream_options(os).enable_tree) {
        return os << put_indent << "Int32(Expression) " << v.value << std::endl;
    } else {
        return os << v.value << ":Int32";
    }
}

std::ostream& operator<<(std::ostream& os, const Float& v)
{
    if (stream_options(os).enable_tree) {
        return os << put_indent << "Float32(Expression) " << v.value << std::endl;
    } else {
        return os << v.value << ":Float32";
    }
}

std::ostream& operator<<(std::ostream& os, const String& v)
{
    if (stream_options(os).enable_tree) {
        return os << put_indent << "String(Expression) " << v.value << std::endl;
    } else {
        return os << '"' << v.value << "\":String";
    }
}

std::ostream& operator<<(std::ostream& os, const Tuple& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Tuple(Expression)" << std::endl;
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

std::ostream& operator<<(std::ostream& os, const Variable& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Variable" << std::endl;
        os << more_indent << v.identifier;
        if (v.type)
            os << *v.type;
        return os << less_indent;
    } else {
        if (!v.identifier.name.empty()) {
            os << v.identifier;
            if (v.type)
                os << ':';
        }
        if (v.type)
            os << ':' << *v.type;
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
        return os << std::endl;
    } else {
        return os << v.name;
    }
}


std::ostream& operator<<(std::ostream& os, const Type& v)
{
    ast::DumpVisitor visitor(os);
    v.apply(visitor);
    return os;
}


std::ostream& operator<<(std::ostream& os, const TypeName& v)
{
    if (stream_options(os).enable_tree) {
        if (!v.name.empty())
            return os << put_indent << "TypeName(Type) " << v.name << std::endl;
        else
            return os;
    } else {
        return os << v.name;
    }
}


std::ostream& operator<<(std::ostream& os, const FunctionType& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "FunctionType(Type)" << std::endl;
        os << more_indent;
        for (const auto& prm : v.params)
            os << prm;
        if (v.result_type)
            os << *v.result_type;
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


std::ostream& operator<<(std::ostream& os, const Call& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Call(Expression)" << std::endl;
        os << more_indent << v.identifier;
        for (const auto& arg : v.args) {
            os << *arg;
        }
        return os << less_indent;
    } else {
        os << v.identifier;
        for (const auto& arg : v.args) {
            os << ' ' << *arg;
        }
        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const OpCall& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "OpCall(Expression)" << std::endl;
        os << more_indent << v.op;
        if (!v.identifier.name.empty())
            os << v.identifier;
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
        os << put_indent << "Condition(Expression)" << std::endl;
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
            << " [L" << v.precedence() << "]" << std::endl;
    } else {
        return os << v.to_cstr();
    }
}


std::ostream& operator<<(std::ostream& os, const Function& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Function(Expression)" << std::endl;
        return os << more_indent << v.type << v.body << less_indent;
    } else {
        return os << "(" << v.type << "{" << v.body << "})";
    }
}


std::ostream& operator<<(std::ostream& os, const Expression& v)
{
    ast::DumpVisitor visitor(os);
    v.apply(visitor);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Definition& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Definition(Statement)" << std::endl;
        return os << more_indent << v.identifier << *v.expression << less_indent;
    } else {
        return os << "/*def*/ " << v.identifier << " = (" << *v.expression << ");";
    }
}

std::ostream& operator<<(std::ostream& os, const Invocation& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Invocation(Statement)" << std::endl;
        return os << more_indent << *v.expression << less_indent;
    } else {
        return os << *v.expression;
    }
}


std::ostream& operator<<(std::ostream& os, const Return& v)
{
    if (stream_options(os).enable_tree) {
        os << put_indent << "Return(Statement)" << std::endl;
        return os << more_indent << *v.expression << less_indent;
    } else {
        return os << *v.expression;
    }
}


std::ostream& operator<<(std::ostream& os, const Block& v)
{
    ast::DumpVisitor visitor(os);
    if (stream_options(os).enable_tree) {
        os << put_indent << "Block";
        if (v.symtab != nullptr)
            os << " [" << std::hex << v.symtab << std::dec << "]";
        os << std::endl;
        os << more_indent;
        for (const auto& stmt : v.statements)
            stmt->apply(visitor);
        return os << less_indent;
    } else {
        for (const auto& stmt : v.statements) {
            stmt->apply(visitor);
            if (&stmt != &v.statements.back())
                os << ";" << std::endl;
        }
        return os;
    }
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

} // namespace ast


std::ostream& operator<<(std::ostream& os, const AST& ast)
{
    if (ast::stream_options(os).enable_tree) {
        os << ast::put_indent << "Module" << std::endl;
        return os << ast::more_indent << ast.body << ast::less_indent;
    } else {
        return os << ast.body;
    }
}


} // namespace xci::script
