// test_script.cpp created on 2019-05-27, part of XCI toolkit
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

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <xci/script/Parser.h>
#include <xci/script/Interpreter.h>
#include <xci/script/Error.h>
#include <xci/script/Stack.h>
#include <xci/script/SymbolTable.h>
#include <xci/script/NativeDelegate.h>
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
    ostringstream os;
    os << ast;
    CHECK( os.str() == expected_output );
}


void check_interpreter(const string& input, const string& expected_output)
{
    static std::unique_ptr<Module> sys_module;
    Interpreter interpreter;

    if (!sys_module) {
        Logger::init(Logger::Level::Warning);
        Vfs vfs;
        vfs.mount(XCI_SHARE_DIR);

        auto f = vfs.read_file("script/sys.ys");
        auto content = f.content();
        sys_module = interpreter.build_module("sys", content->string_view());
    }
    interpreter.add_imported_module(*sys_module);

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


TEST_CASE( "Comments", "[script][parser]" )
{
    check_parser("a  // C-style comment", "a");
    check_parser("/**/a/*C++-style\n \ncomment*/+b/*\n*/", "(a + b)");
}


TEST_CASE( "Values", "[script][parser]" )
{
    check_parser("identifier", "identifier");
    check_parser("123", "123");
    check_parser("1.", "1.0");
    check_parser("1.23", "1.23");
    check_parser("\"string literal\"", "\"string literal\"");
    check_parser("\"escape sequences: \\\"\\n\\0\\x12 \"", "\"escape sequences: \\\"\\n\\0\\x12 \"");
    check_parser("$$ raw \n\r\t\" string $$", "\" raw \\n\\r\\t\\\" string \"");
    check_parser("1,2,3", "(1, 2, 3)");  // comma operator makes tuple, braces are optional
    check_parser("(1,2,\"str\")", "(1, 2, \"str\")");
    check_parser("[1,2,3]", "[1, 2, 3]");
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
    check_parser("a fun b {} c", "a fun b {void} c");
    check_parser("a (fun b {}) c", "a fun b {void} c");
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

    // narrowing type of polymorphic function (`f 1.0 2.0` would be error, while `add 1.0 2.0` still works)
    // check_interpreter("f : Int Int -> Int = add ; f 1 2",        "3");
}


TEST_CASE( "Blocks", "[script][interpreter]" )
{
    // blocks are evaluated and return a value
    check_interpreter("{}",         "void");
    check_interpreter("{1+2}",      "3");
    check_interpreter("{{{1+2}}}",  "3");
    check_interpreter("x=4; b = 3 + {x+1}; b", "8");

    // blocks can be assigned to a name
    check_interpreter("b = {1+2}; b", "3");
    check_interpreter("b = { a = 1; a }; b", "1");
    check_interpreter("b:Int = {1+2}; b", "3");
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
    // [closure.ys] return closure with captured closures, propagate arguments into the closure
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
    check_interpreter("{a=1; b=2}",     "void");
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
    CHECK(native::ValueType<std::string>("test"s).value() == "test"s);
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
        (test_fun1a 10 4 2) +  //  3
        (test_fun1b 0 6 3)     // -2
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
        (add1 1 6) +           //  7
        (add2 3 4)             //  8  (+10 from state)
    )");
    CHECK(result->type() == Type::Int32);
    CHECK(result->as<value::Int32>().value() == 24);
}
