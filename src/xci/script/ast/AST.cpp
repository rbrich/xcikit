// AST.cpp created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "AST.h"
#include <xci/script/Error.h>
#include <xci/compat/macros.h>
#include <xci/core/string.h>

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
    Expression::copy_to(*r);
    type.copy_to(r->type);
    r->body = copy(body);
    r->index = index;
    return r;
}


Condition::IfThen copy(const Condition::IfThen& s)
{
    return {s.first->make_copy(), s.second->make_copy()};
}


std::unique_ptr<ast::Expression> Condition::make_copy() const
{
    auto r = std::make_unique<Condition>();
    Expression::copy_to(*r);
    r->if_then_expr = copy_vector(if_then_expr);
    r->else_expr = else_expr->make_copy();
    return r;
}


std::unique_ptr<ast::Expression> WithContext::make_copy() const
{
    auto r = std::make_unique<WithContext>();
    Expression::copy_to(*r);
    r->context = context->make_copy();
    r->expression = expression->make_copy();
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
    auto r = std::make_unique<Invocation>(expression->make_copy());
    r->type_id = type_id;
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
    r->type_vars = type_vars;
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
    for (const auto& t : type_inst)
        r->type_inst.push_back(t->make_copy());
    r->context = context;
    for (const auto& d : defs) {
        r->defs.emplace_back();
        d.copy_to(r->defs.back());
    }
    r->index = index;
    r->symtab = symtab;
    return r;
}


std::unique_ptr<ast::Statement> TypeDef::make_copy() const
{
    auto r = std::make_unique<TypeDef>();
    r->type_name = type_name;
    r->type = type->make_copy();
    return r;
}


std::unique_ptr<ast::Statement> TypeAlias::make_copy() const
{
    auto r = std::make_unique<TypeAlias>();
    r->type_name = type_name;
    r->type = type->make_copy();
    return r;
}


void Expression::copy_to(Expression& r) const
{
    r.source_loc = source_loc;
    r.definition = definition;
}


std::unique_ptr<ast::Expression> Reference::make_copy() const
{
    auto r = std::make_unique<Reference>();
    Reference::copy_to(*r);
    return r;
}


void Reference::copy_to(Reference& r) const
{
    Expression::copy_to(r);
    r.identifier = identifier;
    if (type_arg)
        r.type_arg = type_arg->make_copy();
    r.chain = chain;
    r.module = module;
    r.index = index;
}


void Call::copy_to(Call& r) const
{
    Expression::copy_to(r);
    if (callable)
        r.callable = callable->make_copy();
    r.args = copy_ptr_vector(args);
    r.wrapped_execs = wrapped_execs;
    r.partial_args = partial_args;
    r.partial_index = partial_index;
    r.intrinsic = intrinsic;
}


std::unique_ptr<ast::Expression> Call::make_copy() const
{
    return pcopy(*this);
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


std::unique_ptr<ast::Type> TupleType::make_copy() const
{
    auto r = std::make_unique<TupleType>();
    for (const auto& t : subtypes)
        r->subtypes.push_back(t->make_copy());
    return r;
}


std::unique_ptr<ast::Type> StructType::make_copy() const
{
    auto r = std::make_unique<StructType>();
    r->subtypes = copy_vector(subtypes);
    return r;
}


std::unique_ptr<ast::Type> FunctionType::make_copy() const
{
    return pcopy(*this);
}


void FunctionType::copy_to(FunctionType& r) const
{
    r.type_params = type_params;
    r.params = copy_vector(params);
    r.result_type = copy(result_type);
    r.context = context;
}


std::unique_ptr<ast::Expression> Literal::make_copy() const
{
    auto r = std::make_unique<Literal>(value);
    Expression::copy_to(*r);
    return r;
}


std::unique_ptr<ast::Expression> Parenthesized::make_copy() const
{
    auto r = std::make_unique<Parenthesized>();
    Expression::copy_to(*r);
    r->expression = expression->make_copy();
    return r;
}


std::unique_ptr<ast::Expression> Tuple::make_copy() const
{
    auto r = std::make_unique<Tuple>();
    Expression::copy_to(*r);
    r->items = copy_ptr_vector(items);
    return r;
}


std::unique_ptr<ast::Expression> List::make_copy() const
{
    auto r = std::make_unique<List>();
    Expression::copy_to(*r);
    r->items = copy_ptr_vector(items);
    r->elem_type_id = elem_type_id;
    return r;
}


std::unique_ptr<ast::Expression> StructInit::make_copy() const
{
    auto r = std::make_unique<StructInit>();
    Expression::copy_to(*r);

    r->items.reserve(items.size());
    for (const auto& item : items)
        r->items.emplace_back(item.first, item.second->make_copy());

    return r;
}


std::unique_ptr<ast::Expression> Cast::make_copy() const
{
    auto r = std::make_unique<Cast>();
    Expression::copy_to(*r);
    r->expression = expression->make_copy();
    r->type = type->make_copy();
    if (cast_function) {
        r->cast_function = std::make_unique<Reference>();
        cast_function->copy_to(*r->cast_function);
    }
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
        void visit(TypeDef&) override {}
        void visit(TypeAlias&) override {}

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
}


Operator::Operator(std::string_view s, bool prefix)
{
    assert(!s.empty());
    char c1 = s[0];
    char c2 = s.size() >= 2 ? s[1] : '\0';
    switch (c1) {
        case ',':    op = Comma; break;
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
        case Comma:         return 1;
        case LogicalOr:     return 2;
        case LogicalAnd:    return 3;
        case Equal:         return 4;
        case NotEqual:      return 4;
        case LessEqual:     return 4;
        case GreaterEqual:  return 4;
        case LessThan:      return 4;
        case GreaterThan:   return 4;
        case BitwiseOr:     return 5;
        case BitwiseXor:    return 5;
        case BitwiseAnd:    return 6;
        case ShiftLeft:     return 7;
        case ShiftRight:    return 7;
        case Add:           return 8;
        case Sub:           return 8;
        case Mul:           return 9;
        case Div:           return 9;
        case Mod:           return 9;
        case Exp:           return 10;
        case Subscript:     return 11;
        case DotCall:       return 12;
        case LogicalNot:    return 13;
        case BitwiseNot:    return 13;
        case UnaryPlus:     return 13;
        case UnaryMinus:    return 13;
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
        case Operator::Comma:       return ",";
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
        case Operator::LogicalNot:  return "!";
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
