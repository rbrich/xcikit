// AST.h created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_H
#define XCI_SCRIPT_AST_H

#include <xci/script/SymbolTable.h>
#include <xci/script/Source.h>
#include <xci/script/Value.h>
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
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
struct TypeDef;
struct TypeAlias;

struct Cast;
struct Literal;
struct Bracketed;
struct Tuple;
struct List;
struct StructInit;
struct Reference;
struct Call;
struct OpCall;
struct Condition;
struct WithContext;
struct Function;

struct TypeName;
struct FunctionType;
struct ListType;
struct TupleType;


class ConstVisitor {
public:
    // statement
    virtual void visit(const Definition&) = 0;
    virtual void visit(const Invocation&) = 0;
    virtual void visit(const Return&) = 0;
    virtual void visit(const Class&) = 0;
    virtual void visit(const Instance&) = 0;
    virtual void visit(const TypeDef&) = 0;
    virtual void visit(const TypeAlias&) = 0;
    // expression
    virtual void visit(const Literal&) = 0;
    virtual void visit(const Bracketed&) = 0;
    virtual void visit(const Tuple&) = 0;
    virtual void visit(const List&) = 0;
    virtual void visit(const StructInit&) = 0;
    virtual void visit(const Reference&) = 0;
    virtual void visit(const Call&) = 0;
    virtual void visit(const OpCall&) = 0;
    virtual void visit(const Condition&) = 0;
    virtual void visit(const WithContext&) = 0;
    virtual void visit(const Function&) = 0;
    virtual void visit(const Cast&) = 0;
    // type
    virtual void visit(const TypeName&) = 0;
    virtual void visit(const FunctionType&) = 0;
    virtual void visit(const ListType&) = 0;
    virtual void visit(const TupleType&) = 0;
};

class Visitor {
public:
    // statement
    virtual void visit(Definition&) = 0;
    virtual void visit(Invocation&) = 0;
    virtual void visit(Return&) = 0;
    virtual void visit(Class&) = 0;
    virtual void visit(Instance&) = 0;
    virtual void visit(TypeDef&) = 0;
    virtual void visit(TypeAlias&) = 0;
    // expression
    virtual void visit(Literal&) = 0;
    virtual void visit(Bracketed&) = 0;
    virtual void visit(Tuple&) = 0;
    virtual void visit(List&) = 0;
    virtual void visit(StructInit&) = 0;
    virtual void visit(Reference&) = 0;
    virtual void visit(Call&) = 0;
    virtual void visit(OpCall&) = 0;
    virtual void visit(Condition&) = 0;
    virtual void visit(WithContext&) = 0;
    virtual void visit(Function&) = 0;
    virtual void visit(Cast&) = 0;
    // type
    virtual void visit(TypeName&) = 0;
    virtual void visit(FunctionType&) = 0;
    virtual void visit(ListType&) = 0;
    virtual void visit(TupleType&) = 0;
};


// Inherit this and add `using StatementVisitor::visit;` to skip other visits
class StatementVisitor: public Visitor {
public:
    // skip expression visits
    void visit(Literal&) final {}
    void visit(Bracketed&) final {}
    void visit(Tuple&) final {}
    void visit(List&) final {}
    void visit(StructInit&) final {}
    void visit(Reference&) final {}
    void visit(Call&) final {}
    void visit(OpCall&) final {}
    void visit(Condition&) final {}
    void visit(WithContext&) final {}
    void visit(Function&) final {}
    void visit(Cast&) final {}
    // skip type visits
    void visit(TypeName&) final {}
    void visit(FunctionType&) final {}
    void visit(ListType&) final {}
    void visit(TupleType&) final {}
};


// Inherit this and add `using TypeVisitor::visit;` to skip other visits
class TypeVisitor: public Visitor {
public:
    // skip statement visits
    void visit(Definition&) final {}
    void visit(Invocation&) final {}
    void visit(Return&) final {}
    void visit(Class&) final {}
    void visit(Instance&) final {}
    void visit(TypeDef&) final {}
    void visit(TypeAlias&) final {}
    // skip expression visits
    void visit(Literal&) final {}
    void visit(Bracketed&) final {}
    void visit(Tuple&) final {}
    void visit(List&) final {}
    void visit(StructInit&) final {}
    void visit(Reference&) final {}
    void visit(Call&) final {}
    void visit(OpCall&) final {}
    void visit(Condition&) final {}
    void visit(WithContext&) final {}
    void visit(Function&) final {}
    void visit(Cast&) final {}
};


// Inherit this and add `using VisitorExclTypes::visit;` to skip other visits
class VisitorExclTypes: public Visitor {
public:
    // skip statement visits regarding types
    void visit(TypeDef&) final {}
    void visit(TypeAlias&) final {}
    // skip type visits
    void visit(TypeName&) final {}
    void visit(FunctionType&) final {}
    void visit(ListType&) final {}
    void visit(TupleType&) final {}
};


struct Identifier {
    Identifier() = default;
    explicit Identifier(std::string s) : name(std::move(s)) {}
    explicit operator bool() const { return !name.empty(); }

    std::string name;
    SourceLocation source_loc;

    // resolved symbol:
    SymbolPointer symbol;
};


// an identifier that doesn't need to be resolved
struct Key {
    Key() = default;
    explicit Key(std::string s) : name(std::move(s)) {}
    explicit Key(Identifier&& ident) : name(std::move(ident.name)), source_loc(ident.source_loc) {}
    explicit operator bool() const { return !name.empty(); }

    std::string name;
    SourceLocation source_loc;
};


struct Type {
    virtual ~Type() = default;
    virtual void apply(ConstVisitor& visitor) const = 0;
    virtual void apply(Visitor& visitor) = 0;
    virtual std::unique_ptr<ast::Type> make_copy() const = 0;

    SourceLocation source_loc;
};


struct TypeName: public Type {
    TypeName() = default;
    explicit TypeName(std::string s) : name(std::move(s)) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Type> make_copy() const override { return std::make_unique<TypeName>(*this); };
    explicit operator bool() const { return !name.empty(); }

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


struct TupleType: public Type {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Type> make_copy() const override;

    std::vector<std::unique_ptr<Type>> subtypes;
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

    std::vector<TypeName> type_params;  // declare type parameters of a generic function: <T,U>
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
    Expression() = default;
    Expression(Expression&&) = default;
    Expression& operator=(Expression&&) = default;
    virtual ~Expression() = default;
    virtual void apply(ConstVisitor& visitor) const = 0;
    virtual void apply(Visitor& visitor) = 0;
    virtual std::unique_ptr<ast::Expression> make_copy() const = 0;

    void copy_to(Expression& r) const;

    SourceLocation source_loc;

    // set when this expression is direct child of a Definition
    Definition* definition = nullptr;
};

struct Literal: public Expression {
    explicit Literal(TypedValue v) : value(std::move(v)) {}
    ~Literal() override { value.decref(); }

    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    TypedValue value;
};

/// An expression in round brackets, e.g. (1 + 2)
struct Bracketed: public Expression {
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
    size_t elem_type_id = 0;
};

// structured initializer, i.e. tuple with identifiers
struct StructInit: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    using Item = std::pair<Key, std::unique_ptr<Expression>>;
    std::vector<Item> items;

    // resolved:
    TypeInfo struct_type;  // used by Compiler to produce tuple in struct order, with defaults filled in
};

// variable reference
struct Reference: public Expression {
    Reference() = default;
    explicit Reference(Identifier&& s) : identifier(std::move(s)) {}
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;
    void copy_to(Reference& r) const;

    Identifier identifier;
    std::unique_ptr<Type> type_arg;  // explicit type argument: e.g. <Int>

    // resolved function/method:
    SymbolPointer chain;  // tip of chain of Instances in case of Method
    Module* module = nullptr;   // module with (instance) function
    Index index = no_index;     // index of (instance) function in module
};

struct Call: public Expression {
    Call() = default;
    Call(Call&& r) = default;
    Call& operator=(Call&&) = default;
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;
    void copy_to(Call& r) const;

    std::unique_ptr<Expression> callable;
    std::vector<std::unique_ptr<Expression>> args;

    // resolved:
    unsigned wrapped_execs = 0;
    unsigned partial_args = 0;
    Index partial_index = no_index;

    bool intrinsic = false;
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
    explicit Operator(std::string_view s, bool prefix=false);
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
    OpCall(OpCall&&) = default;
    OpCall& operator=(OpCall&&) = default;
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
    size_t call_args = 0;  // number of args if the function is inside Call
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


// with <expr:Context> <expr>
struct WithContext: public Expression {
    WithContext() = default;
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    std::unique_ptr<Expression> context;
    std::unique_ptr<Expression> expression;

    // resolved:
    Reference enter_function;
    Reference leave_function;
    TypeInfo expression_type;
    TypeInfo leave_type;   // enter function returns it, leave function consumes it
};


struct Cast: public Expression {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Expression> make_copy() const override;

    std::unique_ptr<Expression> expression;
    std::unique_ptr<Type> type;
    std::unique_ptr<Reference> cast_function;  // none for cast to Void or to same type

    // resolved:
    TypeInfo to_type;    // resolved Type (cast to)
    TypeInfo from_type;  // resolved type of the expression (cast from)
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
    explicit Invocation(std::unique_ptr<Expression>&& expr) : expression(std::move(expr)) {}

    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Statement> make_copy() const override;

    std::unique_ptr<Expression> expression;

    // resolved:
    Index type_id = no_index;
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
    std::vector<TypeName> type_vars;
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
    std::vector<std::unique_ptr<Type>> type_inst;
    std::vector<TypeConstraint> context;
    std::vector<ast::Definition> defs;  // functions in class

    // resolved:
    Index index = no_index;
    SymbolTable* symtab = nullptr;
};


struct TypeDef: public Statement {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Statement> make_copy() const override;

    TypeName type_name;
    std::unique_ptr<Type> type;
};


struct TypeAlias: public Statement {
    void apply(ConstVisitor& visitor) const override { visitor.visit(*this); }
    void apply(Visitor& visitor) override { visitor.visit(*this); }
    std::unique_ptr<ast::Statement> make_copy() const override;

    TypeName type_name;
    std::unique_ptr<Type> type;
};


struct Module {
    ast::Block body;
};


template <class T> std::unique_ptr<T> pcopy(const T& v) { auto r = std::make_unique<T>(); v.copy_to(*r); return r; }
std::unique_ptr<Type> copy(const std::unique_ptr<Type>& v);
inline Variable copy(const Variable& v) { return {v.identifier, copy(v.type)}; }
inline Parameter copy(const Parameter& v) { return {v.identifier, copy(v.type)}; }
Block copy(const Block& v);


} // namespace ast
} // namespace xci::script

#endif // include guard
