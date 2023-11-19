// Parser.cpp created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Parser.h"
#include "Error.h"
#include <xci/core/parser/raw_string.h>
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
struct Reference;

// Spaces and comments
struct LineComment: seq< two<'/'>, until<eolf> > {};
struct BlockComment: if_must< string<'/', '*'>, until< string<'*', '/'>, any > > {};
struct EscapedNewline: seq< one<'\\'>, eol> {};
struct SpaceOrComment: sor< blank, EscapedNewline, BlockComment> {};
struct NlSpaceOrComment: sor< space, EscapedNewline, LineComment, BlockComment > {};
struct SemicolonOrNewline: sor<one<';'>, eol, LineComment> {};
struct SC: star< SpaceOrComment > {};  // optional space or comments
struct NSC: star< NlSpaceOrComment > {};  // optional newlines, space or comments
struct RS: at<space> {};  // require space
struct RSP: at< sor<space, one<'('>> > {};  // require space or paren

// Aux templates
template<class T> struct SepList: list_tail<T, seq<SC, SemicolonOrNewline, NSC> > {};  // list separated by either semicolon or newline

// Basic tokens
//                 (underscore* lower identifier_other* | underscore+ digit+)
struct Identifier: seq< not_at<Keyword>, sor< seq<star<one<'_'>>, lower, star<identifier_other>>,
                                              seq<plus<one<'_'>>, plus<digit>> > > {};
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
struct KeywordClass: TAO_PEGTL_KEYWORD("class") {};
struct KeywordInstance: TAO_PEGTL_KEYWORD("instance") {};
struct KeywordType: TAO_PEGTL_KEYWORD("type") {};
struct KeywordDecl: TAO_PEGTL_KEYWORD("decl") {};
struct KeywordWith: TAO_PEGTL_KEYWORD("with") {};
struct KeywordIf: TAO_PEGTL_KEYWORD("if") {};
struct KeywordThen: TAO_PEGTL_KEYWORD("then") {};
struct KeywordElse: TAO_PEGTL_KEYWORD("else") {};
struct KeywordMatch: TAO_PEGTL_KEYWORD("match") {};
struct Keyword: sor<KeywordFun, KeywordClass, KeywordInstance, KeywordType,
        KeywordDecl, KeywordWith, KeywordIf, KeywordThen, KeywordElse, KeywordMatch> {};

// Literals
template<class D> struct UPlus: seq< D, star<opt<one<'_'>>, D> > {};  // one or more digits D interleaved by underscores ('_')
struct Sign: one<'-','+'> {};
struct BinDigit : one< '0', '1' > {};
struct BinNum: seq<one<'b'>, UPlus<BinDigit>> {};
struct OctNum: if_must<one<'o'>, UPlus<odigit>> {};
struct DecNumFrac: seq< one<'.'>, opt<UPlus<digit>> > {};
struct DecNumExp: seq< one<'e'>, opt<Sign>, must<UPlus<digit>> > {};
struct DecNum: seq< UPlus<digit>, opt<DecNumFrac>, opt<DecNumExp> > {};
struct HexNum: if_must<one<'x'>, UPlus<xdigit>> {};
struct ZeroPrefixNum: seq< one<'0'>, sor<HexNum, OctNum, BinNum, DecNum> > {};
struct NumSuffix: sor<
        seq<one<'u'>, sor<one<'h', 'd', 'l', 'q'>, opt<digit, opt<digit>, opt<digit>>>>,
        seq<one<'i'>, seq<digit, opt<digit>, opt<digit>>>,
        seq<one<'f'>, opt<digit, opt<digit>, opt<digit>>>,
        one<'b', 'c', 'h', 'd', 'l', 'q'> > {};
struct Number: seq< opt<Sign>, sor<ZeroPrefixNum, DecNum>, opt<opt<one<'_'>>, NumSuffix> > {};

struct Char: if_must< one<'\''>, StringChUni, one<'\''> > {};
struct StringContent: until< one<'"'>, StringChUni > {};
struct String: if_must< one<'"'>, StringContent > {};
struct EscapedQuotes: seq<one<'\\'>, three<'"'>, star<one<'"'>>> {};
struct RawStringCh: any {};
struct RawStringContent: until<three<'"'>, sor<EscapedQuotes, RawStringCh>> {};
struct RawString : if_must< three<'"'>, RawStringContent > {};
struct Byte: if_must< seq< one<'b'>, one<'\''> >, StringCh, one<'\''> > {};
struct BytesContent: until< one<'"'>, StringCh > {};
struct Bytes: if_must< seq< one<'b'>, one<'"'> >, BytesContent > {};
struct RawBytes: seq< one<'b'>, RawString > {};
struct Literal: sor< Char, RawString, String, Byte, RawBytes, Bytes, Number > {};

// Types
struct DeclParam: sor< Type, seq< Identifier, opt<SC, one<':'>, SC, must<Type> > > > {};
struct DeclResult: seq< string<'-', '>'>, SC, must<Type>, opt<SC, DeclResult> > {};
struct TypeParams: seq< one<'<'>, must<TypeName>, SC, star<one<','>, SC, must<TypeName>, SC>, must<one<'>'>> > {};
struct TypeConstraint: seq<TypeName, RS, SC, TypeName> {};
struct TypeContext: seq< one<'('>, SC, must<TypeConstraint>, SC, star<one<','>, SC, must<TypeConstraint>, SC>, must<one<')'>> > {};
struct FunctionType: seq< opt<TypeParams>, SC, DeclParam, SC, DeclResult > {};
struct FunctionDecl: seq< opt<TypeParams>, SC, DeclParam, SC, opt<DeclResult>, SC, opt<KeywordWith, SC, must<TypeContext>> > {};
struct PlainTypeName: seq< TypeName, not_at<SC, one<','>> > {};  // not followed by comma (would be TupleType)
struct ListType: seq< one<'['>, SC, must<UnsafeType>, SC, must<one<']'>> > {};
struct TupleType: seq< Type, plus<SC, one<','>, SC, Type> > {};
struct StructItem: seq< Identifier, opt<SC, one<':'>, SC, must<Type>> > {};
struct StructType: seq< StructItem, star<SC, opt<one<','>>, NSC, StructItem>, opt<SC, one<','>> > {};
struct ParenthesizedType: seq< one<'('>, NSC, opt<UnsafeType, NSC>, must<one<')'>> > {};
struct UnsafeType: sor<FunctionType, PlainTypeName, TupleType, StructType, ParenthesizedType, ListType> {};   // usable in context where Type is already expected
struct Type: sor< ParenthesizedType, ListType, TypeName > {};

// Expressions
// * some rules are parametrized with S (space type), choose either SC or NSC (allow newline)
// * in general, rules inside parentheses or brackets use NSC, rules outside use SC
// * this allows leaving out semicolons but still support multiline expressions
template<class S> struct CallRight: seq< RSP, S, ExprArgSafe > {};
template<class S> struct Call: seq< ExprCallable, RSP, S, ExprArgSafe > {};
template<class S> struct TypeDotCallRight: seq< Reference, opt<RSP, S, ExprArgSafe> > {};
template<class S> struct ExprTypeDotCall: seq< TypeName, S, seq< one<'.'>, SC, must<TypeDotCallRight<S>> > > {};
template<class S> struct ExprTypeInit: seq< TypeName, RSP, S, ExprArgSafe > {};
template<class S> struct ExprOperand: sor<Call<S>, ExprArgSafe, ExprPrefix, ExprTypeInit<S>, ExprTypeDotCall<S>> {};
template<class S> struct DotCallRightExpr: sor<TypeName, ExprOperand<S>> {};
template<class S> struct DotCallRight: seq< NSC, one<'.'>, SC, must<DotCallRightExpr<S>> > {};
template<class S> struct ExprInfixRight: seq< sor< CallRight<S>, DotCallRight<S>,
                                seq<S, InfixOperator, NSC, ExprOperand<S>> >, opt< ExprInfixRight<S> > > {};
template<class S> struct TrailingComma: opt<S, one<','>> {};
template<class S> struct ExprInfix: seq< ExprOperand<S>, opt<ExprInfixRight<S>>, TrailingComma<S> > {};
template<class S> struct Expression: sor< ExprCond, ExprWith, ExprStruct, ExprInfix<S> > {};
struct Variable: seq< Identifier, opt<SC, one<':'>, SC, must<UnsafeType> > > {};
struct Block: seq< one<'{'>, NSC, sor< one<'}'>, must<SepList<Statement>, NSC, one<'}'>> > > {};
struct Function: sor< Block, if_must< KeywordFun, NSC, FunctionDecl, NSC, Block> > {};
struct ParenthesizedExpr: seq< one<'('>, NSC, sor< one<')'>, must<Expression<NSC>, NSC, one<')'>> > > {};
struct ExprPrefix: seq< PrefixOperator, SC, must<ExprOperand<SC>>, SC > {};
struct TypeArgs: seq< one<'<'>, Type, SC, star<one<','>, SC, must<Type>, SC>, one<'>'> > {};
struct Reference: seq<Identifier, opt<TypeArgs>, not_at<one<'"'>>> {};
struct List: seq< one<'['>, NSC, sor< one<']'>, must<ExprInfix<NSC>, NSC, one<']'>> > > {};
struct Cast: seq<SC, one<':'>, SC, Type> {};
struct ExprCallable: sor< ParenthesizedExpr, Function, Reference> {};
struct ExprArgSafe: seq< sor< ParenthesizedExpr, List, Function, Literal, Reference >, opt<Cast>> {};  // expressions which can be used as args in Call
struct ExprCondIf: if_must<KeywordIf, NSC, ExprInfix<NSC>, NSC, KeywordThen, NSC, Expression<SC>> {};
struct ExprCondElse: seq< KeywordElse, NSC, must<Expression<SC>> > {};
struct ExprCond: seq< plus< ExprCondIf, NSC >, ExprCondElse> {};
struct ExprWith: if_must< KeywordWith, NSC, ExprArgSafe, NSC, Expression<SC> > {};  // could be parsed as a function, but that wouldn't allow newlines
struct ExprStructItem: seq< Identifier, SC, one<'='>, not_at<one<'='>>, SC, must<ExprArgSafe> > {};
struct ExprStruct: seq< ExprStructItem, star< SC, one<','>, SC, must<ExprStructItem> > > {};

// Block-level statements
struct Declaration: seq< KeywordDecl, NSC, must<Variable> > {};
struct Definition: seq< Variable, SC, seq<one<'='>, not_at<one<'='>>, NSC, must<Expression<SC>>> > {};  // must not match `var == ...`
struct TypeAlias: seq< TypeName, NSC, one<'='>, not_at<one<'='>>, NSC, must<UnsafeType> > {};
struct Statement: sor< Declaration, Definition, TypeAlias, Expression<SC> > {};

// Module-level definitions
struct ClassDeclaration: seq< Variable, SC, opt<one<'='>, SC, must<Expression<SC>>> > {};  // with optional default definition
struct DefClass: seq< KeywordClass, NSC, must<TypeName>, RS, SC, must<plus<TypeName, SC>>,
                      opt<TypeContext>, NSC, must<one<'{'>>, NSC,
                      sor< one<'}'>, must<SepList<ClassDeclaration>, NSC, one<'}'>> > > {};
struct DefInstance: seq< KeywordInstance, SC, opt<TypeParams>, NSC, must<TypeName>, RS, SC,    // instance<T> Init
                         must<plus<Type, SC>>, opt<TypeContext>, NSC, must<one<'{'>>, NSC,     // Int T (Eq T) {
                         sor< one<'}'>, must<SepList<Definition>, NSC, one<'}'>> > > {};       // init = ... }
struct DefType: if_must< KeywordType, NSC, TypeName, NSC, one<'='>, NSC, UnsafeType > {};
struct TopLevelStatement: sor<DefClass, DefInstance, DefType, Statement> {};

// Source module
struct Module: seq<NSC, sor<eof, must<SepList<TopLevelStatement>, NSC, eof>>> {};


// ----------------------------------------------------------------------------
// Helper data structs

struct LiteralHelper {
    std::variant<std::string_view, std::string, double, uint64_t, int64_t> content;
    ValueType type = ValueType::Unknown;
};

struct NumberHelper {
    union {
        double f;  // any float
        int64_t i;  // any int
        uint64_t u;  // any unsigned int
    };
    uint8_t suffix_length = 0;
    uint8_t bits = 64;
    enum : uint8_t { Signed, Unsigned, Float } type = Signed;
    bool is_float() const noexcept { return type == Float; }
    bool is_unsigned() const noexcept { return type == Unsigned; }
};

// temporary specialized variant of ast::Call
struct RefCall {
    std::unique_ptr<ast::Reference> ref;  // callable
    std::unique_ptr<ast::TypeName> type;
    std::unique_ptr<ast::Expression> arg;
    SourceLocation source_loc;

    std::unique_ptr<ast::Call> to_ast() && {
        auto call = std::make_unique<ast::Call>();
        ref->type_args.insert(ref->type_args.begin(), std::move(type));
        call->callable = std::move(ref);
        call->arg = std::move(arg);
        call->source_loc = source_loc;
        return call;
    }
};

// ----------------------------------------------------------------------------
// Actions

template<typename Rule>
struct Action : nothing<Rule> {};


// AST doesn't have a special node for Declaration, it's just a Definition
// without an expression. This simplifies processing as the name and type part
// of both Declaration and Definition is handled in the same way, anyway.
template<>
struct Action<Declaration> : change_states< ast::Definition > {
    template<typename Input>
    static void success(const Input &in, ast::Definition& def, ast::Module& mod) {
        mod.body.statements.push_back(std::make_unique<ast::Definition>(std::move(def)));
    }

    template<typename Input>
    static void success(const Input &in, ast::Definition& def, ast::Block& block) {
        block.statements.push_back(std::make_unique<ast::Definition>(std::move(def)));
    }
};


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
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Condition::IfThen& if_then) {
        if_then.second = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Condition& cnd) {
        cnd.else_expr = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Parenthesized& parenthesized) {
        parenthesized.expression = std::move(expr);
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
        opc.op = ast::Operator{in.string_view(), true};
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
        assert(!outer.arg);
        outer.arg = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, ast::OpCall& inner, ast::Call& outer) {
        assert(!outer.arg);
        outer.arg = std::make_unique<ast::OpCall>(std::move(inner));
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
        opc.op = ast::Operator{in.string_view()};
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
            assert(!left.right_arg);
            left.right_arg = std::move(right.arg);
            left.right_tmp = std::move(right.right_tmp);
            left.source_loc = right.source_loc;
            while (left.right_tmp) {
                auto expr_tmp = std::make_unique<ast::OpCall>(std::move(left));
                left = std::move(*expr_tmp->right_tmp);
                expr_tmp->right_tmp.reset();
                assert(!left.right_arg);
                left.right_arg = std::move(left.arg);
                left.arg = std::move(expr_tmp);
            }
        } else {
            if (left.op.precedence() - int(left.op.is_right_associative()) < right.op.precedence()) {
                // Eg. 1+2*3 :: left='+(2)', right='*(3)'
                //           => left='+(right), right='*(2 3)'
                assert(!right.right_arg);
                right.right_arg = std::move(right.arg);
                assert(!left.right_arg && left.arg);
                right.arg = std::move(left.arg);
                while (right.right_tmp && left.op.precedence() < right.right_tmp->op.precedence()) {
                    // Eg. 1 || 2*3-4 :: left='||(?right)', right='*(2 3), tmp='-(4)'
                    //                => left='||(?right)', right='-(*(2 3) 4)'
                    auto expr_tmp = std::make_unique<ast::OpCall>(std::move(right));
                    right = std::move(*expr_tmp->right_tmp);
                    expr_tmp->right_tmp.reset();
                    assert(!right.right_arg && right.arg);
                    right.right_arg = std::move(right.arg);
                    right.arg = std::move(expr_tmp);
                }
                left.right_tmp = std::move(right.right_tmp);
                left.arg = std::make_unique<ast::OpCall>(std::move(right));
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
    static void success(const Input &in, ast::OpCall& opc, ast::Condition::IfThen& if_then) {
        if_then.first = prepare_expression(opc);
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
            assert(opc.arg && !opc.right_arg);
            opc.arg->source_loc = opc.source_loc;
            return std::move(opc.arg);
        } else {
            return std::make_unique<ast::OpCall>(std::move(opc));
        }
    }
};


template<>
struct Action<ExprCondIf> : change_states< ast::Condition::IfThen > {
    template<typename Input>
    static void apply(const Input &in, ast::Condition::IfThen& if_then) {
        if_then.first->source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Condition::IfThen& if_then, ast::Condition& cond) {
        cond.if_then_expr.emplace_back(std::move(if_then));
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
struct Action<ParenthesizedExpr> : change_states< ast::Parenthesized > {
    template<typename Input>
    static void apply(const Input &in, ast::Parenthesized& parenthesized) {
        parenthesized.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Parenthesized& parenthesized, std::unique_ptr<ast::Expression>& expr) {
        if (!parenthesized.expression) {
            parenthesized.expression = std::make_unique<ast::Tuple>();  // "()" => empty tuple (void)
            parenthesized.expression->source_loc = parenthesized.source_loc;
        }
        expr = std::make_unique<ast::Parenthesized>(std::move(parenthesized));
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
        assert(!call.arg);
        call.arg = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::OpCall& opc) {
        assert(!opc.arg);
        opc.arg = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, RefCall& rcall) {
        assert(!rcall.arg);
        rcall.arg = std::move(expr);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Expression>& expr, ast::Cast& cast) {
        assert(!cast.expression);
        cast.expression = std::move(expr);
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

    template<typename Input>
    static void success(const Input &in, ast::Reference& ref, RefCall& rcall) {
        rcall.ref = std::make_unique<ast::Reference>(std::move(ref));
    }
};


template<class S>
struct Action<CallRight<S>> : change_states< ast::Call > {
    template<typename Input>
    static void apply(const Input &in, ast::Call& call) {
        call.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Call& call, ast::OpCall& outer_opc) {
        auto expr = std::make_unique<ast::Call>(std::move(call));
        assert(!outer_opc.arg);
        outer_opc.arg = std::move(expr);
        outer_opc.op = ast::Operator::Call;
    }
};


template<class S>
struct Action<Call<S>> : change_states< ast::Call > {
    template<typename Input>
    static void apply(const Input &in, ast::Call& call) {
        call.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Call& call, ast::OpCall& outer_opc) {
        assert(!outer_opc.arg);
        outer_opc.arg = std::make_unique<ast::Call>(std::move(call));
    }
};


template<class S>
struct Action<DotCallRight<S>> {
    template<typename Input>
    static void apply(const Input &in, ast::OpCall& opc) {
        opc.op = ast::Operator::DotCall;
    }
};


template<class S>
struct Action<ExprTypeInit<S>> : change_states< ast::Cast > {
    template<typename Input>
    static void apply(const Input &in, ast::Cast& init) {
        init.source_loc.load(in.input(), in.position());
        init.is_init = true;
    }

    template<typename Input>
    static void success(const Input &in, ast::Cast& init, std::unique_ptr<ast::Expression>& expr) {
        expr = std::make_unique<ast::Cast>(std::move(init));
    }

    template<typename Input>
    static void success(const Input &in, ast::Cast& init, ast::OpCall& outer) {
        assert(!outer.arg);
        outer.arg = std::make_unique<ast::Cast>(std::move(init));
    }
};


template<class S>
struct Action<ExprTypeDotCall<S>> : change_states< RefCall > {
    template<typename Input>
    static void apply(const Input &in, RefCall& rcall) {
        rcall.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, RefCall& rcall, std::unique_ptr<ast::Expression>& expr) {
        expr = std::move(rcall).to_ast();
    }

    template<typename Input>
    static void success(const Input &in, RefCall& rcall, ast::OpCall& outer) {
        assert(!outer.arg);
        outer.arg = std::move(rcall).to_ast();
    }
};


template<>
struct Action<Identifier> : change_states< ast::Identifier > {
    template<typename Input>
    static void apply(const Input &in, ast::Identifier& ident) {
        ident.name = intern(in.string());
        ident.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::StructItem& item) {
        item.identifier = ident;
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::Variable& var) {
        var.identifier = ident;
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::Parameter& par) {
        par.identifier = ident;
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::Reference& ref) {
        ref.identifier = ident;
    }

    template<typename Input>
    static void success(const Input &in, ast::Identifier& ident, ast::StructInit& node) {
        node.items.emplace_back(ident, std::unique_ptr<ast::Expression>{});
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
struct Action<TypeParams> : change_states< std::vector<ast::TypeName> > {
    template<typename Input>
    static void success(const Input &in, std::vector<ast::TypeName>& type_params, ast::FunctionType& ftype) {
        ftype.type_params = std::move(type_params);
    }

    template<typename Input>
    static void success(const Input &in, std::vector<ast::TypeName>& type_params, ast::Instance& inst) {
        inst.type_params = std::move(type_params);
    }
};


template<>
struct Action<TypeName> : change_states< ast::TypeName > {
    template<typename Input>
    static void apply(const Input &in, ast::TypeName& tname) {
        tname.name = intern(in.string());
        tname.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::TypeName>(std::move(tname));
    }


    template<typename Input>                                   // Cast
    static void success(const Input &in, ast::TypeName& tname, RefCall& rcall) {
        rcall.type = std::make_unique<ast::TypeName>(std::move(tname));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::Cast& cast) {
        assert(!cast.type);
        cast.type = std::make_unique<ast::TypeName>(std::move(tname));
    }

    template<typename Input>                                   // "DotTypeInit"
    static void success(const Input &in, ast::TypeName& tname, ast::OpCall& outer) {
        auto init = std::make_unique<ast::Cast>();
        init->is_init = true;
        init->type = std::make_unique<ast::TypeName>(std::move(tname));
        outer.arg = std::move(init);
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, std::vector<ast::TypeName>& type_params) {
        type_params.push_back(std::move(tname));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::TypeConstraint& tcst) {
        if (!tcst.type_class)
            tcst.type_class = std::move(tname);
        else
            tcst.type_name = std::move(tname);
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::Class& cls) {
        if (!cls.class_name)
            cls.class_name = std::move(tname);
        else
            cls.type_vars.push_back(std::move(tname));
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::Instance& inst) {
        inst.class_name = std::move(tname);
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::TypeDef& def) {
        def.type_name = std::move(tname);
    }

    template<typename Input>
    static void success(const Input &in, ast::TypeName& tname, ast::TypeAlias& alias) {
        alias.type_name = std::move(tname);
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
struct Action<StructType> : change_states< ast::StructType > {
    template<typename Input>
    static void apply(const Input &in, ast::StructType& ltype) {
        ltype.source_loc.load(in.input(), in.position());
    }

    template<typename Input>
    static void success(const Input &in, ast::StructType& ltype, std::unique_ptr<ast::Type>& type) {
        type = std::make_unique<ast::StructType>(std::move(ltype));
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
        if (!type)
            type = std::make_unique<ast::TupleType>();  // "()" => empty tuple type (Void)
        type->source_loc.load(in.input(), in.position());
    }

    template<typename Input>                                               // DeclResult
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, std::unique_ptr<ast::Type>& outer_type) {
        assert(!outer_type);
        outer_type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::TupleType& tuple) {
        tuple.subtypes.push_back(std::move(type));
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::StructItem& item) {
        item.type = std::move(type);
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
        ref.type_args.push_back(std::move(type));
    }
};


template<>
struct Action<UnsafeType> : change_states< std::unique_ptr<ast::Type> >  {
    template<typename Input>
    static void apply(const Input &in, std::unique_ptr<ast::Type>& type) {
        if (!type)
            type = std::make_unique<ast::TupleType>();  // "()" => empty tuple type (Void)
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
struct Action<StructItem> : change_states< ast::StructItem > {
    template<typename Input>
    static void success(const Input &in, ast::StructItem& item, ast::StructType& stype) {
        stype.subtypes.push_back(std::move(item));
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
struct Action<DeclParam> : change_states< ast::Parameter > {
    template<typename Input>
    static void success(const Input &in, ast::Parameter& par, ast::FunctionType& ftype) {
        ftype.param = std::move(par);
    }
};

template<>
struct Action<DeclResult> : change_states< std::unique_ptr<ast::Type> > {
    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, ast::FunctionType& ftype) {
        ftype.return_type = std::move(type);
    }

    template<typename Input>
    static void success(const Input &in, std::unique_ptr<ast::Type>& type, std::unique_ptr<ast::Type>& outer_type) {
        if (outer_type) {
            // cascading function type: A -> B -> C   =>  A -> (B -> C))
            // `outer_type` is original return type, which becomes parameter (B)
            // `type` becomes tailing return type (C)
            auto r = std::make_unique<ast::FunctionType>();
            r->param.type = std::move(outer_type);
            r->return_type = std::move(type);
            outer_type = std::move(r);
        } else {
            outer_type = std::move(type);
        }
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
struct Action<ClassDeclaration> : change_states< ast::Definition > {
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
            case ValueType::UInt8: {
                if (std::holds_alternative<std::string>(helper.content))
                    value = TypedValue{value::UInt8( std::get<std::string>(helper.content) )};
                else {
                    const auto v = std::get<uint64_t>(helper.content);
                    using l = std::numeric_limits<uint8_t>;
                    if (v > l::max())
                        throw tao::pegtl::parse_error("Integer literal out of range (UInt8)", in);
                    value = TypedValue{value::UInt8((uint8_t) v)};
                    break;
                }
                break;
            }
            case ValueType::UInt16: {
                const auto v = std::get<uint64_t>(helper.content);
                using l = std::numeric_limits<uint16_t>;
                if (v > l::max())
                    throw tao::pegtl::parse_error("Integer literal out of range (UInt16)", in);
                value = TypedValue{value::UInt16((uint16_t) v)};
                break;
            }
            case ValueType::UInt32: {
                const auto v = std::get<uint64_t>(helper.content);
                using l = std::numeric_limits<uint32_t>;
                if (v > l::max())
                    throw tao::pegtl::parse_error("Integer literal out of range (UInt32)", in);
                value = TypedValue{value::UInt32((uint32_t) v)};
                break;
            }
            case ValueType::UInt64:
                value = TypedValue{value::UInt64( std::get<uint64_t>(helper.content) )};
                break;
            case ValueType::UInt128:
                value = TypedValue{value::UInt128( std::get<uint64_t>(helper.content) )};
                break;
            case ValueType::Int8: {
                const auto v = std::get<int64_t>(helper.content);
                using l = std::numeric_limits<int8_t>;
                if (v < l::min() || v > l::max())
                    throw tao::pegtl::parse_error("Integer literal out of range (Int8)", in);
                value = TypedValue{value::Int8((int8_t) v)};
                break;
            }
            case ValueType::Int16: {
                const auto v = std::get<int64_t>(helper.content);
                using l = std::numeric_limits<int16_t>;
                if (v < l::min() || v > l::max())
                    throw tao::pegtl::parse_error("Integer literal out of range (Int16)", in);
                value = TypedValue{value::Int16((int16_t) v)};
                break;
            }
            case ValueType::Int32: {
                const auto v = std::get<int64_t>(helper.content);
                using l = std::numeric_limits<int32_t>;
                if (v < l::min() || v > l::max())
                    throw tao::pegtl::parse_error("Integer literal out of range (Int32)", in);
                value = TypedValue{value::Int32((int32_t) v)};
                break;
            }
            case ValueType::Int64:
                value = TypedValue{value::Int64( std::get<int64_t>(helper.content) )};
                break;
            case ValueType::Int128:
                value = TypedValue{value::Int128( std::get<int64_t>(helper.content) )};
                break;
            case ValueType::Float32:
                value = TypedValue{value::Float32( std::get<double>(helper.content) )};
                break;
            case ValueType::Float64:
                value = TypedValue{value::Float64( std::get<double>(helper.content) )};
                break;
            case ValueType::Float128:
                value = TypedValue{value::Float128( std::get<double>(helper.content) )};
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
            case ValueType::List: {  // [Byte]
                auto& str = std::get<std::string>(helper.content);
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
        n.type = NumberHelper::Float;
    }
};


template<>
struct Action<DecNumExp> {
    template<typename Input>
    static void apply(const Input &in, NumberHelper& n) {
        n.type = NumberHelper::Float;
    }
};


template<>
struct Action<NumSuffix> {
    template<typename Input>
    static void apply(const Input &in, NumberHelper& n) {
        std::string_view sv = in.string_view();
        n.suffix_length = uint8_t(sv.size());
        char suffix = sv.front();
        sv.remove_prefix(1);
        if (strchr("uibchd", suffix) != nullptr) {
            if (n.is_float())
                throw tao::pegtl::parse_error("Integer suffix on float literal", in);
            assert(n.type == NumberHelper::Signed);
        }
        if (suffix == 'u') {
            n.type = NumberHelper::Unsigned;
            if (!sv.empty() && !isdigit((int)(unsigned char)sv.front())) {
                suffix = sv.front();
                sv.remove_prefix(1);
            }
        }
        switch (suffix) {
            case 'u':
            case 'i':
                std::from_chars(sv.data(), sv.data() + sv.size(), n.bits);
                if (n.bits != 8 && n.bits != 16 && n.bits != 32 && n.bits != 64 && n.bits != 128)
                    throw tao::pegtl::parse_error("Invalid bit length of integer literal", in);
                break;
            case 'b':
                n.type = NumberHelper::Unsigned;
                [[ fallthrough ]];
            case 'c':
                n.bits = 8;
                break;
            case 'h':
                n.bits = 16;
                break;
            case 'd':
                n.bits = 32;
                break;
            case 'l':
                n.bits = 64;
                break;
            case 'q':
                n.bits = 128;
                break;
            case 'f':
                n.type = NumberHelper::Float;
                if (!sv.empty()) {
                    std::from_chars(sv.data(), sv.data() + sv.size(), n.bits);
                    if (n.bits != 32 && n.bits != 64 && n.bits != 128)
                        throw tao::pegtl::parse_error("Invalid bit length of float literal", in);
                } else {
                    n.bits = 32;
                }
                break;
        }
    }
};


template<>
struct Action<Number> : change_states< NumberHelper > {
    template<typename Input>
    static void apply(const Input &in, NumberHelper& n) {
        auto s = in.string();
        if (n.suffix_length != 0)
            s.erase(s.size() - n.suffix_length);
        std::erase(s, '_');

        if (n.is_float()) {
            std::istringstream is(s);
            is >> n.f;
            if (!is.eof()) {
                throw tao::pegtl::parse_error("Float not fully parsed.", in);
            }
        } else {
            std::string_view sv = s;

            // skip optional + sign, which is not recognized by from_chars()
            if (sv.front() == '+')
                sv.remove_prefix(1);

            bool minus_sign = false;
            if (sv.front() == '-') {
                minus_sign = true;
                sv.remove_prefix(1);
            }

            int base = 10;
            if (sv.size() > 2 && sv[0] == '0') {
                switch (sv[1]) {
                    case 'b': base = 2; sv.remove_prefix(2); break;
                    case 'o': base = 8; sv.remove_prefix(2); break;
                    case 'x': base = 16; sv.remove_prefix(2); break;
                }
            }

            uint64_t val = 0;
            auto [end, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val, base);
            if (ec == std::errc::result_out_of_range)
                throw tao::pegtl::parse_error("Integer literal out of range", in);
            assert(end == sv.data() + sv.size());

            if (n.is_unsigned()) {
                // wraparound for minus sign, e.g. -1b = 255b
                n.u = minus_sign ? (n.bits >= 64 ? -val : -val + (1ull << n.bits))
                                 : val;
            } else {
                using l = std::numeric_limits<int64_t>;
                if (!minus_sign && val > uint64_t(l::max()))
                    throw tao::pegtl::parse_error("Integer literal out of range", in);
                if (minus_sign && val > uint64_t(l::max()) + 1)
                    throw tao::pegtl::parse_error("Integer literal out of range", in);

                n.i = minus_sign ? int64_t(~val+1) : int64_t(val);
            }
        }
    }

    template<typename Input>
    static void success(const Input &in, NumberHelper& n, LiteralHelper& lit) {
        if (n.is_float()) {
            switch (n.bits) {
                case 128:
                    lit.type = ValueType::Float128;
                    break;
                default:
                case 64:
                    lit.type = ValueType::Float64;
                    break;
                case 32:
                    lit.type = ValueType::Float32;
                    break;
            }
            lit.content = n.f;
        } else {
            switch (n.bits) {
                case 128:
                    lit.type = n.is_unsigned() ? ValueType::UInt128 : ValueType::Int128;
                    break;
                default:
                case 64:
                    lit.type = n.is_unsigned() ? ValueType::UInt64 : ValueType::Int64;
                    break;
                case 32:
                    lit.type = n.is_unsigned() ? ValueType::UInt32 : ValueType::Int32;
                    break;
                case 16:
                    lit.type = n.is_unsigned() ? ValueType::UInt16 : ValueType::Int16;
                    break;
                case 8:
                    lit.type = n.is_unsigned() ? ValueType::UInt8 : ValueType::Int8;
                    break;
            }
            if (n.is_unsigned())
                lit.content = n.u;
            else
                lit.content = n.i;
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
struct Action<Byte> : change_states< std::string > {
    template<typename Input>
    static void success(const Input &in, std::string& str, LiteralHelper& helper) {
        helper.content = std::move(str);
        helper.type = ValueType::Byte;
    }
};


template<>
struct Action<Bytes> : change_states< std::string > {
    template<typename Input>
    static void success(const Input &in, std::string& str, LiteralHelper& helper) {
        helper.content = std::move(str);
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
        helper.content = core::parser::strip_raw_string(std::move(str));
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
    static const char* errmsg;

    template< typename Input, typename... States >
    static void raise( const Input& in, States&&... /*unused*/ ) {
        if (errmsg == nullptr) {
#ifndef NDEBUG
            throw tao::pegtl::parse_error( "parse error matching " + std::string(demangle<Rule>()), in );
#else
            throw tao::pegtl::parse_error( "parse error", in );
#endif
        }
        throw tao::pegtl::parse_error( errmsg, in );
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
        auto rule = demangle<Rule>();
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
        auto rule = demangle<Rule>();
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
        auto rule = demangle<Rule>();
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
        std::cerr << in.position() << "  apply  " << demangle<Rule>() << std::endl;
        return normal<Rule>::template apply< Action >( begin, in, st... );
    }

    template< template< typename... > class Action, typename Input, typename... States >
    static auto apply0( const Input& in, States&&... st )
    -> decltype( normal<Rule>::template apply0< Action >( in, st... ) )
    {
        std::cerr << in.position() << "  apply0 " << demangle<Rule>() << std::endl;
        return normal<Rule>::template apply0< Action >( in, st... );
    }

#endif
};

template<> const char* Control<eof>::errmsg = "invalid syntax";
template<> const char* Control<until< string<'*', '/'>, any >>::errmsg = "unterminated comment";
template<> const char* Control<UPlus<digit>>::errmsg = "expected digit";
template<> const char* Control<UPlus<odigit>>::errmsg = "expected oct digit";
template<> const char* Control<xdigit>::errmsg = "expected hex digit";
template<> const char* Control<UPlus<xdigit>>::errmsg = "expected hex digit";
template<> const char* Control<rep_min_max<1,6,xdigit>>::errmsg = "expected hex digit";
template<> const char* Control<StringCh>::errmsg = "invalid char literal";
template<> const char* Control<StringChUni>::errmsg = "invalid char literal";
template<> const char* Control<StringChEscSpec>::errmsg = "invalid escape sequence";
template<> const char* Control<StringChUniEscSpec>::errmsg = "invalid escape sequence";
template<> const char* Control<one<']'>>::errmsg = "expected `]`";
template<> const char* Control<one<')'>>::errmsg = "expected `)`";
template<> const char* Control<one<'>'>>::errmsg = "expected `>`";
template<> const char* Control<one<'{'>>::errmsg = "expected `{`";
template<> const char* Control<one<'}'>>::errmsg = "expected `}`";
template<> const char* Control<one<'='>>::errmsg = "expected `=`";
template<> const char* Control<one<'\''>>::errmsg = "expected `'`";
template<> const char* Control<RS>::errmsg = "expected a whitespace character";
template<> const char* Control<NSC>::errmsg = "expected a whitespace character";
template<> const char* Control<Expression<SC>>::errmsg = "expected expression";
template<> const char* Control<Expression<NSC>>::errmsg = "expected expression";
template<> const char* Control<ExprOperand<SC>>::errmsg = "expected expression";
template<> const char* Control<ExprArgSafe>::errmsg = "expected expression";
template<> const char* Control<ExprInfix<NSC>>::errmsg = "expected expression";
template<> const char* Control<DeclParam>::errmsg = "expected function parameter declaration";
template<> const char* Control<Block>::errmsg = "expected function body";
template<> const char* Control<ExprCallable>::errmsg = "expected callable";
template<> const char* Control<ExprInfixRight<SC>>::errmsg = "expected infix operator";
template<> const char* Control<ExprInfixRight<NSC>>::errmsg = "expected infix operator";
template<> const char* Control<DotCallRightExpr<SC>>::errmsg = "expected expression or typename (dot call)";
template<> const char* Control<DotCallRightExpr<NSC>>::errmsg = "expected expression or typename (dot call)";
template<> const char* Control<TypeDotCallRight<SC>>::errmsg = "expected funcrion call (type dot call)";
template<> const char* Control<TypeDotCallRight<NSC>>::errmsg = "expected funcrion call (type dot call)";
template<> const char* Control<Variable>::errmsg = "expected variable name";
template<> const char* Control<UnsafeType>::errmsg = "expected type";
template<> const char* Control<Type>::errmsg = "expected type";
template<> const char* Control<plus<Type, SC>>::errmsg = "expected type";
template<> const char* Control<TypeName>::errmsg = "expected type name";
template<> const char* Control<plus<TypeName, SC>>::errmsg = "expected type name";
template<> const char* Control<TypeConstraint>::errmsg = "expected type constraint";
template<> const char* Control<TypeContext>::errmsg = "expected type constraints";
template<> const char* Control<StringContent>::errmsg = "unclosed string literal";
template<> const char* Control<BytesContent>::errmsg = "unclosed bytes literal";
template<> const char* Control<RawStringContent>::errmsg = "unclosed raw string literal";
template<> const char* Control<FunctionDecl>::errmsg = "expected function declaration";
template<> const char* Control<SepList<ClassDeclaration>>::errmsg = "expected function declaration";
template<> const char* Control<SepList<Definition>>::errmsg = "expected function definition";
template<> const char* Control<ExprStructItem>::errmsg = "expected struct item";
template<> const char* Control<KeywordThen>::errmsg = "expected `then` expression";
template<> const char* Control<SepList<Statement>>::errmsg = "expected statement";
template<> const char* Control<SepList<TopLevelStatement>>::errmsg = "expected statement";

// default message
template<typename T> const char* Control<T>::errmsg = nullptr;

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
    in(src.data(), src.size(), SourceRef{m_source_manager, src_id});

    try {
        if (!tao::pegtl::parse< Module, Action, Control >( in, mod ))
            assert(!"input not matched");  // cannot happen, must<eof> always throws on unmatched input
        mod.body.finish();
    } catch (tao::pegtl::parse_error& e) {
        SourceLocation loc;
        loc.load(in, e.positions().front());
        throw xci::script::parse_error(e.message(), loc);
    } catch( const std::exception& e ) {
        throw xci::script::parse_error(e.what());
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
