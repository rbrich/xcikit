#include <utility>

// AST.h created on 2019-05-15, part of XCI toolkit
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

#ifndef XCIKIT_AST_H
#define XCIKIT_AST_H

#include "SymbolTable.h"
#include "Error.h"
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <utility>

namespace xci::script {

class Function;
class Module;


namespace ast {


struct Definition;
struct Invocation;
struct Return;
struct Integer;
struct Float;
struct String;
struct Tuple;
struct List;
struct Reference;
struct Call;
struct OpCall;
struct Condition;
struct Function;
struct Block;
struct Statement;
struct TypeName;
struct FunctionType;
struct ListType;


class ConstVisitor {
public:
    // statement
    virtual void visit(const Definition&) = 0;
    virtual void visit(const Invocation&) = 0;
    virtual void visit(const Return&) = 0;
    // expression
    virtual void visit(const Integer&) = 0;
    virtual void visit(const Float&) = 0;
    virtual void visit(const String&) = 0;
    virtual void visit(const Tuple&) = 0;
    virtual void visit(const List&) = 0;
    virtual void visit(const Reference&) = 0;
    virtual void visit(const Call&) = 0;
    virtual void visit(const OpCall&) = 0;
    virtual void visit(const Condition&) = 0;
    virtual void visit(const Function&) = 0;
    // type
    virtual void visit(const TypeName&) = 0;
    virtual void visit(const FunctionType&) = 0;
    virtual void visit(const ListType&) = 0;
};

class Visitor {
public:
    // statement
    virtual void visit(Definition&) = 0;
    virtual void visit(Invocation&) = 0;
    virtual void visit(Return&) = 0;
    // expression
    virtual void visit(Integer&) = 0;
    virtual void visit(Float&) = 0;
    virtual void visit(String&) = 0;
    virtual void visit(Tuple&) = 0;
    virtual void visit(List&) = 0;
    virtual void visit(Reference&) = 0;
    virtual void visit(Call&) = 0;
    virtual void visit(OpCall&) = 0;
    virtual void visit(Condition&) = 0;
    virtual void visit(Function&) = 0;
    // type
    virtual void visit(TypeName&) = 0;
    virtual void visit(FunctionType&) = 0;
    virtual void visit(ListType&) = 0;
};


// Inherit this and add `using StatementVisitor::visit;` to skip other visits
class StatementVisitor: public Visitor {
public:
    // skip expression visits
    void visit(Integer&) final {}
    void visit(Float&) final {}
    void visit(String&) final {}
    void visit(Tuple&) final {}
    void visit(List&) final {}
    void visit(Reference&) final {}
    void visit(Call&) final {}
    void visit(OpCall&) final {}
    void visit(Condition&) final {}
    void visit(Function&) final {}
    // skip type visits
    void visit(TypeName&) final {}
    void visit(FunctionType&) final {}
    void visit(ListType&) final {}
};


// Inherit this and add `using TypeVisitor::visit;` to skip other visits
class TypeVisitor: public Visitor {
public:
    // statement
    void visit(Definition&) final {}
    void visit(Invocation&) final {}
    void visit(Return&) final {}
    // skip expression visits
    void visit(Integer&) final {}
    void visit(Float&) final {}
    void visit(String&) final {}
    void visit(Tuple&) final {}
    void visit(List&) final {}
    void visit(Reference&) final {}
    void visit(Call&) final {}
    void visit(OpCall&) final {}
    void visit(Condition&) final {}
    void visit(Function&) final {}
};


class BlockProcessor {
public:
    virtual ~BlockProcessor() = default;
    virtual void process_block(script::Function& func, const ast::Block& block) = 0;
};


struct Identifier {
    Identifier() = default;
    explicit Identifier(std::string s) : name(std::move(s)) {}
    explicit operator bool() const { return !name.empty(); }
    std::string name;

    // resolved symbol:
    SymbolPointer symbol;
};


struct Type {
    virtual ~Type() = default;
    virtual void apply(ConstVisitor& visitor) const = 0;
    virtual void apply(Visitor& visitor) = 0;
};


struct TypeName: public Type {
    explicit TypeName(std::string  s) : name(std::move(s)) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::string name;
};


struct ListType: public Type {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<Type> elem_type;
};


struct Parameter {
    Identifier identifier;  // optional
    std::unique_ptr<Type> type;
};


struct FunctionType: public Type {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::vector<Parameter> params;
    std::unique_ptr<Type> result_type;
};


struct Variable {
    Identifier identifier;  // required
    std::unique_ptr<Type> type;
};


struct Block {
    // finish block - convert last Invocation into ReturnStatement
    // (no Invocation -> throw error)
    void finish();

    std::vector<std::unique_ptr<ast::Statement>> statements;

    // resolved:
    SymbolTable* symtab = nullptr;
};


struct Expression {
    virtual ~Expression() = default;
    virtual void apply(ConstVisitor& visitor) const = 0;
    virtual void apply(Visitor& visitor) = 0;

    SourceInfo source_info;
};

struct Integer: public Expression {
    explicit Integer(int32_t v) : value(v) {}
    explicit Integer(const std::string& s);
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    int32_t value;
};

struct Float: public Expression {
    explicit Float(float v) : value(v) {}
    explicit Float(const std::string& s);
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    float value;
};

struct String: public Expression {
    explicit String(std::string s) : value(std::move(s)) {}
    explicit String(std::string_view sv) : value(sv) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::string value;
};

struct Tuple: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::vector<std::unique_ptr<Expression>> items;
};

struct List: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::vector<std::unique_ptr<Expression>> items;
    size_t item_size = 0;
};

// variable reference
struct Reference: public Expression {
    Reference() = default;
    explicit Reference(Identifier&& s) : identifier(std::move(s)) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }

    Identifier identifier;
};

struct Call: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<Expression> callable;
    std::vector<std::unique_ptr<Expression>> args;

    // resolved:
    size_t wrapped_execs = 0;
};

struct Operator {
    enum Op {
        Undefined,
        // binary
        LogicalOr,      // x || y
        LogicalAnd,     // x && y
        Equal,          // x == y
        NotEqual,       // x != y
        LessEqual,      // x <= y
        GreaterEqual,   // x >= y
        LessThan,       // x < y
        GreaterThan,    // x > y
        BitwiseOr,      // x | y
        BitwiseAnd,     // x & y
        BitwiseXor,     // x ^ y
        ShiftLeft,      // x << y
        ShiftRight,     // x >> y
        Add,            // x + y
        Sub,            // x - y
        Mul,            // x * y
        Div,            // x / y
        Mod,            // x % y
        Exp,            // x ** y
        Subscript,      // x ! y
        // unary
        LogicalNot,     // !x
        BitwiseNot,     // ~x
        UnaryPlus,      // +x
        UnaryMinus,     // -x
    };

    Operator() = default;
    explicit Operator(Op op) : op(op) {}
    explicit Operator(const std::string& s, bool prefix=false);
    const char* to_cstr() const;
    int precedence() const;
    bool is_right_associative() const;
    bool is_undefined() const { return op == Undefined; }
    bool operator==(const Operator& rhs) const { return op == rhs.op; }
    bool operator!=(const Operator& rhs) const { return op != rhs.op; }

    Op op = Undefined;
};


// infix operators -> mirrors FunCall
struct OpCall: public Call {
    OpCall() = default;
    OpCall(Operator::Op op) : op(op) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    Operator op;
    std::unique_ptr<OpCall> right_tmp;  // used during parsing, cleared when finished
};


struct Function: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    FunctionType type;
    Block body;

    // resolved:
    Index index = no_index;
};


// if - then - else
struct Condition: public Expression {
    Condition() = default;
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<Expression> cond;
    std::unique_ptr<Expression> then_expr;
    std::unique_ptr<Expression> else_expr;
};


struct Statement {
    virtual ~Statement() = default;
    virtual void apply(ConstVisitor& visitor) const = 0;
    virtual void apply(Visitor& visitor) = 0;
};

struct Definition: public Statement {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    Variable variable;
    std::unique_ptr<Expression> expression;
};

struct Invocation: public Statement {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<Expression> expression;

    // resolved:
    Index type_index = no_index;
};

struct Return: public Statement {
    explicit Return(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<Expression> expression;
};


std::ostream& operator<<(std::ostream& os, const Integer& v);
std::ostream& operator<<(std::ostream& os, const Float& v);
std::ostream& operator<<(std::ostream& os, const String& v);
std::ostream& operator<<(std::ostream& os, const Tuple& v);
std::ostream& operator<<(std::ostream& os, const List& v);
std::ostream& operator<<(std::ostream& os, const Variable& v);
std::ostream& operator<<(std::ostream& os, const Parameter& v);
std::ostream& operator<<(std::ostream& os, const Identifier& v);
std::ostream& operator<<(std::ostream& os, const Type& v);
std::ostream& operator<<(std::ostream& os, const TypeName& v);
std::ostream& operator<<(std::ostream& os, const FunctionType& v);
std::ostream& operator<<(std::ostream& os, const ListType& v);
std::ostream& operator<<(std::ostream& os, const Reference& v);
std::ostream& operator<<(std::ostream& os, const Call& v);
std::ostream& operator<<(std::ostream& os, const OpCall& v);
std::ostream& operator<<(std::ostream& os, const Condition& v);
std::ostream& operator<<(std::ostream& os, const Operator& v);
std::ostream& operator<<(std::ostream& os, const Function& v);
std::ostream& operator<<(std::ostream& os, const Expression& v);
std::ostream& operator<<(std::ostream& os, const Definition& v);
std::ostream& operator<<(std::ostream& os, const Invocation& v);
std::ostream& operator<<(std::ostream& os, const Return& v);
std::ostream& operator<<(std::ostream& os, const Block& v);

// stream manipulators
std::ostream& dump_tree(std::ostream& os);
std::ostream& put_indent(std::ostream& os);
std::ostream& more_indent(std::ostream& os);
std::ostream& less_indent(std::ostream& os);

} // namespace ast


struct AST {
    ast::Block body;
};


std::ostream& operator<<(std::ostream& os, const AST& ast);


} // namespace xci::script


#endif // include guard
