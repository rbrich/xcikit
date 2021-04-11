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
#include <xci/config.h>

#include <string>
#include <sstream>

using namespace std;
using namespace xci::script;
using namespace xci::core;


// Check parsing into AST and dumping back to code
std::string parse(const string& input)
{
    Parser parser;
    ast::Module ast;
    parser.parse(input, ast);
    fold_tuple(ast.body);
    ostringstream os;
    os << ast;
    return os.str();
}


std::string interpret(const string& input, bool import_std=false)
{
    Interpreter interpreter;

    if (import_std) {
        static std::unique_ptr<Module> std_module;
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
    }

    UNSCOPED_INFO(input);
    ostringstream os;
    try {
        auto result = interpreter.eval(input, [&os](const TypedValue& invoked) {
            os << invoked << ';';
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
            Catch::Matchers::StartsWith("parse error: <input>:1:5: invalid syntax"));
}


TEST_CASE( "Values", "[script][parser]" )
{
    CHECK(parse("identifier") == "identifier");
    CHECK(parse("123") == "123");
    CHECK(parse("1.") == "1.0");
    CHECK(parse("1.23") == "1.23");
    CHECK(parse("42b") == "b'*'");  // byte (8-bit integer)
    CHECK(parse("b'B'") == "b'B'");
    CHECK(parse("b\"bytes literal\"") == "b\"bytes literal\"");
    CHECK(parse("'c'") == "'c'");
    CHECK(parse("\"string literal\"") == "\"string literal\"");
    // Note: don't use raw string literal, it causes MSVC Error C2017 due to stringize operation in Catch2
    CHECK(parse("\"escape sequences: \\\"\\n\\0\\x12 \"") == "\"escape sequences: \\\"\\n\\0\\x12 \"");
    CHECK(parse("$$ raw \n\r\t\" string $$") == "\" raw \\n\\r\\t\\\" string \"");
    CHECK(parse("1,2,3") == "1, 2, 3");  // naked tuple
    CHECK(parse("(1,2,\"str\")") == "(1, 2, \"str\")");  // bracketed tuple
    CHECK(parse("[1,2,3]") == "[1, 2, 3]");  // list
    CHECK(parse("[(1,2,3,4)]") == "[(1, 2, 3, 4)]");  // list with a tuple item
    CHECK(parse("[(1,2,3,4), 5]") == "[(1, 2, 3, 4), 5]");
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
    CHECK(interpret("1 .add 2 .mul 3") == "9");
    CHECK(interpret("(1 .add 2).mul 3") == "9");
    CHECK(interpret("1 .add (2 .mul 3)") == "7");
    CHECK(interpret_std("pred (neg (succ (14)))") == "-16");
    CHECK(interpret_std("14 .succ .neg .pred") == "-16");
    CHECK(interpret_std("(((14) .succ) .neg) .pred") == "-16");
}


TEST_CASE( "Value size on stack", "[script][machine]" )
{
    CHECK(Value().size_on_stack() == type_size_on_stack(Type::Void));
    CHECK(Value(false).size_on_stack() == type_size_on_stack(Type::Bool));
    CHECK(Value(0).size_on_stack() == type_size_on_stack(Type::Int32));
    CHECK(Value(int64_t{0}).size_on_stack() == type_size_on_stack(Type::Int64));
    CHECK(Value(0.0f).size_on_stack() == type_size_on_stack(Type::Float32));
    CHECK(Value(0.0).size_on_stack() == type_size_on_stack(Type::Float64));
    CHECK(Value("aaa"sv).size_on_stack() == type_size_on_stack(Type::String));
    CHECK(Value(10, TypeInfo{Type::Int32}).size_on_stack() == type_size_on_stack(Type::List));
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
    CHECK(interpret("add 1 2") == "3");
    CHECK(interpret("sub (add 1 2) 3") == "0");
    CHECK(interpret("sub (1 + 2) 3") == "0");
    CHECK(interpret("(1 + 2) - 3") == "0");
    CHECK(interpret("1 + 2 - 3") == "0");

    CHECK(interpret("1 + 6/5") == "2");
    CHECK(interpret_std("1 + 2 / 3 == 1 + (2 / 3)") == "true");
    CHECK(interpret("-(1 + 2)") == "-3");
    CHECK(interpret("1+1, {2+2}") == "(2, 4)");
    CHECK(interpret("f=fun a:Int {a+1}; [1, f 2]") == "[1, 3]");
}


TEST_CASE( "Types", "[script][interpreter]" )
{
    // each definition can have explicit type
    CHECK(interpret("a:Int = 1 ; a") == "1");

    // function type can be specified in lambda or specified explicitly
    CHECK(interpret("f = fun a:Int b:Int -> Int {a+b}; f 1 2") == "3");
    CHECK(interpret("f : Int Int -> Int = fun a b {a+b}; f 1 2") == "3");

    // TODO: narrowing type of polymorphic function (`f 1.0 2.0` would be error, while `add 1.0 2.0` still works)
    // CHECK(interpret("f : Int Int -> Int = add ; f 1 2") == "3");
}


TEST_CASE( "Blocks", "[script][interpreter]" )
{
    // blocks are evaluated and return a value
    CHECK(interpret("{}") == "");  // empty function (has Void type)
    CHECK(interpret("{{}}") == "");  // empty function in empty function
    CHECK(interpret("{};{};{}") == "");  // three empty functions
    CHECK(interpret("{1+2}") == "3"); // non-empty
    CHECK(interpret("{{{1+2}}}") == "3"); // three wrapped functions, each returns the result of inner one
    CHECK(interpret("{1+2;4;{}}") == "3;4;");  // {} as the last statement changes function result to Void, intermediate results are "invoked"
    CHECK(interpret("x=4; b = 3 + {x+1}; b") == "8");

    // blocks can be assigned to a name
    CHECK(interpret("a = {}; a") == "");  // empty block can be named too, `a` is a named function of Void type
    CHECK(interpret("b = {1+2}; b") == "3");
    CHECK(interpret("b = { a = 1; a }; b") == "1");
    CHECK(interpret("b:Int = {1+2}; b") == "3");

    // blocks are evaluated after all definitions in the scope,
    // which means they can use names from parent scope that are defined later
    CHECK(interpret("y={x}; x=7; y") == "7");
}


TEST_CASE( "Functions and lambdas", "[script][interpreter]" )
{
    // returned lambda
    CHECK(interpret("fun x:Int->Int { x + 1 }") == "<lambda> Int32 -> Int32");
    CHECK_THROWS_AS(interpret("fun x { x + 1 }"), UnexpectedGenericFunction);  // generic lambda must be either assigned or resolved by calling

    // immediately called lambda
    CHECK(interpret("fun x:Int {x+1} 2") == "3");
    CHECK(interpret("fun x {x+1} 2") == "3");  // generic lambda
    CHECK(interpret("b = 3 + fun x {2*x} 2; b") == "7");

    // argument propagation:
    CHECK(interpret("f = fun a:Int { fun b:Int { a+b } }; f 1 2") == "3");  //  `f` returns a function which consumes the second arg
    CHECK(interpret("f = fun a:Int { fun b:Int { fun c:Int { a+b+c } } }; f 1 2 3") == "6");
    CHECK(interpret("{ fun x:Int {x*2} } 3") == "6");  // lambda propagates through wrapped blocks and is then called
    CHECK(interpret("{{{ fun x:Int {x*2} }}} 3") == "6");  // lambda propagates through wrapped blocks and is then called

    // closure: inner function uses outer function's parameter
    CHECK(interpret("f = fun a:Int b:Int c:Int { "
                    "w=fun c1:Int {a / b - c1}; w c }; f 10 2 3") == "2");
    // closure: outer closure used by inner function
    CHECK(interpret("f = fun a:Int b:Int c:Int { "
                    "g=fun c1:Int {a * b - c1}; "
                    "h=fun c1:Int {g c1}; "
                    "h c }; f 1 2 3") == "-1");
    CHECK(interpret("f = fun a:Int b:Int c:Int { "
                    "u=fun b2:Int {a + b2}; v=fun c2:Int {c2 + b}; "
                    "w=fun b1:Int c1:Int {a + u b1 + v c1}; "
                    "w b c }; f 1 2 3") == "9");

    CHECK(interpret("outer = fun y:Int {"
                    "inner = fun x:Int { x + y }; inner y "
                    "}; outer 2") == "4");
    CHECK(interpret("outer = fun y:Int {"
                    "inner = fun x:Int { x + y }; alias = inner; alias y "
                    "}; outer 2") == "4");
    CHECK(interpret("outer = fun y {"
                    "inner = fun x:Int { x + y }; alias = fun x:Int { inner x }; alias y "
                    "}; outer 2") == "4");
}


TEST_CASE( "Partial function call", "[script][interpreter]" )
{
    // partial call: `add 1` returns a lambda which takes single argument
    CHECK(interpret("(add 1) 2") == "3");
    CHECK(interpret("{add 1} 2") == "3");
    CHECK(interpret("f={add 1}; f 2") == "3");
    CHECK(interpret("f=fun x:Int {add x}; f 2 1") == "3");
    CHECK(interpret("f=fun x:Int {add 3}; f 2 1") == "4");
    CHECK(interpret("f=fun x:Int y:Int z:Int { (x - y) * z}; g=fun x1:Int { f 3 x1 }; g 4 5") == "-5");
    CHECK(interpret("f=fun x:Int y:Int { g=fun x1:Int z1:Int { (y - x1) / z1 }; g x }; f 1 10 3") == "3");
    CHECK(interpret("f = fun a:Int b:Int { "
                    "u=fun b2:Int {a + b2}; v=fun c2:Int {c2 - b}; "
                    "w=fun b1:Int c1:Int {a * u b1 / v c1}; "
                    "w b }; f 1 2 3") == "3");
    // [closure.fire] return closure with captured closures, propagate arguments into the closure
    CHECK(interpret("f = fun a:Int { "
                    "u=fun b2:Int {a / b2}; v=fun c2:Int {c2 - a}; "
                    "fun b1:Int c1:Int {a + u b1 + v c1} }; f 4 2 3") == "5");
}


TEST_CASE( "Generic functions", "[script][interpreter]" )
{
    // `f` is a generic function, instantiated to Int->Int by the call
    CHECK(interpret("f=fun x {x + 1}; f (f (f 2))") == "5");
    // generic functions can capture from outer scope
    CHECK(interpret("a=3; f=fun x {a + x}; f 4") == "7");
    // generic type declaration
    CHECK(interpret_std("f = fun x:T y:T -> Bool with (Eq T) { x == y }; f 1 2") == "false");
}


TEST_CASE( "Lexical scope", "[script][interpreter]" )
{
    CHECK(interpret("{a=1; b=2}") == "");
    CHECK_THROWS_AS(Interpreter().eval("{a=1; b=2} a"), UndefinedName);

    CHECK(interpret("x=1; y = { x + 2 }; y") == "3");
    CHECK(interpret("a=1; {b=2; {a + b}}") == "3");
    CHECK(interpret("a=1; f=fun b:Int {a + b}; f 2") == "3");

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


TEST_CASE( "Lists", "[script][interpreter]" )
{
    CHECK(interpret("[1,2,3] ! 2") == "3");
    CHECK_THROWS_AS(Interpreter().eval("[1,2,3]!3"), IndexOutOfBounds);
    //CHECK(interpret("[[1,2],[3,4],[5,6]] ! 1 ! 0") == "3");
    CHECK(interpret("head = fun l:[Int] -> Int { l!0 }; head [1,2,3]") == "1");
}


TEST_CASE( "Type classes", "[script][interpreter]" )
{
    CHECK(interpret("class XEq T { xeq : T T -> Bool }; "
                    "instance XEq Int32 { xeq = { __equal_32 } }; "
                    "xeq 1 2") == "false");
}


TEST_CASE( "Compiler intrinsics", "[script][interpreter]" )
{
    // function signature must be explicitly declared, it's never inferred from intrinsics
    // parameter names are not needed (and not used), intrinsics work directly with stack
    // e.g. __equal_32 pulls two 32bit values and pushes 8bit Bool value back
    CHECK(interpret("my_eq = fun Int32 Int32 -> Bool { __equal_32 }; my_eq 42 (2*21)") == "true");
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
        ((add1 1 6) +          //  7
        (add2 3 4))            //  8  (+10 from state)
    )");
    CHECK(result.type() == Type::Int32);
    CHECK(result.get<int32_t>() == 24);
}
