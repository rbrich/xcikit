// Parser.cpp created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Parser.h"
#include "Error.h"
#include "TypeInfo.h"
#include "parser/raw_string.h"
#include <xci/core/parser/unescape_rules.h>

#include <tao/pegtl.hpp>

#ifndef NDEBUG
#include <tao/pegtl/contrib/analyze.hpp>
#endif

#include <fmt/core.h>

#include <limits>
#include <charconv>
#include <variant>

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
using ValueType = xci::script::Type;


// ----------------------------------------------------------------------------
// Grammar

struct Keyword;
struct ExprCond;
struct ExprWith;
struct Statement;
struct ExprArgSafe;
struct ExprPrefix;
struct ExprCallable;
struct ExprStruct;
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
struct KeywordType: TAO_PEGTL_KEYWORD("type") {};
struct KeywordWith: TAO_PEGTL_KEYWORD("with") {};
struct KeywordMatch: TAO_PEGTL_KEYWORD("match") {};
struct Keyword: sor<KeywordFun, KeywordIf, KeywordThen, KeywordElse,
        KeywordClass, KeywordInstance, KeywordType, KeywordWith, KeywordMatch> {};

// Literals
struct BinDigit : one< '0', '1' > {};
struct OctDigit : range< '0', '7' > {};
struct BinNum: if_must<one<'b'>, plus<BinDigit>> {};
struct OctNum: if_must<one<'o'>, plus<OctDigit>> {};
struct DecNumFrac: seq< one<'.'>, star<digit> > {};
struct DecNum: seq< plus<digit>, opt<DecNumFrac> > {};
struct HexNum: if_must<one<'x'>, plus<xdigit>> {};
struct Sign: one<'-','+'> {};
struct ZeroPrefixNum: seq< one<'0'>, sor<HexNum, OctNum, BinNum, DecNum> > {};
struct NumSuffix: one<'l', 'L', 'f', 'F', 'b', 'B'> {};
struct Number: seq< opt<Sign>, sor<ZeroPrefixNum, DecNum>, opt<NumSuffix> > {};

struct Char: if_must< one<'\''>, StringCh, one<'\''> > {};
struct StringContent: until< one<'"'>, StringCh > {};
struct String: if_must< one<'"'>, StringContent > {};
struct EscapedQuotes: seq<one<'\\'>, three<'"'>, star<one<'"'>>> {};
struct RawStringCh: any {};
struct RawStringContent: until<three<'"'>, sor<EscapedQuotes, RawStringCh>> {};
struct RawString : if_must< three<'"'>, RawStringContent > {};
struct Byte: seq< one<'b'>, Char > {};
struct Bytes: seq< one<'b'>, String > {};
struct RawBytes: seq< one<'b'>, RawString > {};
struct Literal: sor< Char, RawString, String, Byte, RawBytes, Bytes, Number > {};

// Types
struct Parameter: sor< Type, seq< Identifier, opt<SC, one<':'>, SC, must<Type> > > > {};
struct DeclParams: seq< plus<Parameter, SC> > {};
struct DeclResult: if_must< string<'-', '>'>, SC, Type > {};
struct TypeParams: if_must< one<'<'>, TypeName, SC, star_must<one<','>, SC, TypeName, SC>, one<'>'> > {};
struct TypeConstraint: seq<TypeName, RS, SC, TypeName> {};
struct TypeContext: if_must< one<'('>, SC, TypeConstraint, SC, star_must<one<','>, SC, TypeConstraint, SC>, one<')'> > {};
struct FunctionType: seq< opt<TypeParams>, SC, DeclParams, SC, DeclResult > {};
struct FunctionDecl: seq< opt<TypeParams>, SC, DeclParams, SC, opt<DeclResult>, SC, opt<if_must<KeywordWith, SC, TypeContext>> > {};
struct PlainTypeName: seq< TypeName, not_at<SC, one<','>> > {};  // not followed by comma (would be TupleType)
struct ListType: if_must< one<'['>, SC, UnsafeType, SC, one<']'> > {};
struct TupleType: seq< Type, plus<SC, one<','>, SC, Type> > {};
struct BracketedType: if_must< one<'('>, SC, UnsafeType, SC, one<')'> > {};
struct UnsafeType: sor<FunctionType, PlainTypeName, TupleType, BracketedType, ListType> {};   // usable in context where Type is already expected
struct Type: sor< BracketedType, ListType, TypeName > {};

// Expressions
// * some rules are parametrized with S (space type), choose either SC or NSC (allow newline)
// * in general, rules inside brackets (round or square) use NSC, rules outside brackets use SC
// * this allows leaving out semicolons but still support multiline expressions
template<class S> struct Call: seq< ExprCallable, plus<RS, S, ExprArgSafe> > {};
template<class S> struct DotCall: if_must< one<'.'>, SC, seq< ExprCallable, star<RS, S, ExprArgSafe> > > {};
template<class S> struct ExprOperand: sor<Call<S>, ExprArgSafe, ExprPrefix> {};
template<class S> struct ExprInfixRight: seq< sor< DotCall<S>, seq<InfixOperator, NSC, ExprOperand<S>> >, S, opt< ExprInfixRight<S> > > {};
template<class S> struct TrailingComma: opt<S, one<','>> {};
template<class S> struct ExprInfix: seq< ExprOperand<S>, S, opt<ExprInfixRight<S>>, TrailingComma<S> > {};
template<> struct ExprInfix<SC>: seq< ExprOperand<SC>, sor< seq<NSC, at< one<'.'> > >, SC>, opt<ExprInfixRight<SC>>, TrailingComma<SC> > {};  // specialization to allow newline before dotcall even outside brackets
template<class S> struct Expression: sor< ExprCond, ExprWith, ExprStruct, ExprInfix<S> > {};
struct Variable: seq< Identifier, opt<SC, one<':'>, SC, must<UnsafeType> > > {};
struct Block: if_must< one<'{'>, NSC, sor< one<'}'>, seq<SepList<Statement>, NSC, one<'}'>> > > {};
struct Function: sor< Block, if_must< KeywordFun, NSC, FunctionDecl, NSC, Block> > {};
struct BracketedExpr: if_must< one<'('>, NSC, Expression<NSC>, NSC, one<')'> > {};
struct ExprPrefix: if_must< PrefixOperator, SC, ExprOperand<SC>, SC > {};
struct TypeArgs: seq< one<'<'>, Type, one<'>'> > {};
struct Reference: seq<Identifier, opt<TypeArgs>, not_at<one<'"'>>> {};
struct List: if_must< one<'['>, NSC, opt<ExprInfix<NSC>, NSC>, one<']'> > {};
struct Cast: seq<SC, one<':'>, SC, Type> {};
struct ExprCallable: sor< BracketedExpr, Function, Reference> {};
struct ExprArgSafe: seq< sor< BracketedExpr, List, Function, Literal, Reference >, opt<Cast>> {};  // expressions which can be used as args in Call
struct ExprCond: if_must< KeywordIf, NSC, ExprInfix<NSC>, NSC, KeywordThen, NSC, Expression<SC>, NSC, KeywordElse, NSC, Expression<SC>> {};
struct ExprWith: if_must< KeywordWith, NSC, ExprArgSafe, NSC, Expression<SC> > {};  // might be parsed as a function, but that wouldn't allow newlines
struct ExprStructItem: seq< Identifier, SC, one<'='>, not_at<one<'='>>, SC, must<ExprArgSafe> > {};
struct ExprStruct: seq< ExprStructItem, star< SC, one<','>, SC, must<ExprStructItem> > > {};

// Statements
struct Definition: seq< Variable, SC, seq<one<'='>, not_at<one<'='>>, NSC, must<Expression<SC>>> > {};  // must not match `var == ...`
struct TypeAlias: if_must< TypeName, NSC, one<'='>, not_at<one<'='>>, NSC, UnsafeType > {};
struct Statement: sor< TypeAlias, Definition, Expression<SC> > {};

// Module-level definitions
struct ClassDefinition: seq< Variable, SC, opt_must<one<'='>, SC, Expression<SC>> > {};
struct DefClass: if_must< KeywordClass, NSC, TypeName, RS, SC, plus<TypeName, SC>, opt<TypeContext>, NSC,
        one<'{'>, NSC, sor< one<'}'>, must<SepList<ClassDefinition>, NSC, one<'}'>> > > {};
struct DefInstance: if_must< KeywordInstance, NSC, TypeName, RS, SC, plus<Type, SC>, opt<TypeContext>, NSC,
        one<'{'>, NSC, sor< one<'}'>, must<SepList<Definition>, NSC, one<'}'>> > > {};
struct DefType: if_must< KeywordType, NSC, TypeName, NSC, one<'='>, not_at<one<'='>>, NSC, UnsafeType > {};
struct TopLevelStatement: sor<DefClass, DefInstance, DefType, Statement> {};

// Source module
struct Module: must<NSC, opt<SepList<TopLevelStatement>, NSC>, eof> {};


// ----------------------------------------------------------------------------
// Helper data structs

struct LiteralHelper {
    std::variant<std::string_view, std::string, double, int64_t> content;
    ValueType type = ValueType::Unknown;
};

struct NumberHelper {
    std::variant<double, int64_t> num;
    bool is_float = false;
    char suffix = 0;
};


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


template<>
struct Action<TypeAlias> : change_states< ast::TypeAlias > {
    template<typename Input>
    static void success(const Input &in, ast::TypeAlias& alias, ast::Module& mod) {
        mod.body.statements.push_back(std::make_unique<ast::TypeAlias>(std::move(alias)));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeAlias& alias, ast::Block& block) {
        block.statements.push_back(std::make_unique<ast::TypeAlias>(std::move(alias)));
    }
};


template<class S>
struct Action<Expression<S>> : change_states< std::unique_ptr<ast::Expression> > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Expression>& expr) {
        expr->source_loc.load(in.input(), in.position());
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

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::WithContext& with) {
        assert(!with.expression);
        with.expression = std::move(expr);
    }
};


template<>
struct Action<PrefixOperator> {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        ast::Operator op{in.string_view(), true};
        opc.op = op;
    }
};

template<>
struct Action<ExprPrefix> : change_states< ast::OpCall > {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        opc.source_loc.load(in.input(), in.position());
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
        ast::Operator op{in.string_view()};
        opc.op = op;
    }
};


template<class S>
struct Action<ExprInfixRight<S>> : change_states< ast::OpCall > {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        opc.source_loc.load(in.input(), in.position());
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
            left.source_loc = right.source_loc;
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
        opc.source_loc.load(in.input(), in.position());
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
            expr->source_loc = opc.source_loc;
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
        cnd.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Condition& cnd, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Condition>(std::move(cnd));
    }
};


template<>
struct Action<ExprWith> : change_states< ast::WithContext > {
    template<typename Input>
    static void apply(const Input &in, ast::WithContext& with) {
        with.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::WithContext& with, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::WithContext>(std::move(with));
    }
};


template<>
struct Action<ExprStruct> : change_states< ast::StructInit > {
    template<typename Input>
    static void apply(const Input &in, ast::StructInit& node) {
        node.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::StructInit& node, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::StructInit>(std::move(node));
    }
};


template<>
struct Action<BracketedExpr> : change_states< ast::Bracketed > {
    template<typename Input>
    static void apply(const Input &in, ast::Bracketed& bracketed) {
        bracketed.source_loc.load(in.input(), in.position());
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
        lst.source_loc.load(in.input(), in.position());
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
        expr->source_loc.load(in.input(), in.position());
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
        expr->source_loc.load(in.input(), in.position());
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
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::WithContext& with) {
        assert(!with.context);
        with.context = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::StructInit& node) {
        node.items.back().second = std::move(expr);
    }
};


template<>
struct Action<Reference> : change_states< ast::Reference > {
    template<typename Input>
    static void apply(const Input &in, ast::Reference& ref) {
        ref.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Reference& ref, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Reference>(std::move(ref));
    }
};


template<class S>
struct Action<Call<S>> : change_states< ast::Call > {
    template<typename Input>
    static void apply(const Input &in, ast::Call& call) {
        call.source_loc.load(in.input(), in.position());
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
        call.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Call& call, ast::OpCall& outer_opc) {
        auto expr = std::make_unique<ast::Call>(std::move(call));
        outer_opc.args.push_back(std::move(expr));
        outer_opc.op = ast::Operator::DotCall;
    }
};


template<>
struct Action<Identifier> : change_states< ast::Identifier > {
    template<typename Input>
    static void apply(const Input &in, ast::Identifier& ident) {
        ident.name = in.string();
        ident.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::Variable& var) {
        var.identifier = move(ident);
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::Parameter& par) {
        par.identifier = move(ident);
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::Reference& ref) {
        ref.identifier = move(ident);
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::StructInit& node) {
        node.items.emplace_back(ast::Key(move(ident)), std::unique_ptr<ast::Expression>{});
    }
};


template<>
struct Action<Function> : change_states< std::unique_ptr<ast::Function> > {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Function>& fn) {
        fn->source_loc.load(in.input(), in.position());
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
struct Action<TypeName> : change_states< ast::TypeName > {
    template<typename Input>
    static void apply(const Input &in, ast::TypeName& tname) {
        tname.name = in.string();
        tname.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::TypeName>(move(tname));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::FunctionType& ftype) {
        ftype.type_params.push_back(move(tname));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::TypeConstraint& tcst) {
        if (!tcst.type_class)
            tcst.type_class = move(tname);
        else
            tcst.type_name = move(tname);
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::Class& cls) {
        if (!cls.class_name)
            cls.class_name = move(tname);
        else
            cls.type_vars.push_back(move(tname));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::Instance& inst) {
        inst.class_name = move(tname);
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::TypeDef& def) {
        def.type_name = move(tname);
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::TypeAlias& alias) {
        alias.type_name = move(tname);
    }
};


template<>
struct Action<ListType> : change_states< ast::ListType > {
    template<typename Input>
    static void apply(const Input &in, ast::ListType& ltype) {
        ltype.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::ListType& ltype, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::ListType>(std::move(ltype));
    }
};


template<>
struct Action<TupleType> : change_states< ast::TupleType > {
    template<typename Input>
    static void apply(const Input &in, ast::TupleType& ltype) {
        ltype.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::TupleType& ltype, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::TupleType>(std::move(ltype));
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
    static void apply(const Input &in, ast::FunctionType& ftype) {
        ftype.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::FunctionType& ftype, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::FunctionType>(std::move(ftype));
    }
};


template<>
struct Action<Type> : change_states< std::unique_ptr<ast::Type> >  {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Type>& type) {
        type->source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::FunctionType& ftype) {
        ftype.result_type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::TupleType& tuple) {
        tuple.subtypes.push_back(std::move(type));
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::Parameter& par) {
        par.type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::Instance& inst) {
        inst.type_inst.push_back(std::move(type));
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, std::unique_ptr<ast::Expression>& expr) {
        // Cast
        auto cast = std::make_unique<ast::Cast>();
        cast->expression = std::move(expr);
        cast->type = std::move(type);
        expr = std::move(cast);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::Reference& ref) {
        ref.type_arg = std::move(type);
    }
};


template<>
struct Action<UnsafeType> : change_states< std::unique_ptr<ast::Type> >  {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Type>& type) {
        type->source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& inner, std::unique_ptr<ast::Type>& outer) {
        outer = std::move(inner);
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
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::TypeDef& def) {
        def.type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::TypeAlias& alias) {
        alias.type = std::move(type);
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
struct Action<DefType> : change_states< ast::TypeDef > {
    template<typename Input>
    static void success(const Input &in, ast::TypeDef& def, ast::Module& mod) {
        mod.body.statements.emplace_back(std::make_unique<ast::TypeDef>(std::move(def)));
    }
};


template<>
struct Action<Literal> : change_states< LiteralHelper > {
    template<typename Input>
    static void success(const Input &in, LiteralHelper& helper, std::unique_ptr<ast::Expression>& expr) {
        TypedValue value;
        switch (helper.type) {
            default:
            case ValueType::Unknown:
                assert(!"Literal value not handled");
                abort();
            case ValueType::Int32: {
                const auto v = std::get<int64_t>(helper.content);
                using l = std::numeric_limits<int32_t>;
                if (v < l::min() || v > l::max())
                    // The value can't fit in Int32, make it Int64 instead
                    value = TypedValue{value::Int64(v)};
                else
                    value = TypedValue{value::Int32((int32_t) v)};
                break;
            }
            case ValueType::Int64:
                value = TypedValue{value::Int64( std::get<int64_t>(helper.content) )};
                break;
            case ValueType::Float32:
                value = TypedValue{value::Float32( std::get<double>(helper.content) )};
                break;
            case ValueType::Float64:
                value = TypedValue{value::Float64( std::get<double>(helper.content) )};
                break;
            case ValueType::Char:
                value = TypedValue{value::Char( std::get<std::string>(helper.content) )};
                break;
            case ValueType::String:
                if (std::holds_alternative<std::string>(helper.content))
                    value = TypedValue{value::String( std::get<std::string>(helper.content) )};
                else
                    value = TypedValue{value::String( std::get<std::string_view>(helper.content) )};
                break;
            case ValueType::Byte: {
                if (std::holds_alternative<std::string>(helper.content))
                    value = TypedValue{value::Byte( std::get<std::string>(helper.content) )};
                else {
                    const auto v = std::get<int64_t>(helper.content);
                    using l = std::numeric_limits<uint8_t>;
                    if (v < l::min() || v > l::max())
                        throw parse_error("Byte literal out of range", in);
                    value = TypedValue{value::Byte((uint8_t) v)};
                    break;
                }
                break;
            }
            case ValueType::List: {  // [Byte]
                std::string& str = std::get<std::string>(helper.content);
                std::span data{(const std::byte*) str.data(), str.size()};
                value = TypedValue{value::Bytes(data)};
                break;
            }
        }
        expr = std::make_unique<ast::Literal>(std::move(value));
    }
};


template<>
struct Action<DecNumFrac> {
    template<typename Input>
    static void apply(const Input &in, NumberHelper& n) {
        n.is_float = true;
    }
};


template<>
struct Action<NumSuffix> {
    template<typename Input>
    static void apply(const Input &in, NumberHelper& n) {
        n.suffix = tolower(in.peek_char());
        if (n.suffix == 'f')
            n.is_float = true;
    }
};


template<>
struct Action<Number> : change_states< NumberHelper > {
    template<typename Input>
    static void apply(const Input &in, NumberHelper& n) {
        if (n.is_float) {
            auto s = in.string();
            if (n.suffix)
                s.pop_back();
            std::istringstream is(s);
            double val;
            is >> val;
            if (!is.eof()) {
                throw std::runtime_error("Float not fully parsed.");
            }
            n.num = val;
        } else {
            const auto sv = in.string_view();
            const char* first = &sv.front();
            const char* last = &sv.back() + 1;

            // skip optional + sign, which is not recognized by from_chars()
            if (*first == '+')
                ++first;

            // skip suffix
            if (n.suffix)
                --last;

            bool minus_sign = false;
            if (*first == '-') {
                minus_sign = true;
                ++first;
            }

            int base = 10;
            if (last - first >= 2 && first[0] == '0') {
                switch (first[1]) {
                    case 'b': base = 2; first += 2; break;
                    case 'o': base = 8; first += 2; break;
                    case 'x': base = 16; first += 2; break;
                }
            }

            uint64_t val;
            auto [end, ec] = std::from_chars(first, last, val, base);
            if (ec == std::errc::result_out_of_range)
                throw parse_error("Int64 literal out of range", in);
            assert(end == last);

            using l = std::numeric_limits<int64_t>;
            if (!minus_sign && val > uint64_t(l::max()))
                throw parse_error("Int64 literal out of range", in);
            if (minus_sign && val > uint64_t(l::max()) + 1)
                throw parse_error("Int64 literal out of range", in);

            n.num = minus_sign ? int64_t(~val+1) : int64_t(val);
        }
    }

    template<typename Input>
    static void success(const Input &in, NumberHelper& n, LiteralHelper& lit) {
        if (n.is_float) {
            lit.type = (n.suffix == 'f') ? ValueType::Float32 : ValueType::Float64;
            lit.content = std::get<double>(n.num);
        } else {
            switch (n.suffix) {
                default:
                case 0:
                    lit.type = ValueType::Int32;
                    break;
                case 'l':
                    lit.type = ValueType::Int64;
                    break;
                case 'b':
                    lit.type = ValueType::Byte;
                    break;
            }
            lit.content = std::get<int64_t>(n.num);
        }
    }
};


template<>
struct Action<Char> : change_states< std::string > {
    template<typename Input>
    static void success(const Input &in, std::string& str, LiteralHelper& helper) {
        helper.content = std::move(str);
        helper.type = ValueType::Char;
    }
};


template<>
struct Action<String> : change_states< std::string > {
    template<typename Input>
    static void success(const Input &in, std::string& str, LiteralHelper& helper) {
        helper.content = std::move(str);
        helper.type = ValueType::String;
    }
};


template<>
struct Action<Byte> {
    template<typename Input>
    static void apply(const Input &in, LiteralHelper& helper) {
        helper.type = ValueType::Byte;
    }
};


template<>
struct Action<Bytes> {
    template<typename Input>
    static void apply(const Input &in, LiteralHelper& helper) {
        helper.type = ValueType::List;  // [Byte]
    }
};


template<> struct Action<StringChOther> : StringAppendChar {};
template<> struct Action<StringChEscSingle> : StringAppendEscSingle {};
template<> struct Action<StringChEscHex> : StringAppendEscHex {};
template<> struct Action<StringChEscOct> : StringAppendEscOct {};

template<> struct Action<RawStringCh> : StringAppendChar {};

template<>
struct Action<EscapedQuotes> {
    template<typename Input>
    static void apply(const Input &in, std::string& str) {
        assert(in.size() >= 4);
        str.append(in.begin() + 1, in.end());
    }
};

template<>
struct Action<RawString> : change_states< std::string > {
    template<typename Input>
    static void success(const Input &in, std::string& str, LiteralHelper& helper) {
        helper.content = strip_raw_string(move(str));
        helper.type = ValueType::String;
    }
};


template<>
struct Action<RawBytes> {
    template<typename Input>
    static void apply(const Input &in, LiteralHelper& helper) {
        helper.type = ValueType::List;  // [Byte]
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
template<> const std::string Control<Expression<SC>>::errmsg = "expected expression";
template<> const std::string Control<Expression<NSC>>::errmsg = "expected expression";
template<> const std::string Control<DeclParams>::errmsg = "expected function parameter declaration";
template<> const std::string Control<ExprInfixRight<SC>>::errmsg = "expected infix operator";
template<> const std::string Control<ExprInfixRight<NSC>>::errmsg = "expected infix operator";
template<> const std::string Control<Variable>::errmsg = "expected variable name";
template<> const std::string Control<UnsafeType>::errmsg = "expected type";
template<> const std::string Control<Type>::errmsg = "expected type";
template<> const std::string Control<TypeName>::errmsg = "expected type name";
template<> const std::string Control<StringContent>::errmsg = "unclosed string literal";
template<> const std::string Control<RawStringContent>::errmsg = "unclosed raw string literal";

// default message
template< typename T >
const std::string Control< T >::errmsg = "parse error matching " + std::string(tao::demangle< T >());


// ----------------------------------------------------------------------------

} // namespace parser


void Parser::parse(SourceId src_id, ast::Module& mod)
{
    using parser::Module;
    using parser::Action;
    using parser::Control;

    const auto& src = m_source_manager.get_source(src_id);

    tao::pegtl::memory_input<
        tao::pegtl::tracking_mode::eager,
        tao::pegtl::eol::lf_crlf,
        SourceRef>
    //SourceRef{m_source_manager, src_id}
    in(src.data(), src.size(), SourceRef{m_source_manager, src_id});

    try {
        if (!tao::pegtl::parse< Module, Action, Control >( in, mod ))
            throw ParseError("input not matched");
        mod.body.finish();
    } catch (tao::pegtl::parse_error& e) {
        SourceLocation loc;
        loc.load(in, e.positions().front());
        throw ParseError(e.message(), loc);
    } catch( const std::exception& e ) {
        throw ParseError(e.what());
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
