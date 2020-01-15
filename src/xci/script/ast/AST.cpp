// AST.cpp created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "AST.h"
#include <xci/script/Error.h>
#include <xci/compat/macros.h>

#include <range/v3/view/reverse.hpp>

#include <string>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cassert>

namespace xci::script::ast {


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
        void visit(Class&) override {}
        void visit(Instance&) override {}

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
    statements.push_back(std::make_unique<Return>(std::make_unique<Reference>(Identifier{"void"})));
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
                default:    assert(c2 == 0); op = prefix? LogicalNot : Subscript; break;
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
        case Subscript:     return 10;
        case DotCall:       return 11;
        case LogicalNot:    return 12;
        case BitwiseNot:    return 12;
        case UnaryPlus:     return 12;
        case UnaryMinus:    return 12;
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
        case Operator::Subscript:   return "!";
        case Operator::DotCall:     return ".";
    }
    UNREACHABLE;
}


} // namespace xci::script::ast
