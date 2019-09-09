// Parser.cpp created on 2019-05-15, part of XCI toolkit
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

#include "Parser.h"
#include "Error.h"
#include <xci/core/format.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
#include <iostream>

namespace xci::script {

namespace parser {

using namespace tao::pegtl;


// ----------------------------------------------------------------------------
// Grammar

struct Keyword;
struct Expression;
struct Statement;
struct ExprArgSafe;
struct ExprOperand;
struct ExprInfix;
struct Type;

// Spaces and comments
struct LineComment: if_must< two<'/'>, until<eolf> > {};
struct BlockComment: if_must< string<'/', '*'>, until< string<'*', '/'>, any > > {};
struct SpaceOrComment: sor< space, LineComment, BlockComment> {};
struct SC: star< SpaceOrComment > {};
struct RSC: seq<space, SC> {};  // required at least one space

// Basic tokens
struct Semicolon: sor<one<';'>, eolf> {};  // optional at EOL / EOF
struct Identifier: seq< not_at<Keyword>, lower, star< identifier_other > > {};
struct TypeName: seq< upper, star< identifier_other > > {};
struct PrefixOperator: sor< one<'-'>, one<'+'>, one<'!'>, one<'~'> > {};
struct InfixOperator: sor< two<'&'>, two<'|'>, two<'='>, string<'!','='>,
                           string<'<','='>, string<'>','='>,
                           two<'<'>, two<'>'>, one<'<'>, one<'>'>,
                           one<'+'>, one<'-'>, two<'*'>, one<'*'>,
                           one<'/'>, one<'%'>, one<'!'>,
                           one<'&'>, one<'|'>, one<'^'> > {};

// Keywords
struct KeywordIf: TAO_PEGTL_KEYWORD("if") {};
struct KeywordThen: TAO_PEGTL_KEYWORD("then") {};
struct KeywordElse: TAO_PEGTL_KEYWORD("else") {};
struct Keyword: sor<KeywordIf, KeywordThen, KeywordElse> {};

// Literals
struct Integer: seq< opt<one<'-','+'>>, plus<digit> > {};
struct Float: seq< opt<one<'-','+'>>, plus<digit>, one<'.'>, star<digit> > {};
struct StringChEscSingle : one< 'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '0', '\n' > {};
struct StringChEscHex : if_must< one< 'x' >, xdigit, xdigit > {};
struct StringChEsc : if_must< one< '\\' >, sor< StringChEscHex, StringChEscSingle > > {};
struct StringCh : sor< StringChEsc, not_one<'\r', '\n'> > {};
struct String: if_must< one<'"'>, until<one<'"'>, StringCh > > {};
struct RawString : raw_string< '$', '-', '$' > {};  // raw_string = $$ raw text! $$
struct Literal: sor< Float, Integer, String, RawString > {};

// Expressions
struct Variable: seq< Identifier, opt<SC, one<':'>, SC, Type > > {};
struct Parameter: sor< Type, Variable > {};
struct DeclParams: seq< one<'|'>, SC, plus<Parameter, SC>, one<'|'> > {};  // do not fail if '|' matches but the rest doesn't - it may be |-operator instead
struct DeclResult: if_must< string<'-', '>'>, SC, Type > {};
struct FunctionType: seq< DeclParams, SC, opt<DeclResult> > {};
struct ListType: if_must< one<'['>, SC, Type, SC, one<']'> > {};
struct Type: sor< TypeName, ListType, FunctionType > {};
struct Block: if_must< one<'{'>, SC, sor< one<'}'>, seq<Statement, SC, star<Semicolon, SC, Statement, SC>, opt<Semicolon, SC>, one<'}'>> > > {};
struct Function: seq< opt<FunctionType>, SC, Block> {};
struct BracedExpr: if_must< one<'('>, SC, Expression, SC, one<')'> > {};
struct ExprPrefix: if_must< PrefixOperator, SC, ExprOperand, SC > {};
struct Reference: seq< Identifier > {};
struct List: if_must< one<'['>, SC, sor<one<']'>, seq<ExprInfix, SC, until<one<']'>, one<','>, SC, ExprInfix, SC>>> > {};
struct ExprCallable: sor< BracedExpr, Reference, Function> {};
struct ExprArgSafe: sor< BracedExpr, List, Literal, Reference, Function > {};  // expressions which can be used as args in Call
struct Call: seq< ExprCallable, plus<RSC, ExprArgSafe> > {};
struct ExprOperand: sor<Call, ExprArgSafe, ExprPrefix> {};
struct ExprInfixRight: if_must<InfixOperator, SC, ExprOperand, SC, opt<ExprInfixRight>> {};
struct ExprInfix: seq< ExprOperand, SC, opt<ExprInfixRight> > {};
struct ExprCond: if_must< KeywordIf, SC, ExprInfix, SC, KeywordThen, SC, Expression, SC, KeywordElse, SC, Expression> {};
struct ExprTuple: if_must< seq<ExprInfix, SC, one<','>>, SC, ExprInfix, star<SC, one<','>, SC, ExprInfix> > {};
struct Expression: sor< ExprCond, ExprTuple, ExprInfix > {};

// Statements
struct Definition: seq< Variable, SC, seq<one<'='>, not_at<one<'='>>, SC, must<Expression>> > {};  // must not match `var == ...`
struct Invocation: seq< Expression > {};
struct Statement: sor< Definition, Invocation > {};

// Source module
struct Module: until< eof, must<seq< SC, Statement, SC, Semicolon, SC >> > {};


// ----------------------------------------------------------------------------
// Actions

template<typename Rule>
struct Action : nothing<Rule> {};


template<>
struct Action<Module> : change_states< ast::Block > {
    template<typename Input>
    static void success(const Input &in, ast::Block& block, AST &ast) {
        block.finish();
        ast.body = std::move(block);
    }
};

template<>
struct Action<Definition> : change_states< ast::Definition > {
    template<typename Input>
    static void success(const Input &in, ast::Definition& def, ast::Block& block) {
        block.statements.push_back(std::make_unique<ast::Definition>(std::move(def)));
    }
};


template<>
struct Action<Invocation> : change_states< ast::Invocation > {
    template<typename Input>
    static void success(const Input &in, ast::Invocation& inv, ast::Block& block) {
        block.statements.push_back(std::make_unique<ast::Invocation>(std::move(inv)));
    }
};


template<>
struct Action<Expression> : change_states< std::unique_ptr<ast::Expression> > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr->source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Definition& def) {
        def.expression = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Invocation& inv) {
        inv.expression = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Condition& cnd) {
        if (!cnd.then_expr) {
            cnd.then_expr = std::move(expr);
        } else {
            assert(!cnd.else_expr);
            cnd.else_expr = std::move(expr);
        }
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, std::unique_ptr<ast::Expression>& outer_expr) {
        // outer is BracedExpr
        outer_expr = std::move(expr);
    }
};


template<>
struct Action<PrefixOperator> {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        ast::Operator op{in.string(), true};
        opc.op = op;
    }
};

template<>
struct Action<ExprPrefix> : change_states< ast::OpCall > {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        opc.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& inner, ast::OpCall& outer) {
        auto expr = std::make_unique<ast::OpCall>(std::move(inner));
        outer.args.push_back(std::move(expr));
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& inner, ast::Call& outer) {
        auto expr = std::make_unique<ast::OpCall>(std::move(inner));
        outer.args.push_back(std::move(expr));
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& inner, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::OpCall>(std::move(inner));
    }
};

template<>
struct Action<InfixOperator> {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        ast::Operator op{in.string()};
        opc.op = op;
    }
};


template<>
struct Action<ExprInfixRight> : change_states< ast::OpCall > {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        opc.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& right, ast::OpCall& left) {
        // precedence parser, inspired by Pratt parser
        if (left.op.is_undefined()) {
            left.op = right.op;
            // append right args
            left.args.insert(
                left.args.end(),
                std::make_move_iterator(right.args.begin()),
                std::make_move_iterator(right.args.end())
            );
            left.right_tmp = std::move(right.right_tmp);
            left.source_info = right.source_info;
            while (left.right_tmp) {
                auto expr_tmp = std::make_unique<ast::OpCall>(std::move(left));
                left = std::move(*expr_tmp->right_tmp);
                expr_tmp->right_tmp.reset();
                left.args.insert(left.args.begin(), std::move(expr_tmp));
            }
        } else {
            if (left.op.precedence() - int(left.op.is_right_associative()) < right.op.precedence()) {
                // Eg. 1+2*3 :: left='+(2)', right='*(3)'
                //           => left='+(right), right='*(2 3)'
                right.args.insert(right.args.begin(), std::move(left.args.back()));
                while (right.right_tmp && left.op.precedence() < right.right_tmp->op.precedence()) {
                    // Eg. 1 || 2*3-4 :: left='||(?right)', right='*(2 3), tmp='-(4)'
                    //                => left='||(?right)', right='-(*(2 3) 4)'
                    auto expr_tmp = std::make_unique<ast::OpCall>(std::move(right));
                    right = std::move(*expr_tmp->right_tmp);
                    expr_tmp->right_tmp.reset();
                    right.args.insert(right.args.begin(), std::move(expr_tmp));
                }
                left.right_tmp = std::move(right.right_tmp);
                auto expr_right = std::make_unique<ast::OpCall>(std::move(right));
                left.args.back() = std::move(expr_right);
            } else {
                // Eg. 1*2+3 :: left='*(2)', right='+(3)'
                assert(!left.right_tmp);
                left.right_tmp = std::make_unique<ast::OpCall>(std::move(right));
            }
        }
    }
};


template<>
struct Action<ExprInfix> : change_states< ast::OpCall > {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        opc.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& opc, std::unique_ptr<ast::Expression>& expr) {
        expr = prepare_expression(opc);
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& opc, ast::Condition& cnd) {
        cnd.cond = prepare_expression(opc);
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& opc, ast::Tuple& tpl) {
        tpl.items.emplace_back(prepare_expression(opc));
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& opc, ast::List& lst) {
        lst.items.emplace_back(prepare_expression(opc));
    }

private:
    static std::unique_ptr<ast::Expression> prepare_expression(ast::OpCall& opc) {
        // Collapse empty OpCall
        if (opc.op.is_undefined()) {
            assert(!opc.right_tmp);
            assert(opc.args.size() == 1);
            auto& expr = opc.args.front();
            expr->source_info = opc.source_info;
            return move(expr);
        } else {
            return std::make_unique<ast::OpCall>(std::move(opc));
        }
    }
};


template<>
struct Action<ExprCond> : change_states< ast::Condition > {
    template<typename Input>
    static void apply(const Input &in, ast::Condition& cnd) {
        cnd.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Condition& cnd, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Condition>(std::move(cnd));
    }
};


template<>
struct Action<ExprTuple> : change_states< ast::Tuple > {
    template<typename Input>
    static void apply(const Input &in, ast::Tuple& tpl) {
        tpl.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Tuple& tpl, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Tuple>(std::move(tpl));
    }
};


template<>
struct Action<List> : change_states< ast::List > {
    template<typename Input>
    static void apply(const Input &in, ast::List& lst) {
        lst.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::List& lst, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::List>(std::move(lst));
    }
};


template<>
struct Action<ExprCallable> : change_states< std::unique_ptr<ast::Expression> > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr->source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Call& call) {
        call.callable = std::move(expr);
    }
};


template<>
struct Action<ExprArgSafe> : change_states< std::unique_ptr<ast::Expression> > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr->source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Call& call) {
        call.args.push_back(std::move(expr));
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::OpCall& opc) {
        opc.args.push_back(std::move(expr));
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, std::unique_ptr<ast::Expression>& outer_expr) {
        outer_expr = std::move(expr);
    }
};


template<>
struct Action<Reference> : change_states< ast::Reference > {
    template<typename Input>
    static void apply(const Input &in, ast::Reference& ref) {
        ref.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Reference& ref, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Reference>(std::move(ref));
    }
};


template<>
struct Action<Call> : change_states< ast::Call > {
    template<typename Input>
    static void apply(const Input &in, ast::Call& call) {
        call.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Call& call, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Call>(std::move(call));
    }

    template<typename Input>
    static void success(const Input &in, ast::Call& call, ast::Call& outer_call) {
        auto expr = std::make_unique<ast::Call>(std::move(call));
        outer_call.args.push_back(std::move(expr));
    }

    template<typename Input>
    static void success(const Input &in, ast::Call& call, ast::OpCall& outer_opc) {
        auto expr = std::make_unique<ast::Call>(std::move(call));
        outer_opc.args.push_back(std::move(expr));
    }
};

template<>
struct Action<Identifier> {
    template<typename Input>
    static void apply(const Input &in, ast::Variable& var) {
        var.identifier.name = in.string();
    }

    template<typename Input>
    static void apply(const Input &in, ast::Reference& ref) {
        ref.identifier.name = in.string();
    }
};


template<>
struct Action<Function> : change_states< std::unique_ptr<ast::Function> > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Function>& fn) {
        fn->source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Function>& fn, std::unique_ptr<ast::Expression>& expr) {
        expr = std::move(fn);
    }
};


template<>
struct Action<TypeName> {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::TypeName>(in.string());
    }
};


template<>
struct Action<ListType> : change_states< ast::ListType > {
    template<typename Input>
    static void success(const Input &in, ast::ListType& ltype, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::ListType>(std::move(ltype));
    }
};


template<>
struct Action<FunctionType> : change_states< ast::FunctionType > {
    template<typename Input>
    static void success(const Input &in, ast::FunctionType& ftype, std::unique_ptr<ast::Function>& fn) {
        if (!fn)
            fn = std::make_unique<ast::Function>();
        fn->type = std::move(ftype);
    }

    template<typename Input>
    static void success(const Input &in, ast::FunctionType& ftype, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::FunctionType>(std::move(ftype));
    }
};


template<>
struct Action<Type> : change_states< std::unique_ptr<ast::Type> >  {
    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::FunctionType& ftype) {
        ftype.result_type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::ListType& ltype) {
        ltype.elem_type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::Variable& var) {
        var.type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::Parameter& par) {
        par.type = std::move(type);
    }
};

template<>
struct Action<Variable> : change_states< ast::Variable > {
    template<typename Input>
    static void success(const Input &in, ast::Variable& var, ast::Definition& def) {
        def.variable = std::move(var);
    }

    template<typename Input>
    static void success(const Input &in, ast::Variable& var, ast::Parameter& par) {
        par.identifier = std::move(var.identifier);
        par.type = std::move(var.type);
    }
};

template<>
struct Action<Parameter> : change_states< ast::Parameter > {
    template<typename Input>
    static void success(const Input &in, ast::Parameter& par, ast::FunctionType& ftype) {
        ftype.params.push_back(std::move(par));
    }
};

template<>
struct Action<Block> : change_states< ast::Block > {
    template<typename Input>
    static void success(const Input &in, ast::Block& block, std::unique_ptr<ast::Function>& fn) {
        block.finish();
        if (!fn)
            fn = std::make_unique<ast::Function>();
        fn->body = std::move(block);
    }
};


template<>
struct Action<Float> {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Float>(in.string());
        expr->source_info.load(in.input(), in.position());
    }
};


template<>
struct Action<Integer> {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Integer>(in.string());
        expr->source_info.load(in.input(), in.position());
    }
};


template<>
struct Action<String> {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        const auto& str = in.string();
        expr = std::make_unique<ast::String>(str.substr(1, str.size() - 2));
        expr->source_info.load(in.input(), in.position());
    }
};


template<>
struct Action<RawString::content> {
    template<typename Input, typename States>
    static void apply(const Input &in, const States& /* raw_string states */, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::String>(in.string());
        expr->source_info.load(in.input(), in.position());
    }
};


// ----------------------------------------------------------------------------
// Control (error reporting)

template< typename Rule >
struct Control : normal< Rule >
{
    static const std::string errmsg;

    template< typename Input, typename... States >
    static void raise( const Input& in, States&&... /*unused*/ ) {
        throw parse_error( errmsg, in );
    }
};

template<> const std::string Control<Expression>::errmsg = "expected Expression";
template<> const std::string Control<Semicolon>::errmsg = "expected ';'";
template<> const std::string Control<DeclParams>::errmsg = "expected function parameter declaration";
template<> const std::string Control<ExprInfixRight>::errmsg = "expected infix operator";
template<> const std::string Control<Variable>::errmsg = "expected variable name";

// default message
template< typename T >
const std::string Control< T >::errmsg = "parse error matching " + internal::demangle< T >();


// ----------------------------------------------------------------------------

} // namespace parser


void Parser::parse(const std::string& input, AST& ast)
{
    using parser::Module;
    using parser::Action;
    using parser::Control;

    tao::pegtl::memory_input<
        tao::pegtl::tracking_mode::eager,
        tao::pegtl::eol::lf_crlf,
        const char*>  // pass source filename as non-owning char*
    in(input, "<input>");

    try {
        if (!tao::pegtl::parse< Module, Action, Control >( in, ast ))
            throw ParseError{"input not matched"};
    } catch (tao::pegtl::parse_error& e) {
        const auto p = e.positions.front();
        throw ParseError{core::format("{}\n{}\n{}^", e.what(), in.line_at(p),
                                      std::string(p.byte_in_line, ' '))};
    } catch( const std::exception& e ) {
        throw ParseError{e.what()};
    }
}


} // namespace xci::script
