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

#include <string>
#include <sstream>

using namespace std;
using namespace xci::script;


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


void check_interpreter(const string& input, const string& expected_output, uint32_t flags=0)
{
    Interpreter interpreter {flags};

    ostringstream os;
    try {
        auto result = interpreter.eval(input, [&os](const Value& invoked) {
            os << invoked << '\n';
        });
        if (!result->is_void()) {
            os << *result;
        }
    } catch (const Error& e) {
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
    check_parser("$$ raw \n\r\t string $$", "\" raw \n\r\t string \"");
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
}


TEST_CASE( "Disambiguation", "[script][parser]" )
{
    // operator '|' vs. lambda
    check_parser("a |b", "(a | b)");
    check_parser("a |b| c", "((a | b) | c)");
    check_parser("a |b| {}", "a (|b| {void})");  // 'a' is function being called, 'b' is lambda parameter
    check_parser("a |b| ({})", "((a | b) | ({void}))");  // using braces to disambiguate
    check_parser("(a |b) | {}", "((a | b) | ({void}))");
    check_parser("a ( | b | { } )", "a (|b| {void})");
    check_parser("a | {b}", "(a | ({b}))");
}


TEST_CASE( "Stack grow", "[script][machine]" )
{
    Stack stack(4);
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
    Stack stack;

    CHECK(stack.empty());
    stack.push(value::Bool{true});
    CHECK(stack.size() == 1);
    stack.push(value::Int32{73});
    CHECK(stack.size() == 1+4);
    stack.push(value::String{"hello"});
    CHECK(stack.size() == 1+4 + sizeof(size_t) + sizeof(void*));
    CHECK(stack.n_values() == 3);

    CHECK(stack.pull<value::String>().value() == "hello");
    CHECK(stack.pull<value::Int32>().value() == 73);
    CHECK(stack.pull<value::Bool>().value() == true);
}


TEST_CASE( "Expressions", "[script][interpreter]" )
{
    check_interpreter("add 1 2",        "3");
    check_interpreter("1 + 6/5",        "2");
    check_interpreter("1 + 2 / 3 == 1 + (2 / 3)",  "true");
    check_interpreter("-(1 + 2)",       "-3");
    check_interpreter("1+1, {2+2}",       "(2, 4)");
    check_interpreter("f=|a:Int|{a+1}; [1, f 2]",       "[1, 3]");
}


TEST_CASE( "Types", "[script][interpreter]" )
{
    // each definition can have explicit type
    check_interpreter("a:Int = 1 ; a",        "1");

    // function type can be specified in lambda or specified explicitly
    check_interpreter("f = |a:Int b:Int|->Int {a+b}; f 1 2", "3");
    check_interpreter("f:|Int Int|->Int = |a b|{a+b}; f 1 2", "3");

    // narrowing type of polymorphic function (`f 1.0 2.0` would be error, while `add 1.0 2.0` still works)
    // check_interpreter("f:|Int Int|->Int = add ; f 1 2",        "3");
}


TEST_CASE( "Blocks and lambdas", "[script][interpreter]" )
{
    // blocks are evaluated and return a value
    check_interpreter("{}",         "");
    check_interpreter("{1+2}",      "3");
    check_interpreter("{{{1+2}}}",  "3");

    // blocks can be assigned to a name
    check_interpreter("b = {1+2}; b", "3");
    check_interpreter("b:Int = {1+2}; b", "3");

    // argument propagation: `f` returns a function which consumes the second arg
    check_interpreter("f = |a:Int|{ |b:Int|{ a+b } }; f 1 2",     "3");

    // partial call: `(add 1)` returns a lambda which takes single argument
    //check_interpreter("(add 1) 2",     "3");
}


TEST_CASE( "Lexical scope", "[script][interpreter]" )
{
    check_interpreter("a=1; {b=2; {a + b}}",     "3");
    check_interpreter("a=1; f=|b:Int|{a + b}; f 2",  "3");

    // recursion
    check_interpreter("f=|n:Int| -> Int { if n == 1 then 1 else n * f (n-1) }; f 7",  "5040");      // factorial
    check_interpreter("f=|x:Int| -> Int { if x < 2 then x else f (x-1) + f (x-2) }; f 7",  "13");   // Fibonacci number

    // iteration (with tail-recursive functions)
    check_interpreter("fi=|prod:Int cnt:Int max:Int| -> Int { if cnt > max then prod else fi (cnt*prod) (cnt+1) max };\n"
                      "f=|n:Int| -> Int { fi 1 1 n }; f 7",  "5040");  // factorial
    check_interpreter("fi=|a:Int b:Int n:Int| -> Int { if n==0 then b else fi (a+b) a (n-1) };\n"
                      "f=|n:Int| -> Int { fi 1 0 n }; f 7",  "13");    // Fibonacci number
}


TEST_CASE( "Lists", "[script][interpreter]" )
{
    check_interpreter("[1,2,3] ! 2", "3");
    CHECK_THROWS_AS(Interpreter{0}.eval("[1,2,3]!3"), IndexOutOfBounds);
    //check_interpreter("[[1,2],[3,4],[5,6]] @ 1 @ 0", "3");
    check_interpreter("head = |l:[Int]| -> Int { l!0 }; head [1,2,3]", "1");
}


TEST_CASE( "Type classes", "[script][interpreter]" )
{
    check_interpreter("class XEq T { xeq : |T T| -> Bool }; "
                      "instance XEq Int32 { xeq = { __equal_32 } }; "
                      "xeq 1 2", "false");
}


TEST_CASE( "Compiler intrinsics", "[script][interpreter]" )
{
    // function signature must be explicitly declared, it's never inferred from intrinsics
    // parameter names are not needed (and not used), intrinsics work directly with stack
    // e.g. __equal_32 pulls two 32bit values and pushes 8bit Bool value back
    check_interpreter("my_eq = |Int32 Int32| -> Bool { __equal_32 }; my_eq 42 (2*21)", "true");
    // alternative style - essentially the same
    check_interpreter("my_eq : |Int32 Int32| -> Bool = { __equal_32 }; my_eq 42 43", "false");
}
