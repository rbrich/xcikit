// test_script.cpp created on 2019-05-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2021 Radek Brich
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
#include <xci/config.h>

#include <string>
#include <sstream>

using namespace std;
using namespace xci::script;
using namespace xci::core;


// Check parsing into AST and dumping back to code
void check_parser(const string& input, const string& expected_output)
{
    Parser parser;
    ast::Module ast;
    parser.parse(input, ast);
    fold_tuple(ast.body);
    ostringstream os;
    os << ast;
    CHECK( os.str() == expected_output );
}

// Check that parsing the two inputs gives the same result
void check_parser_eq(const string& input1, const string& input2)
{
    Parser parser;
    ast::Module ast1;
    ast::Module ast2;
    parser.parse(input1, ast1);
    parser.parse(input1, ast2);
    ostringstream os1;
    ostringstream os2;
    os1 << ast1;
    os2 << ast2;
    CHECK( os1.str() == os2.str() );
}

void check_parser_error(const string& input, const char* starts_with)
{
    Parser parser;
    ast::Module ast;
    REQUIRE_THROWS_WITH(parser.parse(input, ast), Catch::Matchers::StartsWith(starts_with));
}

void check_interpreter(const string& input, const string& expected_output="true")
{
    static std::unique_ptr<Module> std_module;
    Interpreter interpreter;

    if (!std_module) {
        Logger::init(Logger::Level::Warning);
        Vfs vfs;
        vfs.mount(XCI_SHARE);

        auto f = vfs.read_file("script/std.fire");
        REQUIRE(f.is_open());
        auto content = f.content();
        std_module = interpreter.build_module("std", content->string_view());
    }
    interpreter.add_imported_module(*std_module);

    ostringstream os;
    try {
        auto result = interpreter.eval(input, [&os](const Value& invoked) {
            os << invoked << '\n';
        });
        os << *result;
        result->decref();
    } catch (const ScriptError& e) {
        INFO(e.what());
        INFO(e.detail());
        FAIL("Exception thrown.");
    }

    INFO(input);
    CHECK( os.str() == expected_output );
}


#ifndef NDEBUG
TEST_CASE( "Analyze grammar", "[script][parser]" )
{
   REQUIRE(Parser::analyze_grammar() == 0);
}
#endif


TEST_CASE( "Comments", "[script][parser]" )
{
    check_parser("a  // C-style comment", "a");
    check_parser("/**/a/*C++-style\n \ncomment*/+b/*\n*/", "(a + b)");
}


TEST_CASE( "Optional semicolon", "[script][parser]" )
{
    check_parser_eq("a = 1", "a = 1;");
    check_parser_eq("a = 1\nb = 2\n", "a = 1; b = 2;");
    check_parser_eq("(\n 1\n  + \n2\n)\n\na = 1  // nl still counted\nb=2\nc=3",
            "(1+2); a=1; b=2; c=3");  // newlines are allowed inside brackets
    check_parser_eq("40\n.add 2\n50\n.sub 8", "40 .add 2; 50 .sub 8;");  // dotcall can continue after linebreak
    check_parser_eq("a =\n1", "a=1");  // linebreak is allowed after '=' in definition
    check_parser_eq("1 + \\\n 2", "1+2");  // newline can be escaped
    check_parser_eq("(1 + \\\n 2)", "(1+2)");
    check_parser_error("a=1;;", "parse error: <input>:1:5: invalid syntax");  // empty statement is not allowed, semicolon is only used as a separator
}


TEST_CASE( "Values", "[script][parser]" )
{
    check_parser("identifier", "identifier");
    check_parser("123", "123");
    check_parser("1.", "1.0");
    check_parser("1.23", "1.23");
    check_parser("b'B'", "b'B'");
    check_parser("b\"bytes literal\"", "b\"bytes literal\"");
    check_parser("'c'", "'c'");
    check_parser("\"string literal\"", "\"string literal\"");
    check_parser("\"escape sequences: \\\"\\n\\0\\x12 \"", "\"escape sequences: \\\"\\n\\0\\x12 \"");
    check_parser("$$ raw \n\r\t\" string $$", "\" raw \\n\\r\\t\\\" string \"");
    check_parser("1,2,3", "1, 2, 3");  // naked tuple
    check_parser("(1,2,\"str\")", "(1, 2, \"str\")");  // bracketed tuple
    check_parser("[1,2,3]", "[1, 2, 3]");  // list
    check_parser("[(1,2,3,4)]", "[(1, 2, 3, 4)]");  // list with a tuple item
    check_parser("[(1,2,3,4), 5]", "[(1, 2, 3, 4), 5]");
}


TEST_CASE( "Operator precedence", "[script][parser]" )
{
    check_parser("a+b", "(a + b)");
    check_parser("a + b*c", "(a + (b * c))");
    check_parser("a*b + c", "((a * b) + c)");
    check_parser("a + b*c + d", "((a + (b * c)) + d)");
    check_parser("a || b && c | d << e + f * g ** h", "(a || (b && (c | (d << (e + (f * (g ** h)))))))");
    check_parser("a ** b * c + d << e | f && g || h", "(((((((a ** b) * c) + d) << e) | f) && g) || h)");
    check_parser("a * b + c | d || e & f - g * h / i", "((((a * b) + c) | d) || (e & (f - ((g * h) / i))))");
    check_parser("a || b & c + d * e + f & g && h / i", "(a || (((b & ((c + (d * e)) + f)) & g) && (h / i)))");
    // left associative:
    check_parser("a + b + c + d", "(((a + b) + c) + d)");
    // right associative:
    check_parser("a ** b ** c ** d", "(a ** (b ** (c ** d)))");
    // functions
    check_parser("a fun b {} c", "a fun b {} c");
    check_parser("a (fun b {}) c", "a (fun b {}) c");
    // function calls
    check_interpreter("succ 9 + max 5 4 + 1", "16");
    check_interpreter("(succ 9) + (max 5 4) + 1", "16");
    check_interpreter("succ 9 + 5 .max 4 + 1", "16");
    check_interpreter("1 .add 2 .mul 3", "9");
    check_interpreter("(1 .add 2).mul 3", "9");
    check_interpreter("1 .add (2 .mul 3)", "7");
    check_interpreter("pred (neg (succ (14)))", "-16");
    check_interpreter("14 .succ .neg .pred", "-16");
    check_interpreter("(((14) .succ) .neg) .pred", "-16");
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
    CHECK(stack.size() == 1+4 + sizeof(size_t) + sizeof(void*));
    CHECK(stack.n_values() == 3);

    CHECK(stack.pull<value::String>().value() == "hello");
    CHECK(stack.pull<value::Int32>().value() == 73);
    CHECK(stack.pull<value::Bool>().value() == true);  // NOLINT

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


TEST_CASE( "Expressions", "[script][interpreter]" )
{
    check_interpreter("add 1 2",        "3");
    check_interpreter("sub (add 1 2) 3",        "0");
    check_interpreter("sub (1 + 2) 3",        "0");
    check_interpreter("(1 + 2) - 3",        "0");
    check_interpreter("1 + 2 - 3",        "0");

    check_interpreter("1 + 6/5",        "2");
    check_interpreter("1 + 2 / 3 == 1 + (2 / 3)",  "true");
    check_interpreter("-(1 + 2)",       "-3");
    check_interpreter("1+1, {2+2}",       "(2, 4)");
    check_interpreter("f=fun a:Int {a+1}; [1, f 2]", "[1, 3]");
}


TEST_CASE( "Types", "[script][interpreter]" )
{
    // each definition can have explicit type
    check_interpreter("a:Int = 1 ; a",        "1");

    // function type can be specified in lambda or specified explicitly
    check_interpreter("f = fun a:Int b:Int -> Int {a+b}; f 1 2", "3");
    check_interpreter("f : Int Int -> Int = fun a b {a+b}; f 1 2", "3");

    // TODO: narrowing type of polymorphic function (`f 1.0 2.0` would be error, while `add 1.0 2.0` still works)
    // check_interpreter("f : Int Int -> Int = add ; f 1 2",        "3");
}


TEST_CASE( "Blocks", "[script][interpreter]" )
{
    // blocks are evaluated and return a value
    check_interpreter("{}",         "");  // empty function (has Void type)
    check_interpreter("{{}}",       "");  // empty function in empty function
    check_interpreter("{};{};{}",   "");  // three empty functions
    check_interpreter("{1+2}",      "3"); // non-empty
    check_interpreter("{{{1+2}}}",  "3"); // three wrapped functions, each returns the result of inner one
    check_interpreter("{1+2;4;{}}", "3\n4\n");  // {} as the last statement changes function result to Void, intermediate results are "invoked"
    check_interpreter("x=4; b = 3 + {x+1}; b", "8");

    // blocks can be assigned to a name
    check_interpreter("a = {}; a", "");  // empty block can be named too, `a` is a named function of Void type
    check_interpreter("b = {1+2}; b", "3");
    check_interpreter("b = { a = 1; a }; b", "1");
    check_interpreter("b:Int = {1+2}; b", "3");

    // blocks are evaluated after all definitions in the scope,
    // which means they can use names from parent scope that are defined later
    check_interpreter("y={x}; x=7; y", "7");
}


TEST_CASE( "Functions and lambdas", "[script][interpreter]" )
{
    // immediately called lambda
    check_interpreter("fun x:Int {x+1} 2", "3");
    check_interpreter("fun x {x+1} 2", "3");  // generic lambda
    check_interpreter("b = 3 + fun x {2*x} 2; b", "7");

    // argument propagation: `f` returns a function which consumes the second arg
    check_interpreter("f = fun a:Int { fun b:Int { a+b } }; f 1 2",     "3");

    // closure: inner function uses outer function's parameter
    check_interpreter("f = fun a:Int b:Int c:Int { "
                      "w=fun c1:Int {a / b - c1}; w c }; f 10 2 3", "2");
    // closure: outer closure used by inner function
    check_interpreter("f = fun a:Int b:Int c:Int { "
                      "g=fun c1:Int {a * b - c1}; "
                      "h=fun c1:Int {g c1}; "
                      "h c }; f 1 2 3", "-1");
    check_interpreter("f = fun a:Int b:Int c:Int { "
                      "u=fun b2:Int {a + b2}; v=fun c2:Int {c2 + b}; "
                      "w=fun b1:Int c1:Int {a + u b1 + v c1}; "
                      "w b c }; f 1 2 3", "9");

    check_interpreter("outer = fun y:Int {"
                      "inner = fun x:Int { x + y }; inner y "
                      "}; outer 2", "4");
    check_interpreter("outer = fun y:Int {"
                      "inner = fun x:Int { x + y }; alias = inner; alias y "
                      "}; outer 2", "4");
    check_interpreter("outer = fun y {"
                      "inner = fun x:Int { x + y }; alias = fun x:Int { inner x }; alias y "
                      "}; outer 2", "4");
}


TEST_CASE( "Partial function call", "[script][interpreter]" )
{
    // partial call: `add 1` returns a lambda which takes single argument
    check_interpreter("(add 1) 2",     "3");
    check_interpreter("{add 1} 2",     "3");
    check_interpreter("f={add 1}; f 2",     "3");
    check_interpreter("f=fun x:Int {add x}; f 2 1", "3");
    check_interpreter("f=fun x:Int {add 3}; f 2 1", "4");
    check_interpreter("f=fun x:Int y:Int z:Int { (x - y) * z}; g=fun x1:Int { f 3 x1 }; g 4 5", "-5");
    check_interpreter("f=fun x:Int y:Int { g=fun x1:Int z1:Int { (y - x1) / z1 }; g x }; f 1 10 3", "3");
    check_interpreter("f = fun a:Int b:Int { "
                      "u=fun b2:Int {a + b2}; v=fun c2:Int {c2 - b}; "
                      "w=fun b1:Int c1:Int {a * u b1 / v c1}; "
                      "w b }; f 1 2 3", "3");
    // [closure.fire] return closure with captured closures, propagate arguments into the closure
    check_interpreter("f = fun a:Int { "
                      "u=fun b2:Int {a / b2}; v=fun c2:Int {c2 - a}; "
                      "fun b1:Int c1:Int {a + u b1 + v c1} }; f 4 2 3", "5");
}


TEST_CASE( "Generic functions", "[script][interpreter]" )
{
    // `f` is a generic function, instantiated to Int->Int by the call
    check_interpreter("f=fun x {x + 1}; f (f (f 2))", "5");
    // generic functions can capture from outer scope
    check_interpreter("a=3; f=fun x {a + x}; f 4", "7");
    // generic type declaration
    check_interpreter("f = fun x:T y:T -> Bool with (Eq T) { x == y }; f 1 2", "false");
}


TEST_CASE( "Lexical scope", "[script][interpreter]" )
{
    check_interpreter("{a=1; b=2}",     "");
    CHECK_THROWS_AS(Interpreter{0}.eval("{a=1; b=2} a"), UndefinedName);

    check_interpreter("x=1; y = { x + 2 }; y",     "3");
    check_interpreter("a=1; {b=2; {a + b}}",     "3");
    check_interpreter("a=1; f=fun b:Int {a + b}; f 2",  "3");

    // recursion
    check_interpreter("f=fun n:Int->Int { if n == 1 then 1 else n * f (n-1) }; f 7",  "5040");      // factorial
    check_interpreter("f=fun x:Int->Int { if x < 2 then x else f (x-1) + f (x-2) }; f 7",  "13");   // Fibonacci number

    // iteration (with tail-recursive functions)
    check_interpreter("fi=fun prod:Int cnt:Int max:Int -> Int { if cnt > max then prod else fi (cnt*prod) (cnt+1) max };\n"
                      "f=fun n:Int->Int { fi 1 1 n }; f 7",  "5040");  // factorial
    check_interpreter("fi=fun a:Int b:Int n:Int -> Int { if n==0 then b else fi (a+b) a (n-1) };\n"
                      "f=fun n:Int->Int { fi 1 0 n }; f 7",  "13");    // Fibonacci number
}


TEST_CASE( "Lists", "[script][interpreter]" )
{
    check_interpreter("[1,2,3] ! 2", "3");
    CHECK_THROWS_AS(Interpreter{0}.eval("[1,2,3]!3"), IndexOutOfBounds);
    //check_interpreter("[[1,2],[3,4],[5,6]] @ 1 @ 0", "3");
    check_interpreter("head = fun l:[Int] -> Int { l!0 }; head [1,2,3]", "1");
}


TEST_CASE( "Type classes", "[script][interpreter]" )
{
    check_interpreter("class XEq T { xeq : T T -> Bool }; "
                      "instance XEq Int32 { xeq = { __equal_32 } }; "
                      "xeq 1 2", "false");
}


TEST_CASE( "Compiler intrinsics", "[script][interpreter]" )
{
    // function signature must be explicitly declared, it's never inferred from intrinsics
    // parameter names are not needed (and not used), intrinsics work directly with stack
    // e.g. __equal_32 pulls two 32bit values and pushes 8bit Bool value back
    check_interpreter("my_eq = fun Int32 Int32 -> Bool { __equal_32 }; my_eq 42 (2*21)", "true");
    // alternative style - essentially the same
    check_interpreter("my_eq : Int32 Int32 -> Bool = { __equal_32 }; my_eq 42 43", "false");
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
    CHECK(native::ValueType<uint8_t>(255).value() == 255);
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
    interpreter.add_imported_module(module);

    // free function
    module.add_native_function("test_fun1a", &test_fun1);
    module.add_native_function("test_fun1b", test_fun1);  // function pointer is deduced

    auto result = interpreter.eval(R"(
        ((test_fun1a 10 4 2)     //  3
        + (test_fun1b 0 6 3))    // -2
    )");
    CHECK(result->type() == Type::Int32);
    CHECK(result->as<value::Int32>().value() == 1);
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
        ((add1 1 6) +          //  7
        (add2 3 4))            //  8  (+10 from state)
    )");
    CHECK(result->type() == Type::Int32);
    CHECK(result->as<value::Int32>().value() == 24);
}
