// AST.h created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_H
#define XCI_SCRIPT_AST_H

#include <xci/script/SymbolTable.h>
#include <xci/script/SourceInfo.h>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <utility>

namespace xci::script {

class Function;
class Module;


namespace ast {


struct Statement;
struct Block;

struct Definition;
struct Invocation;
struct Return;
struct Class;
struct Instance;

struct Integer;
struct Float;
struct Char;
struct String;
struct Braced;
struct Tuple;
struct List;
struct Reference;
struct Call;
struct OpCall;
struct Condition;
struct Function;

struct TypeName;
struct FunctionType;
struct ListType;


class ConstVisitor {
public:
    // statement
    virtual void visit(const Definition&) = 0;
    virtual void visit(const Invocation&) = 0;
    virtual void visit(const Return&) = 0;
    virtual void visit(const Class&) = 0;
    virtual void visit(const Instance&) = 0;
    // expression
    virtual void visit(const Integer&) = 0;
    virtual void visit(const Float&) = 0;
    virtual void visit(const Char&) = 0;
    virtual void visit(const String&) = 0;
    virtual void visit(const Braced&) = 0;
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
    virtual void visit(Class&) = 0;
    virtual void visit(Instance&) = 0;
    // expression
    virtual void visit(Integer&) = 0;
    virtual void visit(Float&) = 0;
    virtual void visit(Char&) = 0;
    virtual void visit(String&) = 0;
    virtual void visit(Braced&) = 0;
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
    void visit(Char&) final {}
    void visit(String&) final {}
    void visit(Braced&) final {}
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
    void visit(Class&) final {}
    void visit(Instance&) final {}
    // skip expression visits
    void visit(Integer&) final {}
    void visit(Float&) final {}
    void visit(Char&) final {}
    void visit(String&) final {}
    void visit(Tuple&) final {}
    void visit(Braced&) final {}
    void visit(List&) final {}
    void visit(Reference&) final {}
    void visit(Call&) final {}
    void visit(OpCall&) final {}
    void visit(Condition&) final {}
    void visit(Function&) final {}
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
    virtual std::unique_ptr<ast::Type> make_copy() const = 0;
};


struct TypeName: public Type {
    TypeName() = default;
    explicit TypeName(std::string  s) : name(std::move(s)) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Type> make_copy() const override { return std::make_unique<TypeName>(*this); };

    std::string name;

    // resolved symbol:
    SymbolPointer symbol;
};


struct ListType: public Type {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Type> make_copy() const override;

    std::unique_ptr<Type> elem_type;
};


struct Parameter {
    Identifier identifier;  // optional
    std::unique_ptr<Type> type;
};


struct TypeConstraint {
    TypeName type_class;
    TypeName type_name;
};

struct FunctionType: public Type {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Type> make_copy() const override;
    void copy_to(FunctionType& r) const;

    std::vector<Parameter> params;
    std::unique_ptr<Type> result_type;
    std::vector<TypeConstraint> context;
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
    virtual std::unique_ptr<ast::Expression> make_copy() const = 0;

    SourceInfo source_info;

    // set when this expression is direct child of a Definition
    Definition* definition = nullptr;
};

struct Integer: public Expression {
    explicit Integer(int32_t v) : value(v) {}
    explicit Integer(const std::string& s);
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override { return std::make_unique<Integer>(*this); };

    int32_t value;
};

struct Float: public Expression {
    explicit Float(float v) : value(v) {}
    explicit Float(const std::string& s);
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override { return std::make_unique<Float>(*this); };

    float value;
};

struct Char: public Expression {
  explicit Char(char32_t c) : value(c) {}
  explicit Char(std::string_view sv);
  void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
  void apply(Visitor& visitor) override { visitor.visit(*this); }
  std::unique_ptr<ast::Expression> make_copy() const override { return std::make_unique<Char>(*this); };

  char32_t value;
};

struct String: public Expression {
    explicit String(std::string s) : value(std::move(s)) {}
    explicit String(std::string_view sv) : value(sv) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override { return std::make_unique<String>(*this); };

    std::string value;
};

struct Braced: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    std::unique_ptr<Expression> expression;
};

struct Tuple: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    std::vector<std::unique_ptr<Expression>> items;
};

struct List: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    std::vector<std::unique_ptr<Expression>> items;
    size_t item_size = 0;
};

// variable reference
struct Reference: public Expression {
    Reference() = default;
    explicit Reference(Identifier&& s) : identifier(std::move(s)) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    Identifier identifier;

    // resolved Method:
    SymbolPointer chain;  // tip of chain of Instances in case of Method
    Module* module = nullptr;   // module with instance function
    Index index = no_index;     // index of (instance) function in module
};

struct Call: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;
    void copy_to(Call& r) const;

    std::unique_ptr<Expression> callable;
    std::vector<std::unique_ptr<Expression>> args;

    // resolved:
    size_t wrapped_execs = 0;
    size_t partial_args = 0;
    Index partial_index = no_index;
};

struct Operator {
    enum Op {
        Undefined,
        // binary
        Comma,          // x, y
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
        DotCall,        // x .f y
        // unary
        LogicalNot,     // !x
        BitwiseNot,     // ~x
        UnaryPlus,      // +x
        UnaryMinus,     // -x
    };

    Operator() = default;
    Operator(Op op) : op(op) {}
    explicit Operator(const std::string& s, bool prefix=false);
    const char* to_cstr() const;
    int precedence() const;
    bool is_right_associative() const;
    bool is_undefined() const { return op == Undefined; }
    bool is_dot_call() const { return op == DotCall; }
    bool is_comma() const { return op == Comma; }
    bool operator==(const Operator& rhs) const { return op == rhs.op; }
    bool operator!=(const Operator& rhs) const { return op != rhs.op; }

    Op op = Undefined;
};


// infix operators -> mirrors FunCall
struct OpCall: public Call {
    OpCall() = default;
    explicit OpCall(Operator::Op op) : op(op) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    Operator op;
    std::unique_ptr<OpCall> right_tmp;  // used during parsing, cleared when finished
};


struct Function: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

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
    std::unique_ptr<ast::Expression> make_copy() const override;

    std::unique_ptr<Expression> cond;
    std::unique_ptr<Expression> then_expr;
    std::unique_ptr<Expression> else_expr;
};


struct Statement {
    virtual ~Statement() = default;
    virtual void apply(ConstVisitor& visitor) const = 0;
    virtual void apply(Visitor& visitor) = 0;
    virtual std::unique_ptr<ast::Statement> make_copy() const = 0;
};

struct Definition: public Statement {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Statement> make_copy() const override;
    void copy_to(Definition& r) const;
    SymbolPointer& symbol() { return variable.identifier.symbol; }

    Variable variable;
    std::unique_ptr<Expression> expression;
};

struct Invocation: public Statement {
    Invocation(std::unique_ptr<Expression>&& expr) : expression(std::move(expr)) {}

    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Statement> make_copy() const override;

    std::unique_ptr<Expression> expression;

    // resolved:
    Index type_index = no_index;
};

struct Return: public Statement {
    explicit Return(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Statement> make_copy() const override;

    std::unique_ptr<Expression> expression;
};


struct Class: public Statement {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Statement> make_copy() const override;

    TypeName class_name;
    TypeName type_var;
    std::vector<TypeConstraint> context;
    std::vector<ast::Definition> defs;  // functions in class

    // resolved:
    Index index = no_index;
    SymbolTable* symtab = nullptr;
};


struct Instance: public Statement {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Statement> make_copy() const override;

    TypeName class_name;
    std::unique_ptr<Type> type_inst;
    std::vector<TypeConstraint> context;
    std::vector<ast::Definition> defs;  // functions in class

    // resolved:
    Index index = no_index;
    SymbolTable* symtab = nullptr;
};


struct Module {
    ast::Block body;
};


template <class T> T copy(const T& v) { T r; v.copy_to(r); return r; }
std::unique_ptr<Type> copy(const std::unique_ptr<Type>& v);
inline Variable copy(const Variable& v) { return {v.identifier, copy(v.type)}; }
inline Parameter copy(const Parameter& v) { return {v.identifier, copy(v.type)}; }
Block copy(const Block& v);


} // namespace ast
} // namespace xci::script

#endif // include guard
