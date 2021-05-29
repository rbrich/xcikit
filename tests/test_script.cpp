// test_script.cpp created on 2019-05-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/script/Parser.h>
#include <xci/script/Interpreter.h>
#include <xci/script/Error.h>
#include <xci/script/Stack.h>
#include <xci/script/SymbolTable.h>
#include <xci/script/NativeDelegate.h>
#include <xci/script/ast/fold_tuple.h>
#include <xci/script/dump.h>
#include <xci/core/Vfs.h>
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/config.h>

#include <string>
#include <sstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;
using namespace xci::script;
using namespace xci::core;


// Check parsing into AST and dumping back to code
std::string parse(const string& input)
{
    SourceManager src_man;
    auto src_id = src_man.add_source("<input>", input);
    Parser parser {src_man};
    ast::Module ast;
    parser.parse(src_id, ast);
    fold_tuple(ast.body);
    ostringstream os;
    os << ast;
    return os.str();
}


// Format parser input/output by hand, don't use Catch2's automatic expansion.
// This is easier to read in the log and it also workarounds a bug in MSVC:
//    CHECK(parse(R"("escape sequences: \"\n\0\x12 ")", R"("escape sequences: \"\n\0\x12 ")"));
//    > error C2017: illegal escape sequence
//    > error C2146: syntax error: missing ')' before identifier 'n'
#define PARSE(input, expected)                          \
    do {                                                \
        auto output = parse(input);                     \
        INFO("Input:    " << indent((input), 12));      \
        INFO("Output:   " << indent((output), 12));     \
        INFO("Expected: " << indent((expected), 12));   \
        if ((output) == (expected))                     \
            SUCCEED();                                  \
        else                                            \
            FAIL();                                     \
    } while(false)


void import_std_module(Interpreter& interpreter)
{
    static std::unique_ptr<Module> module;
    static BufferPtr content;
    const char* std_path = "script/std.fire";
    if (!module) {
        Logger::init(Logger::Level::Warning);
        Vfs vfs;
        vfs.mount(XCI_SHARE);

        auto f = vfs.read_file(std_path);
        REQUIRE(f.is_open());
        content = f.content();
        auto src_id = interpreter.source_manager().add_source(std_path, content->string());
        module = interpreter.build_module("std", src_id);
        assert(src_id == 1);
    } else {
        auto src_id = interpreter.source_manager().add_source(std_path, content->string());
        assert(src_id == 1);
        (void) src_id;
    }
    interpreter.add_imported_module(*module);
}


std::string interpret(const string& input, bool import_std=false)
{
    Interpreter interpreter;

    if (import_std) {
        import_std_module(interpreter);
    }

    UNSCOPED_INFO(input);
    ostringstream os;
    try {
        auto result = interpreter.eval(input, [&os](TypedValue&& invoked) {
            os << invoked << ';';
            invoked.decref();
        });
        os << result;
        result.decref();
    } catch (const ScriptError& e) {
        UNSCOPED_INFO("Exception: " << e.what() << "\n" << e.detail());
        throw;
    }
    return os.str();
}


std::string interpret_std(const string& input)
{
    return interpret(input, true);
}


#ifndef NDEBUG
TEST_CASE( "Analyze grammar", "[script][parser]" )
{
   REQUIRE(Parser::analyze_grammar() == 0);
}
#endif


TEST_CASE( "Comments", "[script][parser]" )
{
    CHECK(parse("a  // C-style comment") == "a");
    CHECK(parse("/**/a/*C++-style\n \ncomment*/+b/*\n*/") == "(a + b)");
}


TEST_CASE( "Optional semicolon", "[script][parser]" )
{
    CHECK(parse("a = 1") == parse("a = 1;"));
    CHECK(parse("a = 1\nb = 2\n") == parse("a = 1; b = 2;"));
    CHECK(parse("(\n 1\n  + \n2\n)\n\na = 1  // nl still counted\nb=2\nc=3") ==
          parse("(1+2); a=1; b=2; c=3"));  // newlines are allowed inside brackets
    CHECK(parse("40\n.add 2\n50\n.sub 8") == parse("40 .add 2; 50 .sub 8;"));  // dotcall can continue after linebreak
    CHECK(parse("a =\n1") == parse("a=1"));  // linebreak is allowed after '=' in definition
    CHECK(parse("1 + \n 2") == parse("1+2"));  // linebreak is allowed after infix operator
    CHECK(parse("add 1 \\\n 2") == parse("add 1 2"));  // newline can be escaped
    CHECK(parse("(add 1 \\\n 2)") == parse("(add 1 2)"));
    REQUIRE_THROWS_WITH(parse("a=1;;"),  // empty statement is not allowed, semicolon is only used as a separator
            Catch::Matchers::StartsWith("parse error: invalid syntax"));
}


TEST_CASE( "Values", "[script][parser]" )
{
    CHECK(parse("identifier") == "identifier");
    CHECK(parse("123") == "123");
    CHECK(parse("1.") == "1.0");
    CHECK(parse("1.23") == "1.23");
    CHECK(parse("42b") == "b'*'");  // byte (8-bit integer)
    CHECK(parse("b'B'") == "b'B'");
    PARSE(R"(b"bytes literal")", R"(b"bytes literal")");
    CHECK(parse("'c'") == "'c'");
    PARSE(R"("string literal")", R"("string literal")");
    PARSE(R"("escape sequences: \"\n\0\x12 ")", R"("escape sequences: \"\n\0\x12 ")");
    CHECK(parse("1,2,3") == "1, 2, 3");  // naked tuple
    PARSE(R"((1,2,"str"))", R"((1, 2, "str"))");  // bracketed tuple
    CHECK(parse("[1,2,3]") == "[1, 2, 3]");  // list
    CHECK(parse("[(1,2,3,4)]") == "[(1, 2, 3, 4)]");  // list with a tuple item
    CHECK(parse("[(1,2,3,4), 5]") == "[(1, 2, 3, 4), 5]");
}


TEST_CASE( "Raw strings", "[script][parser]" )
{
    PARSE(R"("""Hello""")", R"("Hello")");
    PARSE("\"\"\"\nHello\n\"\"\"", R"("Hello")");
    PARSE("\"\"\"\n  Hello\n  \"\"\"", R"("Hello")");
    PARSE("\"\"\"\n  Hello\n\"\"\"", R"("  Hello")");
    PARSE("\"\"\"\n    Hello\n  \"\"\"", R"("  Hello")");
    PARSE("\"\"\"\n  \\nHello\\0\n  \"\"\"", R"("\\nHello\\0")");
    PARSE("\"\"\"Hello\n\"\"\"", R"("Hello\n")");
    PARSE("\"\"\"\n  Hello\"\"\"", R"("\n  Hello")");
    PARSE("\"\"\"\n\nHello\n\n\"\"\"", R"("\nHello\n")");
    PARSE("\"\"\"\n  \\\"\"\"Hello\\\"\"\"\n  \"\"\"", R"("\"\"\"Hello\"\"\"")");
    PARSE("\"\"\"\n  \\\"\"\"\"Hello\\\"\"\"\"\n  \"\"\"", R"("\"\"\"\"Hello\"\"\"\"")");
    PARSE("\"\"\"\n  \\\"\"\"\n  \\\\\"\"\"\n  \\\\\\\"\"\"\n  \"\"\"",
                R"("\"\"\"\n\\\"\"\"\n\\\\\"\"\"")");
    CHECK_THROWS_AS(parse("\"\"\"\\\"\"\""), ParseError);
    PARSE("\"\"\"\n\\\n\"\"\"", R"("\\")");
}


TEST_CASE( "Parsing types", "[script][parser]")
{
    CHECK(parse("type MyInt = Int") == "type MyInt = Int");
    CHECK(parse("type MyList = [Int]") == "type MyList = [Int]");
    CHECK(parse("type MyTuple = (String, Int)") == "type MyTuple = (String, Int)");
    CHECK(parse("type MyTuple = String, Int") == "type MyTuple = (String, Int)");  // The round brackets are optional, added in AST dump for clarity
    CHECK(parse("type MyListOfTuples = [String, Int]") == "type MyListOfTuples = [(String, Int)]");
    CHECK(parse("type MyListOfTuples2 = [(String, Int), Int]") == "type MyListOfTuples2 = [((String, Int), Int)]");
    CHECK(parse("MyAlias = Int") == "MyAlias = Int");
    CHECK(parse("MyAlias2 = [Int]") == "MyAlias2 = [Int]");
}


TEST_CASE( "Trailing comma", "[script][parser]" )
{
    CHECK(parse("1,2,3,") == "1, 2, 3");
    CHECK(parse("[1,2,3,]") == "[1, 2, 3]");
    CHECK(parse("(1,2,3,)") == "(1, 2, 3)");
    CHECK_THROWS_AS(parse("1,2,3,,"), ParseError);  // two commas not allowed
    CHECK_THROWS_AS(parse("1,2,,3"), ParseError);
    CHECK_THROWS_AS(parse("(1,2,3,,)"), ParseError);
    CHECK_THROWS_AS(parse("[1,2,3,,]"), ParseError);
    CHECK_THROWS_AS(parse("[,]"), ParseError);
    CHECK(parse("([1,],[2,],[1,2,],)") == "([1], [2], [1, 2])");
    // multiline
    CHECK(parse("1,\n2,\n3,\n") == "1, 2, 3");  // expression continues on next line after operator
    CHECK(parse("1,;\n2,\n3,\n") == "1\n2, 3");  // semicolon splits the multiline expression
    CHECK(parse("(\n1,\n2,\n3,\n)") == "(1, 2, 3)");
    CHECK(parse("[\n1,\n2,\n3,\n]") == "[1, 2, 3]");
    CHECK(parse("[\n1\n,\n2\n,\n3\n,\n]") == "[1, 2, 3]");
}


TEST_CASE( "Operator precedence", "[script][parser]" )
{
    CHECK(parse("a+b") == "(a + b)");
    CHECK(parse("a + b*c") == "(a + (b * c))");
    CHECK(parse("a*b + c") == "((a * b) + c)");
    CHECK(parse("a + b*c + d") == "((a + (b * c)) + d)");
    CHECK(parse("a || b && c | d << e + f * g ** h") == "(a || (b && (c | (d << (e + (f * (g ** h)))))))");
    CHECK(parse("a ** b * c + d << e | f && g || h") == "(((((((a ** b) * c) + d) << e) | f) && g) || h)");
    CHECK(parse("a * b + c | d || e & f - g * h / i") == "((((a * b) + c) | d) || (e & (f - ((g * h) / i))))");
    CHECK(parse("a || b & c + d * e + f & g && h / i") == "(a || (((b & ((c + (d * e)) + f)) & g) && (h / i)))");
    // left associative:
    CHECK(parse("a + b + c + d") == "(((a + b) + c) + d)");
    // right associative:
    CHECK(parse("a ** b ** c ** d") == "(a ** (b ** (c ** d)))");
    // functions
    CHECK(parse("a fun b {} c") == "a fun b {} c");
    CHECK(parse("a (fun b {}) c") == "a (fun b {}) c");
    // function calls
    CHECK(interpret_std("succ 9 + max 5 4 + 1") == "16");
    CHECK(interpret_std("(succ 9) + (max 5 4) + 1") == "16");
    CHECK(interpret_std("succ 9 + 5 .max 4 + 1") == "16");
    CHECK(interpret_std("1 .add 2 .mul 3") == "9");
    CHECK(interpret_std("(1 .add 2).mul 3") == "9");
    CHECK(interpret_std("1 .add (2 .mul 3)") == "7");
    CHECK(interpret_std("pred (neg (succ (14)))") == "-16");
    CHECK(interpret_std("14 .succ .neg .pred") == "-16");
    CHECK(interpret_std("(((14) .succ) .neg) .pred") == "-16");
}


TEST_CASE( "Type argument or comparison?", "[script][parser]" )
{
    PARSE("1 > 2", "(1 > 2)");
    PARSE("1 < 2", "(1 < 2)");
    PARSE("a<b> 3", "((a < b) > 3)");
    PARSE("a<T> 3", "a<T> 3");
}


TEST_CASE( "Value size on stack", "[script][machine]" )
{
    CHECK(Value().size_on_stack() == type_size_on_stack(Type::Void));
    CHECK(Value(false).size_on_stack() == type_size_on_stack(Type::Bool));
    CHECK(Value(0).size_on_stack() == type_size_on_stack(Type::Int32));
    CHECK(Value(int64_t{0}).size_on_stack() == type_size_on_stack(Type::Int64));
    CHECK(Value(0.0f).size_on_stack() == type_size_on_stack(Type::Float32));
    CHECK(Value(0.0).size_on_stack() == type_size_on_stack(Type::Float64));
    CHECK(Value(Value::StringTag{}).size_on_stack() == type_size_on_stack(Type::String));
    CHECK(Value(Value::ListTag{}).size_on_stack() == type_size_on_stack(Type::List));
    CHECK(Value(TypeInfo::Subtypes{}).size_on_stack() == type_size_on_stack(Type::Tuple));
    CHECK(Value(Value::ClosureTag{}).size_on_stack() == type_size_on_stack(Type::Function));
    CHECK(Value(Value::ModuleTag{}).size_on_stack() == type_size_on_stack(Type::Module));
}


TEST_CASE( "Stack grow", "[script][machine]" )
{
    xci::script::Stack stack(4);
    CHECK(stack.capacity() == 4);
    CHECK(stack.size() == 0);

    stack.push(value::Int32{73});
    CHECK(stack.size() == 4);
    CHECK(stack.capacity() == 4);

    stack.push(value::Int32{42});
    CHECK(stack.size() == 8);
    CHECK(stack.capacity() == 8);

    stack.push(value::Int32{333});
    CHECK(stack.size() == 12);
    CHECK(stack.capacity() == 16);

    CHECK(stack.pull<value::Int32>().value() == 333);
    CHECK(stack.pull<value::Int32>().value() == 42);
    CHECK(stack.pull<value::Int32>().value() == 73);
    CHECK(stack.empty());
    CHECK(stack.capacity() == 16);
}


TEST_CASE( "Stack push/pull", "[script][machine]" )
{
    xci::script::Stack stack;

    CHECK(stack.empty());
    stack.push(value::Bool{true});
    CHECK(stack.size() == 1);
    stack.push(value::Int32{73});
    CHECK(stack.size() == 1+4);
    value::String str{"hello"};
    stack.push(str);
    CHECK(stack.size() == 1+4 + sizeof(void*));
    CHECK(stack.n_values() == 3);

    CHECK(stack.pull<value::String>().value() == "hello");
    CHECK(stack.pull<value::Int32>().value() == 73);
    CHECK(stack.pull<value::Bool>().value() == true);  // NOLINT

    CHECK(str.heapslot()->refcount() == 1);
    str.decref();
}


TEST_CASE( "SymbolTable", "[script][compiler]" )
{
    SymbolTable symtab;

    auto alpha = symtab.add({"alpha", Symbol::Value});
    auto beta = symtab.add({"beta", Symbol::Value});
    auto gamma = symtab.add({"Gamma", Symbol::Instance});
    auto delta = symtab.add({"delta", Symbol::Value});

    CHECK(alpha == symtab.find_last_of("alpha", Symbol::Value));
    CHECK(beta == symtab.find_last_of("beta", Symbol::Value));
    CHECK(gamma == symtab.find_last_of("Gamma", Symbol::Instance));
    CHECK(delta == symtab.find_last_of("delta", Symbol::Value));
    CHECK(! symtab.find_last_of("zeta", Symbol::Value));
}


TEST_CASE( "Literals", "[script][interpreter]" )
{
    // Integer literal out of 32bit range is promoted to Int64
    CHECK(interpret("2147483647") == "2147483647");
    CHECK(interpret("2147483648") == "2147483648L");
    CHECK(interpret("-2147483648") == "-2147483648");
    CHECK(interpret("-2147483649") == "-2147483649L");
    // Integer literal out of 64bit range doesn't compile
    CHECK(interpret("9223372036854775807L") == "9223372036854775807L");
    CHECK_THROWS_AS(interpret("9223372036854775808L"), ParseError);
    CHECK(interpret("-9223372036854775808L") == "-9223372036854775808L");
    CHECK_THROWS_AS(interpret("-9223372036854775809L"), ParseError);
}


TEST_CASE( "Expressions", "[script][interpreter]" )
{
    CHECK(interpret_std("add 1 2") == "3");
    CHECK(interpret_std("sub (add 1 2) 3") == "0");
    CHECK(interpret_std("sub (1 + 2) 3") == "0");
    CHECK(interpret_std("(1 + 2) - 3") == "0");
    CHECK(interpret_std("1 + 2 - 3") == "0");

    CHECK(interpret_std("1 + 6/5") == "2");
    CHECK(interpret_std("1 + 2 / 3 == 1 + (2 / 3)") == "true");
    CHECK(interpret_std("-(1 + 2)") == "-3");
    CHECK(interpret_std("1+1, {2+2}") == "(2, 4)");
    CHECK(interpret_std("f=fun a:Int {a+1}; [1, f 2]") == "[1, 3]");
}


TEST_CASE( "Types", "[script][interpreter]" )
{
    // each definition can have explicit type
    CHECK(interpret("a:Int = 1 ; a") == "1");
    CHECK_THROWS_AS(interpret("a:Int = 1.0 ; a"), DefinitionTypeMismatch);

    // function type can be specified in lambda or specified explicitly
    CHECK(interpret_std("f = fun a:Int b:Int -> Int {a+b}; f 1 2") == "3");
    CHECK(interpret_std("f : Int Int -> Int = fun a b {a+b}; f 1 2") == "3");

    // TODO: narrowing type of polymorphic function (`f 1.0 2.0` would be error, while `add 1.0 2.0` still works)
    // CHECK(interpret("f : Int Int -> Int = add ; f 1 2") == "3");
}


TEST_CASE( "User-defined types", "[script][interpreter]" )
{
    CHECK(interpret("TupleAlias = (String, Int); a:TupleAlias = \"hello\", 42; a") == "(\"hello\", 42)");
    CHECK_THROWS_AS(interpret("type MyTuple = (String, Int); a:MyTuple = \"hello\", 42; a"), DefinitionTypeMismatch);
    // TODO: cast from underlying type
    //CHECK(interpret_std("type MyTuple = (String, Int); a = (\"hello\", 42):MyTuple; a") == "(\"hello\", 42)");
    CHECK_THROWS_AS(interpret_std("type MyTuple = (String, Int); (1, 2):MyTuple"), FunctionNotFound);  // bad cast
}


TEST_CASE( "Blocks", "[script][interpreter]" )
{
    CHECK(parse("{}") == "{}");
    CHECK(parse("{{}}") == "{{}}");

    // blocks are evaluated and return a value
    CHECK(interpret("{}") == "");  // empty function (has Void type)
    CHECK(interpret("{{}}") == "");  // empty function in empty function
    CHECK(interpret("{};{};{}") == "");  // three empty functions
    CHECK(interpret_std("{1+2}") == "3"); // non-empty
    CHECK(interpret_std("{{{1+2}}}") == "3"); // three wrapped functions, each returns the result of inner one
    CHECK(interpret_std("{1+2;4;{}}") == "3;4;");  // {} as the last statement changes function result to Void, intermediate results are "invoked"
    CHECK(interpret_std("x=4; b = 3 + {x+1}; b") == "8");

    // blocks can be assigned to a name
    CHECK(interpret("a = {}; a") == "");  // empty block can be named too, `a` is a named function of Void type
    CHECK(interpret_std("b = {1+2}; b") == "3");
    CHECK(interpret("b = { a = 1; a }; b") == "1");
    CHECK(interpret_std("b:Int = {1+2}; b") == "3");

    // blocks are evaluated after all definitions in the scope,
    // which means they can use names from parent scope that are defined later
    CHECK(interpret("y={x}; x=7; y") == "7");
}


TEST_CASE( "Functions and lambdas", "[script][interpreter]" )
{
    CHECK(parse("fun Int -> Int {}") == "fun Int -> Int {}");

    // returned lambda
    CHECK(interpret_std("fun x:Int->Int { x + 1 }") == "<lambda> Int32 -> Int32");
    CHECK_THROWS_AS(interpret_std("fun x { x + 1 }"), UnexpectedGenericFunction);  // generic lambda must be either assigned or resolved by calling

    // immediately called lambda
    CHECK(interpret_std("fun x:Int {x+1} 2") == "3");
    CHECK(interpret_std("fun x {x+1} 2") == "3");  // generic lambda
    CHECK(interpret_std("b = 3 + fun x {2*x} 2; b") == "7");

    // generic function in local scope
    CHECK(interpret_std("outer = fun y { inner = fun x { x+1 }; inner y }; outer 2") == "3");

    // argument propagation:
    CHECK(interpret_std("f = fun a:Int { fun b:Int { a+b } }; f 1 2") == "3");  //  `f` returns a function which consumes the second arg
    CHECK(interpret_std("f = fun a:Int { fun b:Int { fun c:Int { a+b+c } } }; f 1 2 3") == "6");
    CHECK(interpret_std("{ fun x:Int {x*2} } 3") == "6");  // lambda propagates through wrapped blocks and is then called
    CHECK(interpret_std("{{{ fun x:Int {x*2} }}} 3") == "6");  // lambda propagates through wrapped blocks and is then called

    // closure: inner function uses outer function's parameter
    CHECK(interpret_std("f = fun a:Int b:Int c:Int { "
                        "w=fun c1:Int {a / b - c1}; w c }; f 10 2 3") == "2");
    // closure: outer closure used by inner function
    CHECK(interpret_std("f = fun a:Int b:Int c:Int { "
                        "g=fun c1:Int {a * b - c1}; "
                        "h=fun c1:Int {g c1}; "
                        "h c }; f 4 2 3") == "5");
    CHECK(interpret_std("f = fun a:Int b:Int c:Int { "
                        "u=fun b2:Int {a + b2}; v=fun c2:Int {c2 + b}; "
                        "w=fun b1:Int c1:Int {a + u b1 + v c1}; "
                        "w b c }; f 1 2 3") == "9");

    CHECK(interpret_std("outer = fun y:Int {"
                        "inner = fun x:Int { x + y }; inner y "
                        "}; outer 2") == "4");
    CHECK(interpret_std("outer = fun y:Int {"
                        "  inner = fun x:Int { x + y };"
                        "  alias = inner; alias y "
                        "}; outer 2") == "4");
    CHECK(interpret_std("outer = fun y {"
                        "  inner = fun x:Int { x + y };"
                        "  wrapped = fun x:Int { inner x };"
                        "  wrapped y "
                        "}; outer 2") == "4");
    CHECK(interpret_std("outer = fun y {"
                        "  inner = fun x:Int { in2 = fun z:Int{ x + z }; in2 y };"
                        "  wrapped = fun x:Int { inner x }; wrapped y"
                        "}; outer 2") == "4");

    // closure: fully generic
    CHECK(interpret_std("outer = fun y { inner = fun x { x + y }; inner 3 * inner y }; outer 2") == "20");
    CHECK(interpret_std("outer = fun y {"
                        "  inner = fun x { x + y };"
                        "  wrapped = fun x { inner x };"
                        "  wrapped y "
                        "}; outer 2") == "4");

    // multiple specializations
    // * each specialization is generated only once
    CHECK(interpret_std("outer = fun<T> y:T { inner = fun<U> x:U { x + y:U }; inner 3 + inner 4 }; "
                        "outer 1; outer 2; __module.__n_fn") == "9;11;5");
    // * specializations with different types from the same template
    CHECK(interpret_std("outer = fun<T> y:T { inner = fun<U> x:U { x + y:U }; inner 3 + (inner 4l):T }; outer 2") == "11");

    // "Funarg problem" (upwards)
    //CHECK(interpret_std("compose = fun f g { fun x { f (g x) } }; same = compose pred succ; same 42") == "42");
}


TEST_CASE( "Partial function call", "[script][interpreter]" )
{
    // partial call: `add 1` returns a lambda which takes single argument
    CHECK(interpret_std("(add 1) 2") == "3");
    CHECK(interpret_std("{add 1} 2") == "3");
    CHECK(interpret_std("f=add 1; f 2") == "3");
    CHECK(interpret_std("f={add 1}; f 2") == "3");
    CHECK(interpret_std("f=fun x:Int {add x}; f 2 1") == "3");
    CHECK(interpret_std("f=fun x:Int {add 3}; f 2 1") == "4");
    CHECK(interpret_std("f=fun x:Int y:Int z:Int { (x - y) * z}; g=fun x1:Int { f 3 x1 }; g 4 5") == "-5");
    CHECK(interpret_std("f=fun x:Int y:Int { g=fun x1:Int z1:Int { (y - x1) / z1 }; g x }; f 1 10 3") == "3");
    CHECK(interpret_std("f = fun a:Int b:Int { "
                        "u=fun b2:Int {a + b2}; v=fun c2:Int {c2 - b}; "
                        "w=fun b1:Int c1:Int {a * u b1 / v c1}; "
                        "w b }; f 1 2 3") == "3");
    // [closure.fire] return closure with captured closures, propagate arguments into the closure
    CHECK(interpret_std("f = fun a:Int { "
                        "u=fun b2:Int {a / b2}; v=fun c2:Int {c2 - a}; "
                        "fun b1:Int c1:Int {a + u b1 + v c1} }; f 4 2 3") == "5");
}


TEST_CASE( "Generic functions", "[script][interpreter]" )
{
    // `f` is a generic function, instantiated to Int->Int by the call
    CHECK(interpret_std("f=fun x {x + 1}; f (f (f 2))") == "5");
    // generic functions can capture from outer scope
    CHECK(interpret_std("a=3; f=fun x {a + x}; f 4") == "7");
    // generic type declaration, type constraint
    CHECK(interpret_std("f = fun<T> x:T y:T -> Bool with (Eq T) { x == y }; f 1 2") == "false");
}


TEST_CASE( "Lexical scope", "[script][interpreter]" )
{
    CHECK(interpret("{a=1; b=2}") == "");
    CHECK_THROWS_AS(interpret("{a=1; b=2} a"), UndefinedName);

    CHECK(interpret_std("x=1; y = { x + 2 }; y") == "3");
    CHECK(interpret_std("a=1; {b=2; {a + b}}") == "3");
    CHECK(interpret_std("a=1; f=fun b:Int {a + b}; f 2") == "3");

    // recursion
    CHECK(interpret_std("f=fun x:Int->Int { x; if x <= 1 then 0 else f (x-1) }; f 5") == "5;4;3;2;1;0");      // yield intermediate steps
    CHECK(interpret_std("f=fun n:Int->Int { if n == 1 then 1 else n * f (n-1) }; f 7") == "5040");      // factorial
    CHECK(interpret_std("f=fun x:Int->Int { if x < 2 then x else f (x-1) + f (x-2) }; f 7") == "13");   // Fibonacci number

    // iteration (with tail-recursive functions)
    CHECK(interpret_std("fi=fun prod:Int cnt:Int max:Int -> Int { if cnt > max then prod else fi (cnt*prod) (cnt+1) max };\n"
                        "f=fun n:Int->Int { fi 1 1 n }; f 7") == "5040");  // factorial
    CHECK(interpret_std("fi=fun a:Int b:Int n:Int -> Int { if n==0 then b else fi (a+b) a (n-1) };\n"
                        "f=fun n:Int->Int { fi 1 0 n }; f 7") == "13");    // Fibonacci number
}


TEST_CASE( "Casting", "[script][interpreter]" )
{
    CHECK(interpret_std("\"drop this\":Void") == "");
    CHECK(interpret_std("\"noop\":String") == "\"noop\"");
    CHECK(interpret_std("42:Int64") == "42L");
    CHECK(interpret_std("42L:Int32") == "42");
    CHECK(interpret_std("42:Float32") == "42.0f");
    CHECK(interpret_std("42:Float64") == "42.0");
    CHECK(interpret_std("12.9:Int") == "12");
    CHECK(interpret_std("-12.9:Int") == "-12");
    CHECK(interpret_std("a = 42; a:Byte") == "b'*'");
    CHECK(interpret_std("(1 + 2):Int64") == "3L");
    CHECK(interpret_std("0:Bool") == "false");
    CHECK(interpret_std("42:Bool") == "true");
    CHECK(interpret_std("(42:Bool):Int") == "1");
    CHECK(interpret_std("!42:Bool") == "false");  // '!' is prefix operator, cast has higher precedence
    CHECK(interpret_std("-42:Bool") == "true");  // '-' is part of Int literal, not an operator
    CHECK_THROWS_AS(interpret_std("- 42:Bool"), FunctionNotFound);  // now it's operator and that's an error: "neg Bool" not defined
    CHECK(interpret_std("(- 42):Bool") == "true");
    //CHECK(interpret_std("['a', 'b', 'c']:String") == "abc");
    CHECK(interpret_std("(cast 42):Int64") == "42L");
    CHECK(interpret_std("a:Int64 = cast 42; a") == "42L");
    CHECK_THROWS_AS(interpret_std("cast 42"), FunctionNotFound);  // must specify the result type
}


TEST_CASE( "Subscript", "[script][interpreter]" )
{
    // custom implementation (same as in std.fire)
    CHECK(interpret("__type_id<Void>") == "0");
    CHECK_THROWS_AS(interpret("__type_id<X>"), UndefinedTypeName);
    CHECK(interpret("subscript = fun<T> [T] Int -> T { __subscript __type_id<T> }; subscript [1,2,3] 1") == "2");
    // std implementation
    CHECK(interpret_std("subscript [1,2,3] 1") == "2");
    CHECK(interpret_std("[1,2,3] ! 2") == "3");
    CHECK_THROWS_AS(interpret_std("[1,2,3]!3"), IndexOutOfBounds);
    CHECK(interpret_std("['a','b','c'] ! 1") == "'b'");
    CHECK(interpret_std("l=[1,2,3,4,5]; l!0 + l!1 + l!2 + l!3 + l!4") == "15");  // inefficient sum
    CHECK(interpret_std("[[1,2],[3,4],[5,6]] ! 1 ! 0") == "3");
    CHECK(interpret_std("head = fun l:[Int] -> Int { l!0 }; head [1,2,3]") == "1");
    CHECK(interpret_std("head = fun<T> l:[T] -> T { l!0 }; head ['a','b','c']") == "'a'");
}


TEST_CASE( "Type classes", "[script][interpreter]" )
{
    CHECK(interpret("class XEq T { xeq : T T -> Bool }; "
                    "instance XEq Int32 { xeq = { __equal_32 } }; "
                    "xeq 1 2") == "false");
}


TEST_CASE( "With expression, I/O streams", "[script][interpreter]" )
{
    CHECK(parse("with stdout { 42 }") == "with stdout {42};");
    // __streams is a builtin function that returns a tuple of current streams: (in, out, err)
    CHECK(interpret("__streams") == "(in=<stream:stdin>, out=<stream:stdout>, err=<stream:stderr>)");
    CHECK(interpret("with null { __streams }") == "(in=<stream:stdin>, out=<stream:null>, err=<stream:stderr>)");
    CHECK_THROWS_AS(interpret("with 42 {}"), FunctionNotFound);  // function not found: enter Int32
    CHECK(interpret("with (null, null) { __streams }") == "(in=<stream:null>, out=<stream:null>, err=<stream:stderr>)");
    CHECK(interpret("with (null, null, null) { __streams }; __streams")  // `with` scope
          == "(in=<stream:null>, out=<stream:null>, err=<stream:null>);"
             "(in=<stream:stdin>, out=<stream:stdout>, err=<stream:stderr>)");
    CHECK(interpret("with (in=null) { __streams }") == "(in=<stream:null>, out=<stream:stdout>, err=<stream:stderr>)");
    CHECK(interpret("with (out=null) { __streams }") == "(in=<stream:stdin>, out=<stream:null>, err=<stream:stderr>)");
    CHECK(interpret("with (in=null, out=null) { __streams }") == "(in=<stream:null>, out=<stream:null>, err=<stream:stderr>)");
    CHECK(interpret("with (in=stdin, out=stdout, err=stderr) { __streams }") == "(in=<stream:stdin>, out=<stream:stdout>, err=<stream:stderr>)");
    CHECK(interpret("with\n(in=stdin, out=stdout, err=stderr)\n{ 42 }") == "42");
    CHECK(interpret_std("with (in=null):Streams { __streams }") == "(in=<stream:null>, out=<stream:stdout>, err=<stream:stderr>)");  // the missing fields are inherited in `enter` function
    CHECK(interpret_std("(in=null):Streams") == "(in=<stream:null>, out=<stream:undef>, err=<stream:undef>)");  // the missing fields are default initialized => undef stream
    CHECK(interpret_std("(in=null)") == "(in=<stream:null>)");  // anonymous struct

    // write an actual file
    auto filename = fs::temp_directory_path() / "xci_test_script.txt";
    CHECK(interpret("with (open \""s + escape(filename.string()) + "\" \"w\")\n"
                    "    write \"this goes to the file\"") == "");
    CHECK(interpret("with (in=(open \""s + escape(filename.string()) + "\" \"r\"))\n"
                    "    read 9") == "\"this goes\"");
    CHECK(fs::remove(filename));
}


TEST_CASE( "Compiler intrinsics", "[script][interpreter]" )
{
    // function signature must be explicitly declared, it's never inferred from intrinsics
    // parameter names are not needed (and not used), intrinsics work directly with stack
    // e.g. __equal_32 pulls two 32bit values and pushes 8bit Bool value back
    CHECK(interpret_std("my_eq = fun Int32 Int32 -> Bool { __equal_32 }; my_eq 42 (2*21)") == "true");
    // alternative style - essentially the same
    CHECK(interpret("my_eq : Int32 Int32 -> Bool = { __equal_32 }; my_eq 42 43") == "false");
    // intrinsic with arguments
    CHECK(interpret("my_cast : Int32 -> Int64 = { __cast 0x89 }; my_cast 42") == "42L");
}


TEST_CASE( "Native to TypeInfo mapping", "[script][native]" )
{
    CHECK(native::make_type_info<void>().type() == Type::Void);
    CHECK(native::make_type_info<bool>().type() == Type::Bool);
    CHECK(native::make_type_info<uint8_t>().type() == Type::Byte);
    CHECK(native::make_type_info<char>().type() == Type::Char);
    CHECK(native::make_type_info<char16_t>().type() == Type::Char);
    CHECK(native::make_type_info<char32_t>().type() == Type::Char);
    CHECK(native::make_type_info<int>().type() == Type::Int32);  // depends on sizeof(int)
    CHECK(native::make_type_info<long long>().type() == Type::Int64);  // depends on sizeof(long long)
    CHECK(native::make_type_info<int32_t>().type() == Type::Int32);
    CHECK(native::make_type_info<int64_t>().type() == Type::Int64);
    CHECK(native::make_type_info<float>().type() == Type::Float32);
    CHECK(native::make_type_info<double>().type() == Type::Float64);
    CHECK(native::make_type_info<std::string>().type() == Type::String);
    CHECK(native::make_type_info<char*>().type() == Type::String);
    CHECK(native::make_type_info<const char*>().type() == Type::String);
}


TEST_CASE( "Native to Value mapping", "[script][native]" )
{
    CHECK(native::ValueType<void>().type() == Type::Void);
    CHECK(native::ValueType<bool>{true}.value() == true);
    CHECK(native::ValueType<byte>(255).value() == 255);
    CHECK(native::ValueType<char>('y').value() == 'y');
    CHECK(native::ValueType<int32_t>(-1).value() == -1);
    CHECK(native::ValueType<int64_t>(1ll << 60).value() == 1ll << 60);
    CHECK(native::ValueType<float>(3.14f).value() == 3.14f);
    CHECK(native::ValueType<double>(2./3).value() == 2./3);
    native::ValueType<std::string>str ("test"s);
    CHECK(str.value() == "test"s);
    str.decref();
}


int test_fun1(int a, int b, int c) { return (a - b) / c; }

TEST_CASE( "Native functions: free function", "[script][native]" )
{
    Interpreter interpreter;
    Module module;
    import_std_module(interpreter);
    interpreter.add_imported_module(module);

    // free function
    module.add_native_function("test_fun1a", &test_fun1);
    module.add_native_function("test_fun1b", test_fun1);  // function pointer is deduced

    auto result = interpreter.eval(R"(
        ((test_fun1a 10 4 2)     //  3
        + (test_fun1b 0 6 3))    // -2
    )");
    CHECK(result.type() == Type::Int32);
    CHECK(result.get<int32_t>() == 1);
}


TEST_CASE( "Native functions: lambda", "[script][native]" )
{
    Interpreter interpreter;
    Module module;
    interpreter.add_imported_module(module);

    // lambdas
    module.add_native_function("add1", [](int a, int b) { return a + b; });

    // lambda with state (can't use capture)
    int state = 10;
    module.add_native_function("add2",
            [](void* s, int a, int b) { return a + b + *(int*)(s); },
            &state);

    auto result = interpreter.eval(R"(
        (add1 (add1 1 6)       //  7
              (add2 3 4))      //  8  (+10 from state)
    )");
    CHECK(result.type() == Type::Int32);
    CHECK(result.get<int32_t>() == 24);
}
