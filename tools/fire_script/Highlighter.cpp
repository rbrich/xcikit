// Highlighter.cpp.cc created on 2021-03-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Highlighter.h"
#include <xci/core/parser/unescape_rules.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#ifndef NDEBUG
#include <tao/pegtl/contrib/analyze.hpp>
#endif

#include <sstream>

namespace xci::script::tool {

namespace parser {

using namespace tao::pegtl;
using namespace xci::core::parser::unescape;

// ----------------------------------------------------------------------------
// Grammar

// This is a simplified version of the grammar from "xci/script/Parser.cpp".
// The main reason for not sharing the grammar with the actual parser is that
// this one needs to be much less strict - it must allow invalid and incomplete input.
// Some more nondeterminants are added for the purpose of syntax highlithing,
// and some are removed because information about the actual structure (i.e. AST)
// is not needed for the highligher. For example, infix/prefix operators don't need
// to be distinguished and invalid combination of operators is fine here: "1 + / 2".
// Imagine the cursor in the middle of that string...

struct Expression;
struct Block;

// Spaces and comments
struct LineComment: seq< two<'/'>, until<eolf> > {};
struct BlockComment: seq< string<'/', '*'>, until< string<'*', '/'>, any > > {};
struct OpenBlockComment: seq< string<'/', '*'>, until<eof> > {};
struct EscapedNewline: seq< one<'\\'>, eol> {};
struct SpaceOrComment: sor< blank, EscapedNewline, BlockComment> {};
struct NlSpaceOrComment: sor< space, EscapedNewline, LineComment, BlockComment, OpenBlockComment > {};
struct SemicolonOrNewline: sor<one<';'>, eol, LineComment> {};
struct SC: star< SpaceOrComment > {};  // optional space or comments
struct NSC: star< NlSpaceOrComment > {};  // optional newlines, space or comments
struct RS: at<space> {};  // require space

// Aux templates
template<class T> struct SepList: list_tail<T, seq<SC, SemicolonOrNewline, NSC> > {};  // list separated by either semicolon or newline

// Basic tokens
//                 underscore* (lower identifier_other* | digit+)
struct Identifier: seq< star<one<'_'>>, sor<seq<lower, star<identifier_other>>, plus<digit>> > {};
struct TypeName: seq< upper, star< identifier_other > > {};
struct Operator: sor< one<',', ':'>, two<'&'>, two<'|'>, two<'='>, string<'!','='>,
        string<'<','='>, string<'>','='>, one<'='>,
        two<'<'>, two<'>'>, one<'<', '>'>,
        one<'+', '-'>, two<'*'>, one<'*'>,
        seq<one<'/'>, not_at<one<'/', '*'>>>,
        one<'%', '!', '&', '|', '^', '~'> > {};
struct RoundBracketOpen: one<'('> {};
struct RoundBracketClose: one<')'> {};
struct SquareBracketOpen: one<'['> {};
struct SquareBracketClose: one<']'> {};
struct BraceOpen: one<'{'> {};
struct BraceClose: one<'}'> {};

struct SpecialVariable: seq< one<'_'>, star<digit>, not_at<identifier_other> > {};

// Keywords
struct KeywordFun: TAO_PEGTL_KEYWORD("fun") {};
struct KeywordClass: TAO_PEGTL_KEYWORD("class") {};
struct KeywordInstance: TAO_PEGTL_KEYWORD("instance") {};
struct FunnyKeyword: sor<KeywordFun, KeywordClass, KeywordInstance> {};

struct KeywordIf: TAO_PEGTL_KEYWORD("if") {};
struct KeywordThen: TAO_PEGTL_KEYWORD("then") {};
struct KeywordElse: TAO_PEGTL_KEYWORD("else") {};
struct KeywordWith: TAO_PEGTL_KEYWORD("with") {};
struct KeywordMatch: TAO_PEGTL_KEYWORD("match") {};
struct ControlKeyword: sor<KeywordIf, KeywordThen, KeywordElse, KeywordWith, KeywordMatch> {};

struct KeywordVoidT: TAO_PEGTL_KEYWORD("Void") {};
struct KeywordBool: TAO_PEGTL_KEYWORD("Bool") {};
struct KeywordByte: TAO_PEGTL_KEYWORD("Byte") {};
struct KeywordChar: TAO_PEGTL_KEYWORD("Char") {};
struct KeywordInt: TAO_PEGTL_KEYWORD("Int") {};
struct KeywordInt32: TAO_PEGTL_KEYWORD("Int32") {};
struct KeywordInt64: TAO_PEGTL_KEYWORD("Int64") {};
struct KeywordFloat: TAO_PEGTL_KEYWORD("Float") {};
struct KeywordFloat32: TAO_PEGTL_KEYWORD("Float32") {};
struct KeywordFloat64: TAO_PEGTL_KEYWORD("Float64") {};
struct KeywordString: TAO_PEGTL_KEYWORD("String") {};
struct TypeKeyword: sor<KeywordVoidT, KeywordBool, KeywordByte, KeywordChar, KeywordInt,
        KeywordInt32, KeywordInt64, KeywordFloat, KeywordFloat32, KeywordFloat64, KeywordString> {};

struct KeywordVoid: TAO_PEGTL_KEYWORD("void") {};
struct KeywordFalse: TAO_PEGTL_KEYWORD("false") {};
struct KeywordTrue: TAO_PEGTL_KEYWORD("true") {};
struct ValueKeyword: sor<KeywordVoid, KeywordFalse, KeywordTrue> {};

struct Keyword: sor<FunnyKeyword, ControlKeyword, TypeKeyword, ValueKeyword> {};

// Literals
struct Integer: seq< opt<one<'-','+'>>, plus<digit> > {};
struct Float: seq< opt<one<'-','+'>>, plus<digit>, one<'.'>, star<digit> > {};
struct Char: if_must< one<'\''>, StringCh, one<'\''> > {};
struct String: if_must< one<'"'>, until<one<'"'>, StringCh > > {};
struct RawString : raw_string< '$', '-', '$' > {};  // raw_string = $$ raw text! $$
struct Literal: sor< Float, Integer, Char, String, RawString > {};

// REPL commands
struct ShortCommand: seq< sor<one<'h', 'q'>, seq<one<'d'>, one<'m', 'f', 'i'>>>, not_at<identifier_other>> {};
struct LongCommand: sor<TAO_PEGTL_KEYWORD("help"), TAO_PEGTL_KEYWORD("quit"),
        TAO_PEGTL_KEYWORD("dump_module"), TAO_PEGTL_KEYWORD("dump_function"), TAO_PEGTL_KEYWORD("dump_info")> {};
struct ValidCommand: sor<ShortCommand, LongCommand> {};
struct InvalidCommand: star< not_at< blank >, any > {};
struct ReplCommand: if_must<one<'.'>, sor<ValidCommand, InvalidCommand>, SC> {};

// Expressions
struct BracketedExpr: seq< RoundBracketOpen, NSC, Expression, NSC, RoundBracketClose > {};
struct List: seq< SquareBracketOpen, NSC, opt<Expression, NSC>, SquareBracketClose > {};
struct FullyBracketed: sor< BracketedExpr, Block, List > {};
struct PrimaryExpr: sor< Operator, Literal, Keyword, SpecialVariable, Identifier, TypeName > {};

// Incomplete expressions
struct OpenBracket: one< '(', '[' > {};
struct OpenBrace: one< '{' > {};

// Invalid expressions
struct InvalidCloseBracket: one< ')', ']' > {};
struct InvalidCloseBrace: one< '}' > {};

// Statements
struct PartialExpr: sor< FullyBracketed, OpenBracket, OpenBrace, PrimaryExpr > {};
struct Expression: plus< PartialExpr, NSC > {};
struct InvalidExpr: plus< sor< InvalidCloseBracket, InvalidCloseBrace, PartialExpr > > {};
struct Statement: sor< seq<Expression, star<SC, InvalidExpr>>, InvalidExpr > {};

// Statements in a Block (in braces)
struct PartialExprB: sor< FullyBracketed, OpenBracket, PrimaryExpr > {};
struct ExpressionB: plus< PartialExprB, SC > {};
struct InvalidExprB: plus< sor< InvalidCloseBracket, PartialExprB > > {};
struct StatementB: sor< seq<ExpressionB, star<SC, InvalidExprB>>, InvalidExprB > {};
struct Block: seq< BraceOpen, NSC, opt<SepList<StatementB>, NSC>, BraceClose > {};

// The main rule, grammar entry point
struct Main: must< NSC, sor<ReplCommand, SepList<Statement>, success>, NSC, eof > {};

// ----------------------------------------------------------------------------

template< typename Rule >
using HighlightSelector = tao::pegtl::parse_tree::selector< Rule,
        tao::pegtl::parse_tree::store_content::on<
                Main,
                ValidCommand,
                InvalidCommand,
                FunnyKeyword,
                ControlKeyword,
                TypeKeyword,
                ValueKeyword,
                SpecialVariable,
                TypeName,
                Integer,
                Float,
                Char,
                String,
                RawString,
                FullyBracketed,
                RoundBracketOpen,
                RoundBracketClose,
                SquareBracketOpen,
                SquareBracketClose,
                BraceOpen,
                BraceClose,
                OpenBracket,
                OpenBrace,
                InvalidCloseBracket,
                InvalidCloseBrace,
                LineComment,
                BlockComment,
                OpenBlockComment > >;

} // namespace parser

using HighlightColor = Highlighter::HighlightColor;
using Color = Highlighter::Color;
using Mode = Highlighter::Mode;

// NOTE: Don't forget to add each type to HighlightSelector above
static std::pair<const char*, HighlightColor> highlight_color[] {
        // brackets - bg color applied only for brackets under cursor
        {":RoundBracketOpen", {Color::BrightWhite, Mode::Normal, Color::BrightBlack}},
        {":RoundBracketClose", {Color::BrightWhite, Mode::Normal, Color::BrightBlack}},
        {":SquareBracketOpen", {Color::BrightWhite, Mode::Normal, Color::BrightBlack}},
        {":SquareBracketClose", {Color::BrightWhite, Mode::Normal, Color::BrightBlack}},
        {":BraceOpen", {Color::BrightWhite, Mode::Normal, Color::BrightBlack}},
        {":BraceClose", {Color::BrightWhite, Mode::Normal, Color::BrightBlack}},
        {":InvalidCloseBracket", {Color::BrightRed}},
        {":InvalidCloseBrace", {Color::BrightRed, Mode::Bold}},

        // types and variables
        {":SpecialVariable", {Color::Magenta, Mode::Bold}},
        {":TypeName", {Color::Yellow}},

        // keywords, well-known types and names
        {":ControlKeyword", {Color::Magenta}},
        {":FunnyKeyword", {Color::BrightMagenta}},
        {":TypeKeyword", {Color::BrightYellow}},
        {":ValueKeyword", {Color::BrightBlue}},

        // commands
        {":ValidCommand", {Color::BrightYellow, Mode::Bold}},
        {":InvalidCommand", {Color::BrightRed}},

        // numbers
        {":Integer", {Color::BrightCyan}},
        {":Float", {Color::Cyan}},

        // strings
        {":Char", {Color::Green}},
        {":String", {Color::BrightGreen}},
        {":RawString", {Color::BrightGreen}},

        // comments
        {":LineComment", {Color::BrightBlack}},
        {":BlockComment", {Color::BrightBlack}},
        {":OpenBlockComment", {Color::BrightBlack}},
};


static HighlightColor select_highlight_color(std::string_view node_type, bool hl_bracket)
{
    for (auto const& [type, c] : highlight_color) {
        if (node_type.ends_with(type)) {
            // highlight brackets
            if (hl_bracket)
                return HighlightColor{
                    .fg = Color::White,
                    .mode = Mode::Normal,
                    .bg = c.bg
                };
            // normal
            return HighlightColor{
                .fg = c.fg,
                .mode = c.mode,
                .bg = Color::Default
            };
        }
    }
    return {};
}


void Highlighter::switch_color(const HighlightColor& from, const HighlightColor& to)
{
    if (from.mode != to.mode) {
        // reset all attributes, fg/bg needs to be set again (below)
        m_output += m_term.normal().seq();
        if (to.mode != Mode::Normal)
            m_output += m_term.mode(to.mode).seq();
    }
    if (from.fg != to.fg || from.mode != to.mode)
        m_output += m_term.fg(to.fg).seq();
    if (from.bg != to.bg || from.mode != to.mode)
        m_output += m_term.bg(to.bg).seq();
}


HighlightColor Highlighter::highlight_node(
        const tao::pegtl::parse_tree::node& node,
        const HighlightColor& prev_color,
        unsigned cursor, bool hl_bracket)
{
    // Highlight only open/close brackets, not the content
    hl_bracket = hl_bracket && (node.type.ends_with("Open") || node.type.ends_with("Close"));

    // Workaround: parse_tree node doesn't expose its begin/end char* directly.
    // I've tried to use `node.string_view()`, but it makes the iterating more complicated
    // and less readable, so let's do this instead, for now.
    const auto *pos = node.m_begin.data;
    auto color = select_highlight_color(node.type, hl_bracket);
    switch_color(prev_color, color);

    const bool fully_bracketed = node.type.ends_with(":FullyBracketed");
    // When this node is FullyBracketed, then allow highlighting brackets
    // in direct child nodes, if the cursor is positioned on them.
    bool child_hl_bracket = (fully_bracketed
            && (cursor == node.begin().byte || cursor == node.end().byte - 1));

    // Set open bracket flag on any unpaired { ( [
    if (node.type.ends_with(":OpenBracket") || node.type.ends_with(":OpenBrace"))
        m_open_bracket = true;

    // Reset open bracket flag on any unpaired } ) ]
    if (node.type.ends_with(":InvalidCloseBracket") || node.type.ends_with(":InvalidCloseBrace"))
        m_open_bracket = false;

    for (const auto& child : node.children) {
        m_output.append(pos, child->m_begin.data - pos);
        auto child_color = highlight_node(*child, color, cursor, child_hl_bracket);
        switch_color(child_color, color);
        pos = child->m_end.data;
    }

    // Reset open bracket flag if we just closed an expression or a block
    // (Unpaired open brackets inside don't matter.)
    if (fully_bracketed)
        m_open_bracket = false;

    m_output.append(pos, node.m_end.data - pos);
    return color;
}


auto Highlighter::highlight(std::string_view input, unsigned cursor) -> HlResult
{
    using namespace parser;

    tao::pegtl::memory_input<
            tao::pegtl::tracking_mode::eager,
            tao::pegtl::eol::lf_crlf,
            const char*>  // pass source filename as non-owning char*
    in(input.data(), input.size(), "<input>");

    m_output.clear();
    try {
        auto root = tao::pegtl::parse_tree::parse< Main, HighlightSelector >( in );
        if (root->children.size() != 1)
            return {std::string{input} + m_term.format("\n{fg:*red}{t:bold}highlighter parse error:{t:normal} {fg:*red}no match{t:normal}"), false};
        auto last_color = highlight_node(*root->children[0], HighlightColor{}, cursor);
        switch_color(last_color, HighlightColor{});
        return {m_output, m_open_bracket};
    } catch (tao::pegtl::parse_error& e) {
        // The grammar is build in a way that parse error should never happen
        return {std::string{input} + m_term.format("\n{fg:*red}{t:bold}highlighter parse error:{t:normal} {fg:*red}{}{t:normal}", e.what()), false};
    }
}


} // namespace xci::script::tool
