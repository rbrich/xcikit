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

using ranges::cpp20::views::reverse;


template <class T>
auto copy_vector(const std::vector<T>& s)
{
    std::vector<T> r;
    r.reserve(s.size());
    for (const auto& item : s)
        r.emplace_back(copy(item));
    return r;
}


template <class T>
auto copy_ptr_vector(const std::vector<std::unique_ptr<T>>& s)
{
    std::vector<std::unique_ptr<T>> r;
    r.reserve(s.size());
    for (const auto& item : s)
        r.emplace_back(item->make_copy());
    return r;
}


std::unique_ptr<ast::Expression> Function::make_copy() const
{
    auto r = std::make_unique<Function>();
    r->type = copy(type);
    r->source_info = source_info;
    r->body = copy(body);
    r->index = index;
    return r;
}


std::unique_ptr<ast::Expression> Condition::make_copy() const
{
    auto r = std::make_unique<Condition>();
    r->source_info = source_info;
    r->cond = cond->make_copy();
    r->then_expr = then_expr->make_copy();
    r->else_expr = else_expr->make_copy();
    return r;
}


std::unique_ptr<ast::Statement> Definition::make_copy() const
{
    auto r = std::make_unique<Definition>();
    copy_to(*r);
    return r;
}


void Definition::copy_to(Definition& r) const
{
    r.variable = copy(variable);
    r.expression = expression->make_copy();
}


std::unique_ptr<ast::Statement> Invocation::make_copy() const
{
    auto r = std::make_unique<Invocation>();
    r->expression = expression->make_copy();
    r->type_index = type_index;
    return r;
}


std::unique_ptr<ast::Statement> Return::make_copy() const
{
    return std::make_unique<Return>(expression->make_copy());
}


std::unique_ptr<ast::Statement> Class::make_copy() const
{
    auto r = std::make_unique<Class>();
    r->class_name = class_name;
    r->type_var = type_var;
    r->context = context;
    r->defs.reserve(defs.size());
    for (const auto& d : defs) {
        r->defs.emplace_back();
        d.copy_to(r->defs.back());
    }
    r->index = index;
    r->symtab = symtab;
    return r;
}

std::unique_ptr<ast::Statement> Instance::make_copy() const
{
    auto r = std::make_unique<Instance>();
    r->class_name = class_name;
    r->type_inst = type_inst->make_copy();
    r->context = context;
    for (const auto& d : defs) {
        r->defs.emplace_back();
        d.copy_to(r->defs.back());
    }
    r->index = index;
    r->symtab = symtab;
    return r;
}


std::unique_ptr<ast::Expression> Reference::make_copy() const
{
    auto r = std::make_unique<Reference>();
    r->source_info = source_info;
    r->identifier = identifier;
    r->chain = chain;
    r->module = module;
    r->index = index;
    return r;
}


void Call::copy_to(Call& r) const
{
    if (callable)
        r.callable = callable->make_copy();
    r.source_info = source_info;
    r.args = copy_ptr_vector(args);
    r.wrapped_execs = wrapped_execs;
    r.partial_args = partial_args;
    r.partial_index = partial_index;
}


std::unique_ptr<ast::Expression> Call::make_copy() const
{
    return std::make_unique<Call>(copy(*this));
}


std::unique_ptr<ast::Expression> OpCall::make_copy() const
{
    auto r = std::make_unique<OpCall>();
    Call::copy_to(*r);
    r->op = op;
    return r;
}


std::unique_ptr<ast::Type> ListType::make_copy() const
{
    auto r = std::make_unique<ListType>();
    r->elem_type = copy(elem_type);
    return r;
}


std::unique_ptr<ast::Type> FunctionType::make_copy() const
{
    return std::make_unique<FunctionType>(copy(*this));
}


void FunctionType::copy_to(FunctionType& r) const
{
    r.params = copy_vector(params);
    r.result_type = copy(result_type);
    r.context = context;
}


std::unique_ptr<ast::Expression> Tuple::make_copy() const
{
    auto r = std::make_unique<Tuple>();
    r->source_info = source_info;
    r->items = copy_ptr_vector(items);
    return r;
}


std::unique_ptr<ast::Expression> List::make_copy() const
{
    auto r = std::make_unique<List>();
    r->source_info = source_info;
    r->items = copy_ptr_vector(items);
    r->item_size = item_size;
    return r;
}


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
    for (auto& stmt : reverse(statements)) {
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


std::unique_ptr<Type> copy(const std::unique_ptr<Type>& v)
{
    if (v)
        return v->make_copy();
    return {};
}


Block copy(const Block& v)
{
    return {copy_ptr_vector(v.statements)};
}


} // namespace xci::script::ast
