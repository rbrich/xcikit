// Parser.cpp created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Parser.h"
#include "Error.h"
#include <xci/core/parser/unescape_rules.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#ifndef NDEBUG
#include <tao/pegtl/contrib/analyze.hpp>
#endif

#include <fmt/core.h>

// Enable for detailed trace of Grammar rules being matched
// #define XCI_SCRIPT_PARSER_TRACE

#ifdef XCI_SCRIPT_PARSER_TRACE
#include <xci/core/string.h>
#include <xci/core/container/ChunkedStack.h>
#endif

namespace xci::script {

namespace parser {

using namespace tao::pegtl;
using namespace xci::core::parser::unescape;


// ----------------------------------------------------------------------------
// Grammar

struct Keyword;
struct ExprCond;
struct Statement;
struct ExprArgSafe;
struct ExprOperand;
struct ExprCallable;
struct Type;
struct UnsafeType;

// Spaces and comments
struct LineComment: if_must< two<'/'>, until<eolf> > {};
struct BlockComment: if_must< string<'/', '*'>, until< string<'*', '/'>, any > > {};
struct EscapedNewline: seq< one<'\\'>, eol> {};
struct SpaceOrComment: sor< blank, EscapedNewline, BlockComment> {};
struct NlSpaceOrComment: sor< space, EscapedNewline, LineComment, BlockComment > {};
struct SemicolonOrNewline: sor<one<';'>, eol, LineComment> {};
struct SC: star< SpaceOrComment > {};  // optional space or comments
struct NSC: star< NlSpaceOrComment > {};  // optional newlines, space or comments
struct RS: at<space> {};  // require space

// Aux templates
template<class T> struct SepList: list_tail<T, seq<SC, SemicolonOrNewline, NSC> > {};  // list separated by either semicolon or newline

// Basic tokens
//                 underscore* (lower identifier_other* | digit+)
struct Identifier: seq< not_at<Keyword>, star<one<'_'>>, sor<seq<lower, star<identifier_other>>, plus<digit>> > {};
struct TypeName: seq< upper, star< identifier_other > > {};
struct PrefixOperator: sor< one<'-'>, one<'+'>, one<'!'>, one<'~'> > {};
struct InfixOperator: sor< one<','>, two<'&'>, two<'|'>, two<'='>, string<'!','='>,
                           string<'<','='>, string<'>','='>,
                           two<'<'>, two<'>'>, one<'<'>, one<'>'>,
                           one<'+'>, one<'-'>, two<'*'>, one<'*'>,
                           seq<one<'/'>, not_at<one<'/', '*'>>>, one<'%'>, one<'!'>,
                           one<'&'>, one<'|'>, one<'^'> > {};

// Keywords
struct KeywordFun: TAO_PEGTL_KEYWORD("fun") {};
struct KeywordIf: TAO_PEGTL_KEYWORD("if") {};
struct KeywordThen: TAO_PEGTL_KEYWORD("then") {};
struct KeywordElse: TAO_PEGTL_KEYWORD("else") {};
struct KeywordClass: TAO_PEGTL_KEYWORD("class") {};
struct KeywordInstance: TAO_PEGTL_KEYWORD("instance") {};
struct KeywordWith: TAO_PEGTL_KEYWORD("with") {};
struct KeywordMatch: TAO_PEGTL_KEYWORD("match") {};
struct Keyword: sor<KeywordFun, KeywordIf, KeywordThen, KeywordElse,
        KeywordClass, KeywordInstance, KeywordWith, KeywordMatch> {};

// Literals
struct Integer: seq< opt<one<'-','+'>>, plus<digit> > {};
struct Float: seq< opt<one<'-','+'>>, plus<digit>, one<'.'>, star<digit> > {};
struct Char: if_must< one<'\''>, StringCh, one<'\''> > {};
struct Bytes: if_must< seq<one<'b'>, one<'"'>>, until<one<'"'>, StringCh > > {};
struct String: if_must< one<'"'>, until<one<'"'>, StringCh > > {};
struct RawString : raw_string< '$', '-', '$' > {};  // raw_string = $$ raw text! $$
struct Literal: sor< Float, Integer, Char, Bytes, String, RawString > {};

// Expressions
// * some rules are parametrized with S (space type), choose either SC or NSC (allow newline)
// * in general, rules inside brackets (round or square) use NSC, rules outside brackets use SC
// * this allows leaving out semicolons but still support multiline expressions
template<class S> struct DotCall: if_must< one<'.'>, SC, seq< ExprCallable, star<RS, S, ExprArgSafe> > > {};
template<class S> struct ExprInfixRight: seq< sor< DotCall<S>, if_must<InfixOperator, S, ExprOperand> >, S, opt< ExprInfixRight<S> > > {};
template<class S> struct ExprInfix: seq< ExprOperand, S, opt<ExprInfixRight<S> > > {};
template<> struct ExprInfix<SC>: seq< ExprOperand, sor< seq<NSC, at< one<'.'> > >, SC>, opt<ExprInfixRight<SC> > > {};  // specialization to allow newline before dotcall even outside brackets
template<class S> struct Expression: sor< ExprCond, ExprInfix<S> > {};
struct Variable: seq< Identifier, opt<SC, one<':'>, SC, must<UnsafeType> > > {};
struct Parameter: sor< Type, seq< Identifier, opt<SC, one<':'>, SC, must<Type> > > > {};
struct DeclParams: seq< plus<Parameter, SC> > {};
struct DeclResult: if_must< string<'-', '>'>, SC, Type > {};
struct TypeConstraint: seq<TypeName, RS, SC, TypeName> {};
struct TypeContext: if_must< one<'('>, SC, TypeConstraint, SC, star_must<one<','>, SC, TypeConstraint, SC>, one<')'> > {};
struct FunctionType: seq< DeclParams, SC, DeclResult > {};
struct FunctionDecl: seq< DeclParams, SC, opt<DeclResult>, SC, opt<if_must<KeywordWith, SC, TypeContext>> > {};
struct ListType: if_must< one<'['>, SC, Type, SC, one<']'> > {};
struct UnsafeType: sor<FunctionType, ListType, TypeName> {};   // usable in context where Type is already expected
struct BracketedType: if_must< one<'('>, SC, UnsafeType, SC, one<')'> > {};
struct Type: sor< BracketedType, ListType, TypeName > {};
struct Block: if_must< one<'{'>, NSC, sor< one<'}'>, seq<SepList<Statement>, NSC, one<'}'>> > > {};
struct Function: sor< Block, if_must< KeywordFun, NSC, FunctionDecl, NSC, Block> > {};
struct BracketedExpr: if_must< one<'('>, NSC, Expression<NSC>, NSC, one<')'> > {};
struct ExprPrefix: if_must< PrefixOperator, SC, ExprOperand, SC > {};
struct Reference: seq<Identifier, not_at<one<'"'>>> {};
struct List: if_must< one<'['>, NSC, opt<ExprInfix<NSC>, NSC>, one<']'> > {};
struct ExprCallable: sor< BracketedExpr, Function, Reference> {};
struct ExprArgSafe: sor< BracketedExpr, List, Function, Literal, Reference > {};  // expressions which can be used as args in Call
struct Call: seq< ExprCallable, plus<RS, SC, ExprArgSafe> > {};
struct ExprOperand: sor<Call, ExprArgSafe, ExprPrefix> {};
struct ExprCond: if_must< KeywordIf, NSC, ExprInfix<NSC>, NSC, KeywordThen, NSC, Expression<SC>, NSC, KeywordElse, NSC, Expression<SC>> {};

// Statements
struct Definition: seq< Variable, SC, seq<one<'='>, not_at<one<'='>>, NSC, must<Expression<SC>>> > {};  // must not match `var == ...`
struct Statement: sor< Definition, Expression<SC> > {};

// Module-level definitions
struct ClassDefinition: seq< Variable, SC, opt_must<one<'='>, SC, Expression<SC>> > {};
struct DefClass: if_must< KeywordClass, NSC, TypeName, RS, SC, TypeName, SC, opt<TypeContext>, NSC,
        one<'{'>, NSC, sor< one<'}'>, must<SepList<ClassDefinition>, NSC, one<'}'>> > > {};
struct DefInstance: if_must< KeywordInstance, NSC, TypeName, RS, SC, Type, SC, opt<TypeContext>, NSC,
        one<'{'>, NSC, sor< one<'}'>, must<SepList<Definition>, NSC, one<'}'>> > > {};
struct TopLevelStatement: sor<DefClass, DefInstance, Statement> {};

// Source module
struct Module: must<NSC, opt<SepList<TopLevelStatement>, NSC>, eof> {};


// ----------------------------------------------------------------------------
// Actions

template<typename Rule>
struct Action : nothing<Rule> {};


template<>
struct Action<Definition> : change_states< ast::Definition > {
    template<typename Input>
    static void success(const Input &in, ast::Definition& def, ast::Module& mod) {
        mod.body.statements.push_back(std::make_unique<ast::Definition>(std::move(def)));
    }

    template<typename Input>
    static void success(const Input &in, ast::Definition& def, ast::Block& block) {
        block.statements.push_back(std::make_unique<ast::Definition>(std::move(def)));
    }

    template<typename Input>
    static void success(const Input &in, ast::Definition& def, ast::Instance& inst) {
        inst.defs.push_back(std::move(def));
    }
};


template<class S>
struct Action<Expression<S>> : change_states< std::unique_ptr<ast::Expression> > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr->source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Module& mod) {
        mod.body.statements.push_back(std::make_unique<ast::Invocation>(std::move(expr)));
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Block& block) {
        block.statements.push_back(std::make_unique<ast::Invocation>(std::move(expr)));
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Definition& def) {
        def.expression = std::move(expr);
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
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Bracketed& bracketed) {
        bracketed.expression = std::move(expr);
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


template<class S>
struct Action<ExprInfixRight<S>> : change_states< ast::OpCall > {
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


template<class S>
struct Action<ExprInfix<S>> : change_states< ast::OpCall > {
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
struct Action<BracketedExpr> : change_states< ast::Bracketed > {
    template<typename Input>
    static void apply(const Input &in, ast::Bracketed& bracketed) {
        bracketed.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Bracketed& bracketed, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Bracketed>(std::move(bracketed));
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


template<class S>
struct Action<DotCall<S>> : change_states< ast::Call > {
    template<typename Input>
    static void apply(const Input &in, ast::Call& call) {
        call.source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Call& call, ast::OpCall& outer_opc) {
        auto expr = std::make_unique<ast::Call>(std::move(call));
        outer_opc.args.push_back(std::move(expr));
        outer_opc.op = ast::Operator::DotCall;
    }
};


template<>
struct Action<Identifier> {
    template<typename Input>
    static void apply(const Input &in, ast::Variable& var) {
        var.identifier.name = in.string();
    }

    template<typename Input>
    static void apply(const Input &in, ast::Parameter& par) {
        par.identifier.name = in.string();
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
struct Action<FunctionDecl> : change_states< ast::FunctionType > {
    template<typename Input>
    static void success(const Input &in, ast::FunctionType& ftype, std::unique_ptr<ast::Function>& fn) {
        if (!fn)
            fn = std::make_unique<ast::Function>();
        fn->type = std::move(ftype);
    }
};



template<>
struct Action<TypeName> {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::TypeName>(in.string());
    }

    template<typename Input>
    static void apply(const Input &in, ast::TypeConstraint& tcst) {
        if (tcst.type_class.name.empty())
            tcst.type_class.name = in.string();
        else
            tcst.type_name.name = in.string();
    }

    template<typename Input>
    static void apply(const Input &in, ast::Class& cls) {
        if (cls.class_name.name.empty())
            cls.class_name.name = in.string();
        else
            cls.type_var.name = in.string();
    }

    template<typename Input>
    static void apply(const Input &in, ast::Instance& inst) {
        inst.class_name.name = in.string();
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
struct Action<TypeConstraint> : change_states< ast::TypeConstraint > {
    template<typename Input>
    static void success(const Input &in, ast::TypeConstraint& tcst, ast::FunctionType& ftype) {
        ftype.context.push_back(std::move(tcst));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeConstraint& tcst, ast::Class& cls) {
        cls.context.push_back(std::move(tcst));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeConstraint& tcst, ast::Instance& inst) {
        inst.context.push_back(std::move(tcst));
    }
};


template<>
struct Action<FunctionType> : change_states< ast::FunctionType > {
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
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::Parameter& par) {
        par.type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::Instance& inst) {
        inst.type_inst = std::move(type);
    }
};


template<>
struct Action<UnsafeType> : change_states< std::unique_ptr<ast::Type> >  {
    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& inner, std::unique_ptr<ast::Type>& outer) {
        outer = std::move(inner);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::Variable& var) {
        var.type = std::move(type);
    }
};


template<>
struct Action<Variable> : change_states< ast::Variable > {
    template<typename Input>
    static void success(const Input &in, ast::Variable& var, ast::Definition& def) {
        def.variable = std::move(var);
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
struct Action<DefClass> : change_states< ast::Class > {
    template<typename Input>
    static void success(const Input &in, ast::Class& cls, ast::Module& mod) {
        mod.body.statements.emplace_back(std::make_unique<ast::Class>(std::move(cls)));
    }
};


template<>
struct Action<ClassDefinition> : change_states< ast::Definition > {
    template<typename Input>
    static void success(const Input &in, ast::Definition& def, ast::Class& cls) {
        cls.defs.push_back(std::move(def));
    }
};


template<>
struct Action<DefInstance> : change_states< ast::Instance > {
    template<typename Input>
    static void success(const Input &in, ast::Instance& inst, ast::Module& mod) {
        mod.body.statements.emplace_back(std::make_unique<ast::Instance>(std::move(inst)));
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
struct Action<Char> : change_states< std::string > {
  template<typename Input>
  static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
    expr->source_info.load(in.input(), in.position());
  }

  template<typename Input>
  static void success(const Input &in, std::string& str, std::unique_ptr<ast::Expression>& expr) {
    expr = std::make_unique<ast::Char>(str);
  }
};


template<>
struct Action<Bytes> : change_states< std::string > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr->source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::string& str, std::unique_ptr<ast::Expression>& expr) {
        str.shrink_to_fit();
        expr = std::make_unique<ast::Bytes>(str);
    }
};


template<>
struct Action<String> : change_states< std::string > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr->source_info.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::string& str, std::unique_ptr<ast::Expression>& expr) {
        str.shrink_to_fit();
        expr = std::make_unique<ast::String>(std::move(str));
    }
};


template<> struct Action<StringChOther> : StringAppend {};
template<> struct Action<StringChEscSingle> : StringAppendEscSingle {};
template<> struct Action<StringChEscHex> : StringAppendEscHex {};
template<> struct Action<StringChEscOct> : StringAppendEscOct {};


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

#ifdef XCI_SCRIPT_PARSER_TRACE
static core::ChunkedStack<std::string> g_untraced;

inline bool do_not_trace(std::string_view rule) {
    return rule == "xci::script::parser::SC"
        || rule == "xci::script::parser::Identifier";
}
#endif

template< typename Rule >
struct Control : normal< Rule >
{
    static const std::string errmsg;

    template< typename Input, typename... States >
    static void raise( const Input& in, States&&... /*unused*/ ) {
        throw parse_error( errmsg, in );
    }

#ifdef XCI_SCRIPT_PARSER_TRACE
    template< typename Input >
    static void print_input( const Input& in )
    {
        size_t size = in.size();
        if (size > 20)
            std::cerr << '"' << xci::core::escape({in.current(), 20}) << "\"...";
        else
            std::cerr << '"' << xci::core::escape({in.current(), size}) << "\"$";
    }

    template< typename Input, typename... States >
    static void start( const Input& in, States&&... st )
    {
        auto rule = tao::demangle<Rule>();
        if (do_not_trace(rule)) {
            if (g_untraced.empty()) {
                std::cerr << in.position() << "  start  " << rule << " (untraced)" << "; current ";
                print_input( in );
                std::cerr << std::endl;
            }
            g_untraced.push(std::string(rule));
        }
        if (!g_untraced.empty()) {
            normal<Rule>::start( in, st... );
            return;
        }

        std::cerr << in.position() << "  start  " << rule << "; current ";
        print_input( in );
        std::cerr << std::endl;
        normal<Rule>::start( in, st... );
    }

    template< typename Input, typename... States >
    static void success( const Input& in, States&&... st )
    {
        auto rule = tao::demangle<Rule>();
        if (!g_untraced.empty() && g_untraced.top() == rule)
            g_untraced.pop();
        if (!g_untraced.empty()) {
            normal<Rule>::success( in, st... );
            return;
        }

        std::cerr << in.position() << " success " << rule << "; next ";
        print_input( in );
        std::cerr << std::endl;
        normal<Rule>::success( in, st... );
    }

    template< typename Input, typename... States >
    static void failure( const Input& in, States&&... st )
    {
        auto rule = tao::demangle<Rule>();
        if (!g_untraced.empty() && g_untraced.top() == rule)
            g_untraced.pop();
        if (!g_untraced.empty()) {
            normal<Rule>::failure( in, st... );
            return;
        }

        std::cerr << in.position() << " failure " << rule << std::endl;
        normal<Rule>::failure( in, st... );
    }

    template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
    static auto apply( const Iterator& begin, const Input& in, States&&... st )
    -> decltype( normal<Rule>::template apply< Action >( begin, in, st... ) )
    {
        std::cerr << in.position() << "  apply  " << tao::demangle<Rule>() << std::endl;
        return normal<Rule>::template apply< Action >( begin, in, st... );
    }

    template< template< typename... > class Action, typename Input, typename... States >
    static auto apply0( const Input& in, States&&... st )
    -> decltype( normal<Rule>::template apply0< Action >( in, st... ) )
    {
        std::cerr << in.position() << "  apply0 " << tao::demangle<Rule>() << std::endl;
        return normal<Rule>::template apply0< Action >( in, st... );
    }

#endif
};

template<> const std::string Control<eof>::errmsg = "invalid syntax";
template<> const std::string Control<Expression<SC>>::errmsg = "expected Expression";
template<> const std::string Control<Expression<NSC>>::errmsg = "expected Expression";
template<> const std::string Control<DeclParams>::errmsg = "expected function parameter declaration";
template<> const std::string Control<ExprInfixRight<SC>>::errmsg = "expected infix operator";
template<> const std::string Control<ExprInfixRight<NSC>>::errmsg = "expected infix operator";
template<> const std::string Control<Variable>::errmsg = "expected variable name";

// default message
template< typename T >
const std::string Control< T >::errmsg = "parse error matching " + std::string(tao::demangle< T >());


// ----------------------------------------------------------------------------

} // namespace parser


void Parser::parse(std::string_view input, ast::Module& mod)
{
    using parser::Module;
    using parser::Action;
    using parser::Control;

    tao::pegtl::memory_input<
        tao::pegtl::tracking_mode::eager,
        tao::pegtl::eol::lf_crlf,
        const char*>  // pass source filename as non-owning char*
    in(input.data(), input.size(), "<input>");

    try {
        if (!tao::pegtl::parse< Module, Action, Control >( in, mod ))
            throw ParseError{"input not matched"};
        mod.body.finish();
    } catch (tao::pegtl::parse_error& e) {
        const auto p = e.positions().front();
        throw ParseError{fmt::format("{}\n{}\n{:>{}}", e.what(), in.line_at(p), '^', p.column)};
    } catch( const std::exception& e ) {
        throw ParseError{e.what()};
    }
}


#ifndef NDEBUG
size_t Parser::analyze_grammar()
{
    using parser::Module;

    return tao::pegtl::analyze< Module >(1);
}
#endif


} // namespace xci::script
